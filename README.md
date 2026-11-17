# dynamic-control

An LLVM 23 compiler extension and runtime for dynamically selecting function
instrumentation through atomic function-pointer dispatch.

LLVM 23.1.6 is pinned. The runtime, annotated IR pass, and LLVM patch workflow
are implemented. Compiler sanity checks pass at O0 and O2 on macOS ARM64.
The Google Benchmark fixture is the next checkpoint.

## Runtime sanity check

With CMake and a C++17 compiler available, run `make test`. This tests the
runtime independently with handwritten dispatch. Compiler checks are separate:

```sh
make setup
make compiler                   # also needs Ninja; initial build is substantial
make test-compiler               # real transformed code, O0/O2, switch on/off
make test-tooling
make check-patches
```

See [development](docs/development.md) for prerequisites and local tool paths.

## Selection and control

```cpp
extern "C" [[clang::annotate("dc")]]
int target(int x) noexcept { return x + 1; }
```

Compile selected application translation units with `-mllvm -dc-enable`.
Only annotated definitions are prepared. Compile runtime and harness sources
without that flag. Without the flag, annotations are inert.

```cpp
if (const DcFunction *function = dc_find("target")) {
    dc_enable(function);
    dc_disable(function);
}
```

Handles require a successful lookup. Names are exact linkage names; `extern
"C"` keeps fixture names simple. All prepared functions start disabled.

This prototype assumes 64-bit pointers and ordinary nonthrowing, fixed-arity
fixture functions. Use `noexcept` or `-fno-exceptions`; leave LTO off. General ABI,
unwinding, permit-failure, and nested-permit handling are future work. The pass
makes selected boundaries noinline; the benchmark baseline is also noinline.

## First demonstration

For each eligible function, emit:

- `foo`: the stable function entry; atomically load `foo_target` and tail-dispatch
  with the original arguments and calling convention.
- `foo_body`: the original implementation.
- `foo_guard`: acquire a memory-semaphore permit, call `foo_body`, then release
  the permit. Start with a wrapper; measure before considering a cloned body.
- `foo_target`: a writable atomic function pointer, initially `foo_body`.

Enable instrumentation by publishing `foo_guard`; disable it by publishing
`foo_body`. The slot must never target the dispatcher itself.

The first demonstration uses pointer updates. Binary rewriting is deferred.
Switching affects future entries; invocations already admitted finish their
chosen path and release any permit they acquired.
Coverage is an explicit design question: source functions versus surviving
machine functions, optimization and inlining, code compiled by other toolchains,
and the meaning of a possible six-level limit from an entrypoint.

## LLVM patch workflow

Follow the workflow in [LDB](https://github.com/cheerioskun/ldb):

- Pin `third_party/llvm` to the unmodified upstream LLVM 23.1.6 commit.
- Track compiler changes in `patches/llvm/`, alongside runtime and tests.
- Export local LLVM development commits using `git format-patch` and an ordered
  `series` file. Keep the recorded submodule pin at upstream.
- Setup installs export hooks; isolated patch-replay and Git workflow checks
  protect the pin and preserve unrelated source/index changes.

See [the repository layout](docs/layout.md) and
[the open design questions](docs/design.md).
