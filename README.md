# dynamic-control

An LLVM 23.1.6 pass and runtime for toggling memory-semaphore hooks on annotated
functions. Dispatch uses atomic function pointers. No frame-lowering changes.

[Start at the checkpoint](CHECKPOINT.md) for a reading order, design choices,
saved artifacts, and verification commands.

[Netbench on Shenango](results/netbench-shenango/README.md) records the application
comparison with hardcoded instrumentation, its results, and the application patch.

## Build and run

```sh
docker run --rm -it cheerioskun/dynamic-control
```

Inside the container:

```sh
make bench
```

The image includes the built patched compiler, dependencies, and benchmark
source. Building the image takes time; running the benchmark does not rebuild
LLVM. `make test test-compiler` runs the behavior checks.

The published image supports Linux amd64 and arm64. To build it yourself:

```sh
docker build --build-arg DC_BUILD_JOBS=8 -t cheerioskun/dynamic-control .
```

For native development, install Git, Python 3, CMake, Ninja, and a C++ compiler,
then run `make compiler`. `DC_BUILD_JOBS` controls build parallelism (default 2).
See [development](docs/development.md) and [bench](bench/README.md).

Docker standardizes the build environment. Timings still belong to the machine
running it; Docker Desktop on a Mac runs Linux in a VM.

Shenango still needs a prepared Linux host: its kernel module, hugepages, and
network/device setup are outside the image. Its container setup is not wired
into this image yet.

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

The LLVM release is selected in `toolchain.json`; compiler changes live in
`patches/llvm/`. Setup applies them with `git am`.
