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
and differences of medians. Each sample includes the loop and its data dependency.

```sh
build/bench/dc_bench --dc-mode=hooked --benchmark_min_time=0.2s
python3 bench/run.py --repetitions 7 --min-time 0.2s
```

Unhooked minus baseline estimates the cost of making this boundary controllable.
Hooked minus unhooked includes the guard and synthetic msem calls. The
[Svalinn mock](../runtime/msem_mock.md) checks the real clock on acquire/release
and runs its controller every 3 ms using synthetic bandwidth. Initialization
and Gaussian sample generation happen before timing. It always allows entry,
with no permit accounting or locking. The baseline is the unannotated function
in the same binary. Earlier empty-hook results remain in [RESULTS.md](RESULTS.md).

## Full LTO

Use a separate build directory and LLVM 23 for the harness and runtime too.
Google Benchmark remains a native library. `make compiler` builds the matching
LTO library on macOS and LLD on Linux.

```sh
cmake -S . -B build/bench-lto -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="$PWD/build/llvm/bin/clang++" \
  -DCMAKE_AR="$PWD/build/llvm/bin/llvm-ar" \
  -DCMAKE_RANLIB="$PWD/build/llvm/bin/llvm-ranlib" \
  -DDC_BUILD_BENCH=ON -DBUILD_TESTING=OFF -DDC_LTO=ON
cmake --build build/bench-lto --parallel 2
python3 bench/run.py --binary build/bench-lto/dc_bench
```

The compile-time `-mllvm -dc-enable` is unchanged. LTO may optimize away calls or
inline copies before preparation; the annotation does not preserve them.

## Detached Linux node run

From a checkout on the node, with GCC/G++, CMake, Ninja, LLD, Git, and Python:

```sh
mkdir -p build/node-runs/first
nohup setsid env CC=gcc CXX=g++ DC_BUILD_JOBS=4 DC_BENCH_CPU=2 \
  bash scripts/run-node.sh "$PWD/build/node-runs/first" \
  >build/node-runs/first/launcher.log 2>&1 </dev/null &
```

Use a new run directory each time. The script prepares LLVM, builds the compiler,
runs sanity checks, builds the fixture, saves disassembly, then measures on the
chosen CPU. It uses seven randomized blocks with at least 0.5 seconds per sample.
The run directory holds `run.log`, `pid`, `phase`, `exit-code`, and `.done` on
success or `.failed` on failure. Raw benchmark data is under `results/`.
The built Clang uses a local `clang++.cfg` to select the host G++ library directory.
Set `DC_LTO=1` in the launch environment to run the full-LTO fixture in
`build/bench-lto/`, including the existing smoke fixture with full and ThinLTO.
