#!/usr/bin/env python3
"""Prepare LLVM and export its local commits as parent patches."""
from dependency import main

if __name__ == "__main__":
    raise SystemExit(main("llvm"))
