#!/usr/bin/env python3
"""Fetch the selected LLVM release and apply our patches once."""

import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "third_party/llvm"


def git(*args, capture=False):
    return subprocess.run(["git", "-C", str(SOURCE), *args], check=True,
                          capture_output=capture, text=True)


def main():
    info = json.loads((ROOT / "toolchain.json").read_text())
    tag = info["upstream_tag"]
    if not (SOURCE / ".git").exists():
        subprocess.run(["git", "clone", "--depth=1", "--filter=blob:none",
                        "--no-checkout", "--branch", tag,
                        "https://github.com/llvm/llvm-project.git", str(SOURCE)], check=True)
        git("sparse-checkout", "set", "--cone", *info["sparse_paths"])
        git("checkout", "--detach", tag)
    # Local commits are the developer's working compiler; leave them alone.
    if int(git("rev-list", "--count", f"{tag}..HEAD", capture=True).stdout):
        print("Using the existing LLVM development checkout.")
        return
    if git("status", "--porcelain", capture=True).stdout:
        raise SystemExit("LLVM has local edits; leave them intact and apply patches manually.")
    patches = ROOT / "patches/llvm"
    names = (patches / "series").read_text().splitlines()
    if names:
        git("-c", "user.name=dynamic-control", "-c", "user.email=build@localhost",
            "am", "--whitespace=nowarn", *[str(patches / name) for name in names])


if __name__ == "__main__":
    main()
