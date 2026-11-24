# dynamic-control

An LLVM 23.1.6 pass and runtime for toggling memory-semaphore hooks on annotated
functions. Dispatch uses atomic function pointers. No frame-lowering changes.

[Start at the checkpoint](CHECKPOINT.md) for a reading order, design choices,
saved artifacts, and verification commands.

[Netbench on Shenango](results/netbench-shenango/README.md) records the application
comparison with hardcoded instrumentation, its results, and the application patch.

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
empty-hook microbenchmark.

## Select and toggle functions

```cpp
#include "dynamic_control.h"

extern "C" [[clang::annotate("dc")]]
int target(int x) noexcept { return x + 1; }
```

Compile application targets with `-mllvm -dc-enable`. The annotation is a hint:
the pass prepares definitions remaining after per-file optimization. It does not
preserve calls or hook copies already inlined. Full LTO and ThinLTO are supported;
the flag is only needed at compilation. Other functions compile normally.

```cpp
if (const DcFunction *function = dc_find("target")) {
    dc_enable(function);
    dc_disable(function);
}
```

Names are exact linkage names; `extern "C"` keeps them simple. `dc_find` returns
null for missing names; names are unique in this prototype. Prepared functions
start disabled. Toggling affects future entries; calls already in a guard finish
and execute their release hook.

## Measurements and source

[Hook-overhead results](results/hook-overhead/README.md) compare baseline,
unhooked, and empty-hook calls. The microbenchmark code lives in `bench/`.

LLVM stays pinned to unmodified upstream source. Compiler edits are committed
locally inside the submodule and exported to `patches/llvm/`, following
[LDB's workflow](https://github.com/cheerioskun/ldb). See
[the checkpoint](CHECKPOINT.md) and [development commands](docs/development.md).
