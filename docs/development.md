# LLVM development

LLVM is pinned to upstream `llvmorg-23.1.6`. The submodule Git link and
`toolchain.json` must identify the same unmodified upstream commit. Compiler
changes belong in `patches/llvm/`, alongside runtime and tests.

The setup/export/check tools and their real-Git fixture tests are adapted from
`~/repos/ldb`. The workflow is the same:

```sh
make setup
make compiler                 # CMake, Ninja, host C++ compiler required
make test-tooling
make check-patches
```

`DC_BUILD_JOBS` controls compiler build concurrency; default 2. Build only the
native backend initially. Output stays under ignored `build/llvm/`. On this Mac,
CMake/Ninja were installed in `build/tools/`; prepend its `bin/` to PATH if needed.

Edit and commit source inside `third_party/llvm`. Its post-commit hook exports
and stages the patch series in the parent. Then stage the related parent files
and commit normally. The parent pre-commit hook verifies/re-exports patches.

Do not stage the LLVM submodule's development HEAD as an upstream upgrade.
The parent pin stays upstream even while the local checkout contains patch
commits. `python3 scripts/llvm.py export` is the manual export command.

Setup is idempotent and preserves developer edits. Patch checking reconstructs
the staged series in an isolated index. Hooks reject dirty compiler source,
manual patch edits that would be overwritten, and accidental pin changes.
Run setup in every clone to install the hooks.

Source/IR regression checks, runtime checks, and compiler builds are separate
evidence. A successful patch replay is not evidence that the extension works.
