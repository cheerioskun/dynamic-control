# Prototype checkpoint

The prototype prepares annotated functions and toggles hooks for future entries.
It is an LLVM IR pass, a small runtime, and a three-path Google Benchmark fixture.
LLVM is 23.1.6. There are no frame-lowering changes.

The application comparison is recorded in
[results/netbench-shenango](results/netbench-shenango/README.md): correctness
parity with hardcoded `Wait()/work/Post()` and similar observed performance,
using the real semaphore without permit TLS.

## Read in this order

| File | What to look for |
|---|---|
| [bench/targets.cc](bench/targets.cc) | Identical plain and annotated arithmetic functions |
| [DynamicControl.cpp](third_party/llvm/llvm/lib/Transforms/Instrumentation/DynamicControl.cpp) | Annotation selection, body cloning, guard, dispatcher, address table |
| [runtime/dynamic_control.h](runtime/dynamic_control.h) and [.cc](runtime/dynamic_control.cc) | Two records and registration/find/enable/disable |
| [bench/bench.cc](bench/bench.cc) | Same timed loop, three modes, setup outside timing |
| [runtime/msem_mock.cc](runtime/msem_mock.cc) | Copied synthetic controller; [provenance and behavior](runtime/msem_mock.md) |

The compiler source appears after `make setup`. Its published form is the
patch series in [patches/llvm/series](patches/llvm/series).

## Design choices

Selection has two knobs: `[[clang::annotate("dc")]]` on a definition and
`-mllvm -dc-enable` when compiling it. Unannotated functions have no slot or
redirection. C linkage gives the fixtures simple lookup names.

The annotation is a hint. The pass runs late in each file's compilation, after
ordinary optimization and before LTO. It prepares remaining eligible definitions;
it does not preserve calls, prevent earlier inlining, or hook inlined copies.
Full LTO and ThinLTO then proceed normally; no link-time enable flag is needed.

For each selected `foo`, the pass emits:

```text
foo:          acquire-load foo.dc_target; musttail call destination
foo.dc_body:  original implementation
foo.dc_guard: dc_acquire(descriptor); call body; dc_release(descriptor); return
foo.dc_target initially points to body
```

The public entry stays `foo`, including for function pointers and surviving
recursive calls. The benchmark explicitly uses `noinline` to keep its measurement
boundary. Prepared dispatchers, guards, and bodies are also `noinline`.
`musttail` removes the dispatcher's frame.
The guard shares the body, so it pays an extra call instead of duplicating code.
Wrappers drop the original function's purity/side-effect attributes: loading a
mutable destination and calling hooks must remain visible to optimization.

Each module has a constant table of `{name, body, guard, slot}`, a registration
node, and a constructor. Constructors prepend nodes to one linked list before
`main`. No linker-section convention or heap allocation is needed. `dc_find`
walks the tables outside timing; function entry loads its slot directly.

`dc_enable` release-stores the guard; `dc_disable` release-stores the body.
Entries racing with a toggle can take either path. An invocation already in a
guard still executes its release, even if its body disables the function.

Happy-path assumptions: fixed-arity nonthrowing functions, 64-bit pointers,
unique linkage names, valid handles, startup registration, and modules
remaining loaded. Broad selection, binary patching, and real permit policy are
outside this checkpoint.

The Svalinn mock checks the steady clock on both hooks and runs its controller
every 3 ms using synthetic bandwidth. Initialization is outside timing. It
always grants entry, with no permit accounting or locks. The compiler fixtures
replace it with counting hooks to verify balanced acquire/release calls.

## Verify

```sh
make test                 # runtime and handwritten dispatcher
make test-compiler        # actual compiler: IR, O0/O2, flag off/on, execution
python3 tests/compiler.py --lto full  # same fixture, full LTO
python3 tests/compiler.py --lto thin  # same fixture, ThinLTO
make check-patches       # patches reproduce the compiler source tree
make bench               # build and measure all three paths
```

`tests/smoke.cc` checks direct/indirect calls, recursion, two module tables,
argument/return forwarding, toggling, and release after disable. These checks
and inspection of the final assembly establish that dispatch and hooks survive
optimization. Patch tooling follows LDB; [development commands](docs/development.md)
cover it separately from the runtime.

## Saved artifacts

[bench/RESULTS.md](bench/RESULTS.md) contains both empty-hook and synthetic-msem
measurements on M5 and Xeon, with samples and source/binary hashes in
`bench/results/*.json`. Each reported result names the source commit measured;
the cleanup checkpoint does not relabel or replace those measurements.

On `hemantp@pc475.emulab.net`, under `/users/hemantp/dynamic-control/`:

| Directory | Contents |
|---|---|
| `build/node-runs/20261117T190149Z/` | Empty-hook binary, logs, raw JSON, manifest, disassembly, `.done` |
| `build/node-runs/20261117T214122Z-msem/` | Synthetic-msem binary and the same run artifacts |
| `build/node-runs/20261118T031720Z-lto/` | Full-LTO synthetic-msem binary, logs, samples, disassembly, `.done` |

Logs, JSON, and disassembly are also local under `build/node-runs/`. The full-LTO
x86 binary is copied locally too; earlier frozen x86 binaries remain on the node. Mac runs are under `build/bench/results/`; their
exact directories are in the results page.
Build output is ignored by Git. Download the frozen synthetic-msem x86 binary:

```sh
scp hemantp@pc475.emulab.net:/users/hemantp/dynamic-control/build/node-runs/20261117T214122Z-msem/dc_bench ./dc_bench-x86_64
```

Download the frozen full-LTO x86 binary:

```sh
scp hemantp@pc475.emulab.net:/users/hemantp/dynamic-control/build/node-runs/20261118T031720Z-lto/dc_bench ./dc_bench-lto-x86_64
```
