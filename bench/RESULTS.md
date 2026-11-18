# Prototype sanity measurements

## Full LTO, annotation as a hint

2026-11-17. The pass now runs after per-file optimization, before LTO. It prepares
remaining annotated definitions; it does not preserve calls or instrument copies
already inlined. Full and ThinLTO pass the existing O0/O2 execution fixture with
preparation off/on. The benchmark below uses **full LTO** for targets, harness,
runtime, and synthetic msem mock; Google Benchmark v1.10.0 remains native.
LLVM Clang 23 compiles the whole fixture, including the harness/runtime that used
host compilers in earlier runs. These are new builds, not an isolated measurement
of changing only the LTO flag.

Seven randomized blocks, one process per case. M5: minimum 0.2 s per sample.
Xeon E5530: minimum 0.5 s, pinned to CPU 2; scaling and ASLR enabled.

| Machine | Path | Median CPU ns/call | Min–max CPU ns/call |
|---|---|---:|---:|
| Xeon E5530 | Baseline | 2.636 | 2.628–2.639 |
| Xeon E5530 | Unhooked | 3.017 | 3.016–3.019 |
| Xeon E5530 | Hooked | 73.960 | 73.927–75.557 |
| Apple M5 | Baseline | 1.357 | 1.356–1.362 |
| Apple M5 | Unhooked | 1.257 | 1.077–1.641 |
| Apple M5 | Hooked | 28.411 | 28.274–28.982 |

Hooked minus unhooked is **70.943 ns/call on the Xeon** and **27.154 ns/call on
the M5**, between medians. Xeon unhooked minus baseline is 0.381 ns in this run;
M5 unhooked variance leaves its dispatch penalty unresolved. Final linked assembly retains
the acquire-load and indirect branch, the guard's acquire/body/release calls,
and both timed target calls. The synthetic controller and clock checks remain.

Saved samples and provenance: [Xeon](results/lto-linux-x86_64.json),
[M5](results/lto-macos-arm64.json). The Xeon run completed with `.done`, exit 0.
Its frozen binary, logs, JSON, and disassembly are on the node and locally under
`build/node-runs/20261118T031720Z-lto/`. Mac artifacts and binary are under
`build/bench/results/lto-macos-arm64-20261117/`.

## Synthetic Svalinn msem

2026-11-17. The [portable Svalinn controller](../runtime/msem_mock.md) replaces
the empty hooks. Each acquire/release reads the real steady clock, and the
controller runs every 3 ms with synthetic bandwidth. Gaussian samples are
initialized before timing. Entry always succeeds; no permit accounting or locks.

Same three-path arithmetic fixture, LLVM 23.1.6 patch, and Google Benchmark
v1.10.0. Seven randomized blocks, one process per case. Xeon: CPU 2, minimum
0.5 s per sample, GCC 13.3 harness/runtime. M5: minimum 0.2 s, Apple Clang 21
harness/runtime. CPU scaling and ASLR were left enabled on the Xeon.

| Machine | Path | Median CPU ns/call | Min–max CPU ns/call |
|---|---|---:|---:|
| Xeon E5530 | Baseline | 2.263 | 2.263–2.264 |
| Xeon E5530 | Unhooked | 3.395 | 3.394–3.412 |
| Xeon E5530 | Hooked | 74.729 | 73.197–77.837 |
| Apple M5 | Baseline | 1.358 | 1.356–1.466 |
| Apple M5 | Unhooked | 1.059 | 0.769–1.376 |
| Apple M5 | Hooked | 28.474 | 28.340–28.870 |

Hooked minus unhooked is **71.334 ns/call on the Xeon** and **27.415 ns/call on
the M5**, between medians. These include two clock checks and amortized controller
work. M5 unhooked variance still leaves its dispatch penalty unresolved. The
Xeon's unhooked-minus-baseline difference is 1.132 ns in this build; the changed
baseline and unhooked values mean comparisons with the earlier binary also
include code layout and run conditions.

The linked binaries retain the dispatcher, guard, clock checks, and controller
calculations. On x86, both hooks call `update_capacity`; it calls
`steady_clock::now`, branches on elapsed time, and retains reward updates,
random sampling, and softmax exponentiation. The node's runtime, compiler, and
patch-replay checks all passed. The detached run completed with `.done`, exit 0.

Saved samples and provenance:
[Xeon](results/synthetic-msem-linux-x86_64.json),
[M5](results/synthetic-msem-macos-arm64.json).
Raw artifacts remain under `build/node-runs/20261117T214122Z-msem/` on the node
and locally, and `build/bench/results/msem-macos-arm64-20261117/` on the Mac.
The node's new binary is `build/bench/dc_bench`, with a frozen copy in its run
directory. The previous empty-hook binary is preserved in
`build/node-runs/20261117T190149Z/dc_bench`; its SHA-256 matches the saved results.

## Empty hooks: Xeon E5530, Linux x86_64

2026-11-17, Intel Xeon E5530 @ 2.40 GHz (Nehalem), four cores/eight threads.
Patched LLVM 23.1.6 compiled the targets; GCC 13.3 compiled the harness, runtime,
mock, and Google Benchmark v1.10.0. One binary, identical noinline arithmetic
targets, out-of-line always-allow acquire/release. Seven randomized blocks,
a fresh process per case, minimum 0.5 seconds per measurement, pinned to CPU 2.
CPU scaling and ASLR were enabled.

| Path | Median CPU ns/call | Min–max CPU ns/call |
|---|---:|---:|
| Baseline | 2.639 | 2.639–2.640 |
| Unhooked | 3.018 | 3.016–3.019 |
| Hooked | 8.298 | 8.296–8.306 |

Unhooked minus baseline is **0.378 ns/call**; hooked minus unhooked is
**5.280 ns/call**, both differences of medians. The samples are tightly grouped
within this run. The M5's sub-nanosecond hooked increment does not carry over to
this Xeon. These measurements include the loop and its data dependency, with
empty semaphore hooks.

The linked dispatcher retains the slot load and indirect jump:

```asm
movq bench_controlled.dc_target(%rip), %rax
jmpq *%rax
```

The guard retains separate calls to `dc_acquire`, `bench_controlled.dc_body`, and
`dc_release`, with register saves and restores around them. Both timed loops
retain direct calls to their intended targets. Plain and body have identical
arithmetic instructions. Runtime checks, the LLVM IR test, compiler execution
checks at O0/O2 with preparation on/off, and patch replay all passed.

[Saved samples, ranges, and provenance](results/sanity-linux-x86_64.json).
The detached job finished with exit code 0 and `.done`. Full JSON/logs/manifest
and disassembly are preserved on the node and copied locally under
`build/node-runs/20261117T190149Z/`.

## Empty hooks: Apple M5, macOS ARM64

2026-11-17, Apple M5, macOS ARM64. LLVM 23.1.6 with the annotated dispatch patch;
Google Benchmark v1.10.0. One binary, identical noinline arithmetic targets,
out-of-line always-allow acquire/release. Seven randomized blocks, a fresh
process per case, minimum 0.2 seconds per measurement.

| Path | Median CPU ns/call | Min–max CPU ns/call |
|---|---:|---:|
| Baseline | 1.369 | 1.359–1.415 |
| Unhooked | 1.229 | 0.845–1.574 |
| Hooked | 2.075 | 2.064–2.095 |

Hooked minus unhooked is 0.846 ns/call between medians. The unhooked samples span
0.845–1.574 ns/call, leaving the dispatch penalty unresolved. This fixture measures
the wrapper and empty acquire/release calls together.

The linked assembly retains both timed target calls. Baseline and body have
identical arithmetic instructions. The ARM64 dispatcher contains an acquire
pointer load and an indirect branch; the guard calls acquire, body, and release.

[Saved samples and source hashes](results/sanity-macos-arm64.json) retain the
small comparison. Full raw JSON/logs/manifest are local under
`build/bench/results/20261117T184322.047161Z/`.

Reproduce with `make bench`; see [the fixture commands](README.md).
