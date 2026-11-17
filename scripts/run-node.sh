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

phase sanity
make test
make test-compiler
make check-patches

phase benchmark-build
cmake -S . -B build/bench -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DDC_BUILD_BENCH=ON -DBUILD_TESTING=OFF
cmake --build build/bench --parallel "$DC_BUILD_JOBS"
build/llvm/bin/llvm-objdump -d build/bench/bench/targets.o >"$run_dir/targets-disassembly.txt"
build/llvm/bin/llvm-objdump -d \
    --disassemble-symbols=bench_controlled,bench_controlled.dc_guard,dc_acquire,dc_release \
    build/bench/dc_bench >"$run_dir/linked-disassembly.txt"

phase benchmark
cpu=${DC_BENCH_CPU:-2}
printf '%s\n' "$cpu" >"$run_dir/benchmark-cpu"
taskset -c "$cpu" python3 bench/run.py --repetitions 7 --min-time 0.5s \
    --output "$run_dir/results"
phase complete
