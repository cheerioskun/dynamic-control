# Prototype sanity measurements

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
