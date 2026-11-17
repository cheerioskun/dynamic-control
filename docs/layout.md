# Minimal repository layout

The runtime, compiler pass, compiler workflow, and integration tests are
implemented. The benchmark is the next checkpoint.

```text
README.md
Makefile                          setup, compiler, test, bench, check-patches
toolchain.json                    upstream URL, exact LLVM 23 revision, build settings
third_party/llvm/                 upstream submodule, recorded pin stays unmodified
patches/llvm/
  series
  0001-Add-dynamic-control.patch  pass, pipeline integration, LLVM regression tests
scripts/
  llvm.py                         adapt LDB setup/export/check workflow
  build-llvm.sh                   build the patched native LLVM toolchain
.githooks/
  pre-commit                     validate/export patch series
  llvm/post-commit               export local compiler commits
runtime/
  CMakeLists.txt
  dynamic_control.h              descriptor and small C ABI
  dynamic_control.cc             registration, lookup, atomic enable/disable
  msem_mock.cc                    out-of-line acquire/release; always succeeds
bench/
  CMakeLists.txt                 Google Benchmark and two executable builds
  bench.cc                       shared benchmark loop and untimed setup
  targets.cc                     noinline target functions in a separate TU
  run.py                         build/run cases and save JSON plus provenance
  README.md                      commands and interpretation
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
patches. Do not keep a second copy under a parent `compiler/` directory. Start
with one pass and its integration/tests in one patch. Add a frame-lowering patch
only if a demonstrated requirement needs it.

## Types and functions

- One LLVM pass class, `DynamicControlPass`. Ordinary helper functions perform
  eligibility checks, generate dispatcher/guard functions, and emit descriptors.
- One compiler/runtime descriptor, `DcFunction`: name, body address, guard
  address, and location of the atomic dispatch slot. This is a conceptual field
  list; the implementation must define and verify the shared ABI and alignment.
  Calls remain signature-correct in generated IR; runtime metadata is not used
  to invoke functions through a generic signature.
- A linked registry of module tables, populated by compiler-emitted constructors.
  Registration needs no heap allocation. Lookup and toggling stay outside timed
  benchmark loops. Registered modules must remain loaded until process exit.
- Small runtime operations: register descriptors, find a function, enable or
  disable it. Keep atomic publication in one implementation.
- Two external semaphore operations: `dc_acquire` and `dc_release`. The initial
  runtime supplies the always-allow implementation in `runtime/msem_mock.cc`.
  Benchmarks and smoke tests link that runtime. Replacing it with the real
  semaphore does not change dispatch generation. Failure and nesting policy
  remain open; the mock does not validate those behaviors.

No controller class, strategy hierarchy, generic instrumentation framework, or
plugin system is needed. Runtime/control code, semaphore helpers, and benchmark
harness code must be excluded from transformation.

## Benchmark comparison

| Case | Target compiler | Dispatch state |
|---|---|---|
| Baseline | Patched LLVM 23, unannotated target | Original function |
| Unhooked | Patched LLVM 23 | Slot selects body |
| Hooked | Patched LLVM 23 | Slot selects guard with always-allow mock |

The first fixture uses one binary with two identically implemented noinline
targets, only one annotated. The unannotated target is the baseline. Compile
only the target translation unit with the extension; keep support code fixed.
An independent unmodified-Clang comparison can follow; these first numbers do
not establish equivalence of the two compiler builds.

All three measurements use the same executable. Select
the path before timing, and run each case in a fresh process with repeated
measurements. Start with no LTO, a noinline target, observable inputs/results,
and out-of-line mock operations so optimization cannot erase the measured calls.
Check emitted assembly to establish what each case actually executes.

Report absolute ns/call and these differences with run-to-run variation:

- Unhooked minus baseline: cost of keeping dispatch available.
- Hooked minus unhooked: incremental guard and mock-call cost.

The latter includes wrapper overhead and the mock's work; it does not isolate
them. These measurements do not establish real memory-semaphore overhead,
contention behavior, or application performance. Add a direct acquire/body/release
control later if separating those costs becomes necessary.

Write raw Google Benchmark JSON, compiler revisions, commands/flags, and machine
metadata under ignored `build/bench/results/<run>/`. Keep correctness counters
in the smoke fixture rather than adding counter overhead to the timed mock.
