#!/usr/bin/env python3
"""Run the three paths in randomized blocks, saving every raw measurement."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MODES = ("baseline", "unhooked", "hooked")


def output(*args):
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "build/bench/dc_bench")
    parser.add_argument("--repetitions", type=int, default=7)
    parser.add_argument("--min-time", default="0.2s")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    if not args.binary.is_file() or args.repetitions < 1:
        parser.error("build the benchmark first and use positive repetitions")
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    destination = (args.output or ROOT / "build/bench/results" / timestamp).resolve()
    destination.mkdir(parents=True, exist_ok=False)
    rng = random.Random(args.seed)
    commands = []
    for repetition in range(args.repetitions):
        modes = list(MODES)
        rng.shuffle(modes)
        for mode in modes:
            raw = destination / f"{repetition:02d}-{mode}.json"
            commands.append([str(args.binary), f"--dc-mode={mode}",
                             f"--benchmark_min_time={args.min_time}",
                             "--benchmark_color=false", "--benchmark_repetitions=1",
                             "--benchmark_out_format=json", f"--benchmark_out={raw}"])
    sources = [p for folder in ("runtime", "bench") for p in (ROOT / folder).glob("*")
               if p.is_file()]
    sources += [ROOT / "CMakeLists.txt", ROOT / "toolchain.json"]
    sources += list((ROOT / "patches/llvm").glob("*"))
    command_file = args.binary.parent / "target-command.txt"
    manifest = {
        "started_utc": timestamp, "seed": args.seed, "commands": commands,
        "system": platform.system(), "release": platform.release(),
        "machine": platform.machine(), "logical_cpus": os.cpu_count(),
        "cpu_affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
        "parent_commit": output("git", "rev-parse", "HEAD"),
        "dirty": bool(output("git", "status", "--porcelain", "--ignore-submodules")),
        "llvm_commit": output("git", "-C", "third_party/llvm", "rev-parse", "HEAD"),
        "llvm_tree": output("git", "-C", "third_party/llvm", "rev-parse", "HEAD^{tree}"),
        "binary_sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        "target_command": command_file.read_text().splitlines(),
        "link_options": (args.binary.parent / "link-options.txt").read_text().splitlines(),
        "source_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sources},
    }
    (destination / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    samples = {mode: [] for mode in MODES}
    for command in commands:
        mode = command[1].split("=", 1)[1]
        raw = Path(command[-1].split("=", 1)[1])
        with raw.with_suffix(".log").open("w") as log:
            subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                           check=True)
        data = json.loads(raw.read_text())
        rows = [row for row in data["benchmarks"] if row.get("run_type") == "iteration"]
        if len(rows) != 1 or rows[0].get("error_occurred") or rows[0]["time_unit"] != "ns":
            raise RuntimeError(f"Unexpected benchmark result: {raw}")
        row = rows[0]
        samples[mode].append({key: row[key] for key in ("cpu_time", "real_time", "iterations")})
        print(f"{raw.stem}: {row['cpu_time']:.3f} ns/call (CPU)", flush=True)
    medians = {mode: statistics.median(s["cpu_time"] for s in values)
               for mode, values in samples.items()}
    summary = {
        "unit": "ns/call", "cpu_medians": medians, "samples": samples,
        "unhooked_minus_baseline": medians["unhooked"] - medians["baseline"],
        "hooked_minus_unhooked": medians["hooked"] - medians["unhooked"],
    }
    (destination / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary["cpu_medians"], indent=2))
    print(f"Results: {destination}")


if __name__ == "__main__":
    main()
