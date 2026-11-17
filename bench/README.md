# First dispatch benchmark

```sh
make compiler
make bench
```

The build fetches pinned Google Benchmark v1.10.0 with a SHA-256 check. Only
`targets.cc` is compiled by patched LLVM 23, using C++17, O2, no exceptions/LTO,
and `-mllvm -dc-enable`. Harness, library, runtime, and mock use the same fixed
host compiler. Both targets are noinline and calculate `x * 3 + 1`.

One binary provides three measurements:

| Mode | Target | Path |
|---|---|---|
| baseline | Unannotated | Original function |
| unhooked | Annotated, disabled | Atomic load, tail dispatch, body |
| hooked | Annotated, enabled | Atomic load, tail dispatch, acquire, body, release |

The runner makes seven randomized blocks of these modes, one fresh process per
case, with at least 0.2 seconds per measurement. Initialization, lookup, toggles,
and mode selection occur outside timing. The timed loop has a direct target
call, a data dependency on the prior result, and `DoNotOptimize`.

Raw Google Benchmark JSON, logs, a manifest (binary/source hashes, LLVM tree,
target compiler command, run order, machine metadata), and summary stay under
ignored `build/bench/results/<UTC run>/`. Summaries report CPU ns/call, medians,
and differences of medians; inspect individual samples before interpreting small
differences. They include loop overhead and the chosen call dependency pattern.

```sh
build/bench/dc_bench --dc-mode=hooked --benchmark_min_time=0.2s
python3 bench/run.py --repetitions 7 --min-time 0.2s
```

Unhooked minus baseline estimates the cost of making this boundary controllable.
Hooked minus unhooked includes both the guard and always-allow mock calls. The
mock has no counters, controller, real permits, or contention. The baseline is
an untransformed function in the same binary, not a separate stock-Clang build.
No application-speedup, real semaphore, or cross-machine claim follows.
