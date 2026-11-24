# Shenango instrumentation parity

**Experiment:** can LLVM-generated dynamic-control instrumentation replace
explicit `Wait() → memory work → Post()` in the same netbench application?

We chose to verify two things:

1. **Correctness: parity with hardcoded instrumentation.** Run the same work
   with the same arguments and results, block until acquisition succeeds, and
   release every acquired permit. Check contending user threads and that an
   active guard still releases after instrumentation is disabled.
2. **Performance: similar performance to hardcoded instrumentation.** Compare
   worker execution and netbench RPC throughput under matching configurations,
   with repeated paired runs to make variation visible.

Both variants use the same workload, real blocking semaphore, adaptive
controller, counters, compiler settings, and machine configuration. Neither
uses permit TLS: C++ OS-thread-local permit state would be incorrect for
Shenango's migrating user threads. Correctness-build counters are global
atomics and are compiled out of timed binaries.

This evaluates the instrumentation mechanism. It does not test whether the
control policy improves the application. Similar performance was an empirical
comparison, not a preregistered statistical equivalence test or a proven
overhead bound.

## Evidence

- [Correctness logs and generated-code checks](results/amd134/): enabled and
  disabled control, forced contention, disable during an active invocation,
  and 1,024 checked network responses per variant.
- [Single-core measurements](performance/amd134/RESULTS.md): nine paired worker
  runs and seven RPC pairs, including enabled and disabled worker cases.
- [Multicore measurements](performance/amd134/MULTICORE.md): 2/4/8/16 Shenango
  scheduler threads, five pairs per point, with balanced permits in every
  correctness case. Scheduler-thread counts include SMT hardware threads.

The recorded correctness cases passed. Multicore median paired RPC differences
were within 0.2%, while individual pairs varied by up to 6.8%. This supports
parity in the exercised cases and similar observed throughput, with the limits
described in the result reports. The counter/backend accuracy question remains
separate and unchanged in both variants.

## One owning repository

`dynamic-control` owns the experiment, results, compiler extension, and runtime.
The application is a pinned dependency, using the same patch workflow as LLVM:

```text
third_party/llvm/                  pinned upstream compiler
patches/llvm/                      compiler changes
runtime/                           dispatch registry and ABI
third_party/svalinn-shenango/       pinned upstream application
patches/svalinn-shenango/           application integration and test/build drivers
experiments/shenango/              experiment statement and recorded evidence
```

The Shenango gitlink stays at upstream commit
`9c2d6b79f857eb74c95d6032f400403d518e7564`, matching `shenango.json`.
The patch series first reproduces the tested application integration, then
adapts repository paths and uses the parent registry instead of a copied one.
Only the application-specific semaphore hooks live in the application patch.
Do not link the standalone synthetic `msem_mock.cc` backend into Shenango.

The earlier `svalinn-shenango` branch at `b2ecf1c` remains historical provenance;
continued development belongs in this repository and its prepared submodule.
The separate LDB debugger and earlier mahoraga research are not dependencies of
this experiment and were not merged into it.

## Prepare, edit, export

From this repository:

```sh
make setup-shenango
make check-shenango-patches
```

As with LLVM, edit and commit inside the prepared submodule. Its post-commit
hook exports and stages `patches/svalinn-shenango/`. The parent pre-commit hook
validates both dependency series. Do not stage a locally patched submodule HEAD
as the upstream pin. `make setup-shenango` preserves local commits and edits;
it does not reset a developer checkout. Explicit export is available as
`python3 scripts/shenango.py export`.

Building and running requires a prepared Linux Shenango machine and its normal
upstream dependencies; preparing the outer submodule does not build them.
After the parent patched Clang and Shenango libraries are built:

```sh
make -C third_party/svalinn-shenango/breakwater/apps/netbench/src -f Makefile.parity MODE=hardcoded parity scaling
make -C third_party/svalinn-shenango/breakwater/apps/netbench/src -f Makefile.parity MODE=auto parity scaling
make -C third_party/svalinn-shenango/breakwater/apps/netbench/src -f Makefile.parity MODE=hardcoded VERIFY=1 selftest
make -C third_party/svalinn-shenango/breakwater/apps/netbench/src -f Makefile.parity MODE=auto VERIFY=1 selftest
python3 third_party/svalinn-shenango/breakwater/apps/netbench/src/bench_multicore.py --source-commit "$(git rev-parse HEAD)" --cores 2 4 8 16 --output "$PWD/build/experiments/shenango/new-run"
```

Run directories must be new. New raw output goes under ignored `build/` until
selected for an experiment record. Source, configuration, and binary hashes
must accompany reported measurements.

## Historical results and this migration

All archived result files were copied byte-for-byte from the historical working
branch. [Provenance](provenance.json) records the source revision, old prefix,
file count, and archive digest. Original paths and source/binary hashes inside
run manifests remain historical facts; they were not rewritten to suggest the
new repository layout was measured. The second application patch changes
layout and runtime sourcing, not the recorded observations. This migration was
validated by exact patch replay, source comparison, archive hashing, and Git
workflow tests; it is not a new runtime/performance run.
