# Hook overhead

We wanted to measure what the extension adds to a function call before putting
any useful work in its acquire/release hooks. The hooks here are empty.

The microbenchmark calls the same tiny arithmetic function through three paths:

- **Baseline:** an ordinary, unannotated function.
- **Unhooked:** prepared by the pass, with its target pointing to the original body.
- **Hooked:** target pointing to the guard, which calls acquire, body, and release.

Both functions are noinline and calculate `x * 3 + 1`. Lookup and toggling happen
before timing. The loop feeds each result into the next call, so the reported
time includes that loop and dependency. Code lives in `bench/`.

We ran this on the local Apple M5 Mac and on
`hemantp@pc475.emulab.net`, an Intel Xeon E5530 at 2.40 GHz, in November 2026.
The remote checkout was `/users/hemantp/dynamic-control`.

Our patched LLVM 23.1.6 compiled the targets at O2 without LTO. The harness
used the host compiler: Apple Clang on the Mac and GCC 13.3 on the Xeon.
We used Google Benchmark, seven randomized blocks, and a fresh process for each
case. Samples ran for at least 0.2 seconds on the Mac and 0.5 seconds on the
Xeon. The Xeon runs were pinned to CPU 2; scaling and ASLR stayed enabled.

Median CPU time per call:

| Machine | Baseline | Unhooked | Hooked |
|---|---:|---:|---:|
| Apple M5 | 1.369 ns | 1.229 ns | 2.075 ns |
| Xeon E5530 | 2.639 ns | 3.018 ns | 8.298 ns |

On the Xeon, preparing the boundary added about **0.38 ns/call**, and enabling
the guard added another **5.28 ns/call**. The samples were tightly grouped
within that run.

On the M5, hooked minus unhooked was about **0.85 ns/call**, but unhooked samples
varied from 0.845 to 1.574 ns. Its apparently faster unhooked median does not
show a speedup; the dispatch penalty was unresolved in those measurements.

We inspected the linked assembly: the dispatcher retained its slot load and
indirect branch, the guard retained acquire/body/release calls, and both timed
loops called their intended targets. Baseline and body did identical arithmetic.

These are small-function measurements of dispatch and empty hooks, not the
cost of a semaphore or evidence of application benefit. With full LTO, empty
hook calls may disappear; the numbers above are non-LTO. For behavior and
performance with a real semaphore, see [netbench on Shenango](../netbench-shenango/README.md).

On November 24 we checked the Docker workflow on `hp097.utah.cloudlab.us`
(xl170, Linux amd64, 20 build threads) and the M5's Docker Desktop Linux arm64
VM (four build threads). Both images passed runtime/compiler checks and ran
`make bench` with networking disabled, without rebuilding LLVM.
The sanity-run medians were:

| Docker host | Baseline | Unhooked | Hooked |
|---|---:|---:|---:|
| CloudLab xl170 | 1.483 ns | 1.785 ns | 5.041 ns |
| M5 Linux VM | 0.702 ns | 0.701 ns | 1.875 ns |

These check that the packaged benchmark works; they are not a controlled
comparison with the native runs above.
