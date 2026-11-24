# Empty-hook microbenchmark

`targets.cc` contains identical plain and annotated arithmetic functions.
`bench.cc` times baseline, prepared-but-disabled, and enabled calls.
`empty_hooks.cc` supplies no-op acquire/release hooks. The runtime library
contains only the dispatch registry; applications provide their own hooks.

```sh
make bench
```

Only the targets use the patched compiler by default. The harness and Google
Benchmark use the host compiler. Lookup, toggling, and mode selection are outside
timing. The runner prints medians from seven randomized blocks.

For one path:

```sh
build/bench/dc_bench --dc-mode=hooked --benchmark_min_time=0.2s
```

`DC_LTO=ON` enables full LTO when configuring with the matching patched compiler.
Empty hooks may be optimized away under LTO.

Measurements and interpretation live in
[results/hook-overhead](../results/hook-overhead/README.md).
