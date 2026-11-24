#!/usr/bin/env python3
"""Print empty-hook timings from randomized blocks of the three call paths."""

import argparse
import json
from pathlib import Path
import random
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MODES = ("baseline", "unhooked", "hooked")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "build/bench/dc_bench")
    parser.add_argument("--repetitions", type=int, default=7)
    parser.add_argument("--min-time", default="0.2s")
    args = parser.parse_args()
    if args.repetitions < 1:
        parser.error("repetitions must be positive")
    samples = {mode: [] for mode in MODES}
    rng = random.Random(0)
    for _ in range(args.repetitions):
        modes = list(MODES)
        rng.shuffle(modes)
        for mode in modes:
            result = subprocess.run(
                [str(args.binary.resolve()), f"--dc-mode={mode}",
                 f"--benchmark_min_time={args.min_time}", "--benchmark_format=json"],
                capture_output=True, text=True, check=True)
            rows = [row for row in json.loads(result.stdout)["benchmarks"]
                    if row.get("run_type") == "iteration"]
            if len(rows) != 1 or rows[0].get("error_occurred") or rows[0]["time_unit"] != "ns":
                raise RuntimeError(f"Unexpected benchmark output: {result.stdout}")
            samples[mode].append(rows[0]["cpu_time"])
    for mode, values in samples.items():
        print(f"{mode}: {statistics.median(values):.3f} ns/call "
              f"(range {min(values):.3f}–{max(values):.3f})")


if __name__ == "__main__":
    main()
