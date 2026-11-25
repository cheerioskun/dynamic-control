# Development

The Docker image includes a built patched LLVM and the tools needed for the
empty-hook benchmark. From its shell, run `make bench` or `make test test-compiler`.

For native builds, `make compiler` fetches the release selected in
`toolchain.json`, applies `patches/llvm/series`, and builds the native backend.
An existing LLVM development checkout with local commits is left intact.
Setup will not overwrite uncommitted source edits.

Compiler source lives in `third_party/llvm`; build output is in `build/llvm`.
`DC_BUILD_JOBS` controls parallelism, defaulting to two jobs. Extra CMake options
can be passed to `scripts/build-llvm.sh`.

Edit and commit compiler changes locally inside the LLVM checkout. Export them
with ordinary `git format-patch` relative to the release tag, and list the patch
filenames in `patches/llvm/series`. There are no automatic commit hooks or
source-tree matching checks. The parent submodule pin stays at upstream.

Application-specific patches and result notes belong together under `results/`.
The benchmark source and empty hooks live in `bench/`.
