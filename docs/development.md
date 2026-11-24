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

For a detached Linux build and benchmark, see [the node runner](../bench/README.md#detached-linux-node-run).

## Shenango application patches

The same dependency-patch engine backs `scripts/llvm.py` and
`scripts/shenango.py`. `shenango.json` records upstream; the staged
`third_party/svalinn-shenango` gitlink must match it. `make setup-shenango`
prepares the application and installs its post-commit export hook.
`make check-shenango-patches` reconstructs its patch series in a temporary Git
index without touching source or the developer index. Neither setup command
automatically builds the application's nested dependencies.

Edit and commit inside the prepared application submodule, then commit the
generated patch files in the parent. Preserve the upstream gitlink rather than
staging the local patched HEAD. Parent commits check both patch series and can
be blocked by unfinished edits inside either dependency. Do not reset unrelated
compiler work to record an application change.

The [experiment entry point](../experiments/shenango/README.md) owns verification
scope, provenance, results, and application reproduction commands.
