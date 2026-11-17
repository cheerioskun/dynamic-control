# dynamic-control

An LLVM 23 compiler extension and runtime for dynamically selecting function
instrumentation through atomic function-pointer dispatch.

Status: the runtime contract and its handwritten ABI sanity fixture work. The
annotated compiler pass and Google Benchmark fixture are the next checkpoints.

## Runtime sanity check

With CMake and a C++17 compiler available, run `make test`. This currently tests
the runtime with handwritten dispatch, not compiler-generated instrumentation.

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

## Planned LLVM patch workflow

Follow the workflow in [LDB](https://github.com/cheerioskun/ldb):

- Pin `third_party/llvm` to an unmodified upstream LLVM 23 commit.
- Track compiler changes in `patches/llvm/`, alongside runtime and tests.
- Export local LLVM development commits using `git format-patch` and an ordered
  `series` file. Keep the recorded submodule pin at upstream.
- Add setup hooks and isolated patch-replay checks when bootstrapping the
  toolchain; these are not installed or implemented yet.

See [the repository layout](docs/layout.md) and
[the open design questions](docs/design.md).
