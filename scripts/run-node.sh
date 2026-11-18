#!/usr/bin/env bash
# Linux prototype job. Launch with nohup/setsid; use a fresh output directory.
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
run_dir=${1:?Usage: run-node.sh OUTPUT_DIRECTORY}
mkdir -p "$run_dir"
run_dir=$(cd "$run_dir" && pwd)
exec >"$run_dir/run.log" 2>&1
echo "$$" >"$run_dir/pid"
finish() {
    status=$?
    printf '%s\n' "$status" >"$run_dir/exit-code"
    date -u +%FT%TZ >"$run_dir/finished-utc"
    if [ "$status" -eq 0 ]; then
        touch "$run_dir/.done"
    else
        touch "$run_dir/.failed"
    fi
}
trap finish EXIT
phase() {
    printf '%s\n' "$1" >"$run_dir/phase"
    printf '\n[%s] %s\n' "$(date -u +%FT%TZ)" "$1"
}
cd "$root"
date -u +%FT%TZ >"$run_dir/started-utc"
git rev-parse HEAD >"$run_dir/source-commit"
{ uname -a; lscpu; free -h; } >"$run_dir/machine.txt"

phase source
# Avoid checking out every LLVM project before narrowing to our source paths.
if [ ! -e third_party/llvm/.git ]; then
    tag=$(python3 -c 'import json; print(json.load(open("toolchain.json"))["upstream_tag"])')
    revision=$(python3 -c 'import json; print(json.load(open("toolchain.json"))["upstream_revision"])')
    mapfile -t paths < <(python3 -c 'import json; print("\n".join(json.load(open("toolchain.json"))["sparse_paths"]))')
    git clone --filter=blob:none --no-checkout --depth=1 --branch "$tag" \
        https://github.com/llvm/llvm-project.git third_party/llvm
    git -C third_party/llvm sparse-checkout set --cone "${paths[@]}"
    git -C third_party/llvm checkout --detach "$revision"
    git submodule init third_party/llvm
    git submodule absorbgitdirs third_party/llvm
fi
python3 scripts/llvm.py setup

phase compiler
export DC_BUILD_JOBS=${DC_BUILD_JOBS:-4}
./scripts/build-llvm.sh -DLLVM_USE_LINKER=lld \
    -DCLANG_ENABLE_STATIC_ANALYZER=OFF -DCLANG_ENABLE_ARCMT=OFF
# Match the host g++ installation rather than auto-selecting a newer, possibly
# incomplete GCC directory. Keep this configuration local to the built compiler.
gcc_dir=$(dirname "$(g++ -print-libgcc-file-name)")
printf '%s\n' "--gcc-install-dir=$gcc_dir" >build/llvm/bin/clang++.cfg
cp build/llvm/bin/clang++.cfg "$run_dir/clang++.cfg"

phase sanity
make test
make test-compiler
if [ "${DC_LTO:-0}" = 1 ]; then
    python3 tests/compiler.py --lto full
    python3 tests/compiler.py --lto thin
fi
make check-patches

phase benchmark-build
bench_dir=build/bench
bench_options=()
if [ "${DC_LTO:-0}" = 1 ]; then
    bench_dir=build/bench-lto
    bench_options=(-DDC_LTO=ON "-DCMAKE_CXX_COMPILER=$root/build/llvm/bin/clang++"
                   "-DCMAKE_AR=$root/build/llvm/bin/llvm-ar"
                   "-DCMAKE_RANLIB=$root/build/llvm/bin/llvm-ranlib")
fi
cmake -S . -B "$bench_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DDC_BUILD_BENCH=ON -DBUILD_TESTING=OFF "${bench_options[@]}"
cmake --build "$bench_dir" --parallel "$DC_BUILD_JOBS"
if [ "${DC_LTO:-0}" = 1 ]; then
    build/llvm/bin/opt -S "$bench_dir/bench/targets.o" -o "$run_dir/targets.ll"
else
    build/llvm/bin/llvm-objdump -d "$bench_dir/bench/targets.o" >"$run_dir/targets-disassembly.txt"
fi
build/llvm/bin/llvm-objdump -d \
    --disassemble-symbols=bench_controlled,bench_controlled.dc_guard,dc_acquire,dc_release \
    "$bench_dir/dc_bench" >"$run_dir/linked-disassembly.txt"
cp "$bench_dir/dc_bench" "$run_dir/dc_bench"
cp "$bench_dir/target-command.txt" "$bench_dir/link-options.txt" "$run_dir/"

phase benchmark
cpu=${DC_BENCH_CPU:-2}
printf '%s\n' "$cpu" >"$run_dir/benchmark-cpu"
taskset -c "$cpu" python3 bench/run.py --repetitions 7 --min-time 0.5s \
    --binary "$bench_dir/dc_bench" --output "$run_dir/results"
phase complete
