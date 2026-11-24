# LLVM development

LLVM is pinned to `llvmorg-23.1.6`. The submodule Git link and `toolchain.json`
identify the same unmodified upstream commit. Compiler changes live in
`patches/llvm/`, alongside runtime and tests.

Setup/export/check tools and Git fixture tests come from
[LDB](https://github.com/cheerioskun/ldb/tree/2eb5977f0b774631e31440245c8ca278440c1275).

```sh
make setup
make compiler
make test
make test-compiler
make test-tooling
make check-patches
```

- `test`: runtime checks with handwritten dispatch.
- `test-compiler`: IR and execution checks at O0/O2, with the switch on and off.
- `test-tooling`: Git workflow fixtures, without downloading LLVM.
- `check-patches`: replay staged patches against the upstream pin.

## Build settings

Requires Git, Python 3, CMake, Ninja, and a C++17 compiler. Build output stays
under `build/llvm/`. `DC_BUILD_JOBS` sets parallelism (default 2); only the native
backend is built. Setup includes LLVM's `libc` source for common headers without
building the libc runtime. Extra CMake settings go to `scripts/build-llvm.sh`.
The build exports `build/llvm/compile_commands.json`; `.clangd` points the editor
there so LLVM's source and generated headers resolve.

CMake and Ninja can be installed inside the checkout:

```sh
python3 -m venv build/tools
build/tools/bin/pip install cmake ninja
export PATH="$PWD/build/tools/bin:$PATH"
```

## Commit compiler changes

Edit and commit inside `third_party/llvm`. Its post-commit hook exports and stages
the patch series in the parent. Stage related runtime/test changes and commit
the parent normally. Its pre-commit hook verifies and re-exports the series.

Keep the recorded submodule pin at upstream; do not stage its local development
HEAD. `python3 scripts/llvm.py export` exports manually. Setup preserves local
source edits; checks use an isolated index. Run `make setup` in every clone to
activate the hooks.

For the empty-hook microbenchmark, see [bench](../bench/README.md).
