# dynamic-control

An LLVM 23.1.6 pass and runtime for toggling memory-semaphore hooks on annotated
functions. Dispatch uses atomic function pointers. No frame-lowering changes.

## Build and run

Requires Git, Python 3, CMake, Ninja, and a C++17 compiler.

```sh
make setup
make compiler
make test test-compiler
make bench
```

`DC_BUILD_JOBS` controls compiler build parallelism (default 2). The compiler
builds the native backend. See [development](docs/development.md) for the patch
workflow and local tool installation, and [benchmarks](bench/README.md) for the
three-path fixture and detached node runner.

## Select and toggle functions

```cpp
#include "dynamic_control.h"

extern "C" [[clang::annotate("dc")]]
int target(int x) noexcept { return x + 1; }
```

Compile application targets with `-mllvm -dc-enable`. The pass prepares annotated
definitions; other functions compile normally. Runtime and harness sources use
the host compiler without that flag.

```cpp
if (const DcFunction *function = dc_find("target")) {
    dc_enable(function);
    dc_disable(function);
}
```

Names are exact linkage names; `extern "C"` keeps them simple. `dc_find` returns
null for missing or ambiguous names. Prepared functions start disabled. Toggling
affects future entries; calls already in a guard finish and release their permit.

## Dispatch

An annotated `foo` keeps its public entry and gains three private symbols:

- `foo.dc_body`: the original implementation.
- `foo.dc_guard`: acquire, call the body, release, return.
- `foo.dc_target`: a writable atomic pointer, initially pointing to the body.

`foo` acquire-loads the slot and tail-dispatches. Enabling publishes the guard;
disabling publishes the body. A module constructor registers a constant table
of names and addresses. Lookup stays off the function-entry path.

The prototype uses nonthrowing, fixed-arity functions with 64-bit pointers and
LTO off. Selected boundaries are noinline. The runtime currently links an
always-allow semaphore mock.

## Measurements and source

[Results](bench/RESULTS.md) include samples and disassembly checks. The fixture
measures baseline, unhooked, and hooked calls in one binary.

LLVM stays pinned to unmodified upstream source. Compiler edits are committed
locally inside the submodule and exported to `patches/llvm/`, following
[LDB's workflow](https://github.com/cheerioskun/ldb). See
[the design](docs/design.md), [layout](docs/layout.md), and
[development commands](docs/development.md).
