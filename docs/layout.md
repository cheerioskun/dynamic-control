# Minimal repository layout

The runtime, compiler pass, compiler workflow, integration tests, and benchmark
are implemented. The pass prepares annotated functions; the benchmark uses the always-allow mock.

```text
README.md
Makefile                          setup, compiler, test, bench, check-patches
toolchain.json                    LLVM revision, release tag, sparse source paths
third_party/llvm/                 upstream submodule, recorded pin stays unmodified
patches/llvm/
  series
  0001-*.patch                   pass, pipeline integration, LLVM regression tests
scripts/
  llvm.py                         adapt LDB setup/export/check workflow
  build-llvm.sh                   build the patched native LLVM toolchain
  run-node.sh                     detached Linux build, sanity checks, benchmarks
.githooks/
  pre-commit                     validate/export patch series
  llvm/post-commit               export local compiler commits
runtime/
  CMakeLists.txt
  dynamic_control.h              descriptor and small C ABI
  dynamic_control.cc             registration, lookup, atomic enable/disable
  msem_mock.cc, msem_mock.h        synthetic Svalinn controller and initialization
  msem_mock.md                    source provenance and retained behavior
bench/
  CMakeLists.txt                 Google Benchmark and one three-path executable
  bench.cc                       shared benchmark loop and untimed setup
  targets.cc                     noinline target functions in a separate TU
  run.py                         run cases and save JSON plus provenance
  README.md                      commands and interpretation
  RESULTS.md                     first sanity measurements
  results/*.json                 saved samples and source hashes per platform
tests/
  runtime.cc                     handwritten ABI/runtime check
  smoke.cc                       compiler-generated dispatch and permit accounting
  targets.cc, targets_extra.cc   annotated/unannotated targets in two modules
  compiler.py                    O0/O2, switch on/off, IR and execution checks
  tooling/test_llvm_workflow.py   retain LDB workflow checks when porting tooling
docs/
  design.md
  layout.md
```

Compiler implementation lives inside the LLVM checkout and is published through
patches. Do not keep a second copy under a parent `compiler/` directory. The pass, pipeline integration, and IR test travel in one patch.

## Types and functions

`DynamicControlPass` generates dispatchers, guards, slots, descriptors, and module
constructors. Compiler source lives in LLVM and travels as a patch.

The runtime has two records: `DcFunction` describes a function; `DcModule` links
its module's table into the registry. C ABI functions provide registration,
lookup, enable/disable, and semaphore acquire/release.

`runtime/msem_mock.cc` supplies the synthetic Svalinn hooks. The smoke fixture
replaces these with counting hooks. The benchmark uses synthetic bandwidth and
real clock checks, with initialization outside timing.

See [the design](design.md) for dispatch and table details, and
[the benchmark guide](../bench/README.md) for measurement commands.
