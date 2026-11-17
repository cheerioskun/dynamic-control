#!/usr/bin/env python3
"""Exercise the actual patched compiler and runtime at O0 and O2."""

import argparse
import os
from pathlib import Path
import platform
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def run(args):
    subprocess.run([str(arg) for arg in args], check=True, cwd=ROOT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", type=Path, default=ROOT / "build/llvm/bin/clang++")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    if not args.clang.is_file():
        parser.error("patched clang++ not found; run make compiler first")
    output = ROOT / "build/tests"
    output.mkdir(parents=True, exist_ok=True)
    target_flags = []
    if platform.system() == "Darwin":
        sdk = subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
        target_flags = ["-isysroot", sdk]
    tools = args.clang.parent
    run([tools / "llvm-lit", "-v",
         ROOT / "third_party/llvm/llvm/test/Transforms/Instrumentation/dynamic-control.ll"])
    for level in (0, 2):
        for enabled in (False, True):
            label = f"O{level}-{'on' if enabled else 'off'}"
            objects = []
            for source in ("targets", "targets_extra"):
                obj = output / f"{source}-{label}.o"
                flags = ["-mllvm", "-dc-enable"] if enabled else []
                run([args.clang, *target_flags, "-std=c++17", f"-O{level}", "-fno-exceptions",
                     *flags, "-c", ROOT / f"tests/{source}.cc", "-o", obj])
                objects.append(obj)
            exe = output / f"smoke-{label}"
            run([args.cxx, "-std=c++17", "-O2", "-pthread", "-Iruntime",
                 ROOT / "runtime/dynamic_control.cc", ROOT / "tests/smoke.cc",
                 *objects, "-o", exe])
            run([exe] + ([] if enabled else ["--expect-unprepared"]))

    for suffix, flags in (("ll", ["-S", "-emit-llvm"]), ("s", ["-S"])):
        run([args.clang, *target_flags, "-std=c++17", "-O2", "-fno-exceptions", "-mllvm",
             "-dc-enable", *flags, ROOT / "tests/targets.cc", "-o",
             output / f"targets-O2-on.{suffix}"])

    print("compiler: prototype sanity checks passed")


if __name__ == "__main__":
    main()
