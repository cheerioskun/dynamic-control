# dynamic-control

An LLVM 23 compiler extension and runtime for dynamically selecting function
instrumentation through atomic function-pointer dispatch.

Status: design sketch. No compiler extension, LLVM checkout, runtime, or demo
has been implemented. The exact LLVM 23 upstream revision remains to be pinned.

## Proposed first demonstration

For each eligible function, emit:

- `foo`: the stable function entry; atomically load `foo_target` and tail-dispatch
  with the original arguments and calling convention.
- `foo_body`: the original implementation.
- `foo_guard`: the instrumentation path, with behavior to be agreed.
- `foo_target`: a writable atomic function pointer, initially `foo_body`.

Enable instrumentation by publishing `foo_guard`; disable it by publishing
`foo_body`. The slot must never target the dispatcher itself.

The first demonstration uses pointer updates. Binary rewriting is deferred.
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

See [the open design questions](docs/design.md).
