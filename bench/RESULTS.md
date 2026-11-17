# Prototype sanity measurements

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
