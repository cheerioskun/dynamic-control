# Blocking netbench performance on amd134

This is the original single-core run. Subsequent multicore measurements are in
`MULTICORE.md`; its RPC runner uses separate client processes. Reproducing the
historical RPC method below requires its recorded source commit `fc08d6a`.

The normal 25-iteration memory worker has essentially the same execution time
with hardcoded and generated control. The RPC test measured slightly lower
median throughput for auto, with paired differences crossing zero. These runs
support a small application-level cost in this configuration, not an exact
overhead bound or a claim of equal performance under contention.

Machine: AMD EPYC 7302P, Linux 6.8.0-149-generic, patched Clang 23.1.6.
Each application has one Shenango scheduler thread and semaphore capacity one.
The real epsilon-greedy semaphore, AMD counters, and original 32 KiB
`MemBWAntagonistWorker` are used without controller/counter changes.
Verification counters are compiled out (`VERIFY=0`). No frequency settings were
changed; the host exposes no cpufreq governor/boost files at the checked paths.

## Worker execution time

Nine paired runs, alternating hardcoded/auto order and enabled/disabled order.
Each process warms up and calibrates outside timing, then measures about 300 ms
per work size. The loop calls the same boundary as netbench, inside Shenango,
without RPC/network/client costs. Enabled acquisitions are uncontended here.

| Control | Work iterations | Hardcoded median | Auto median | Median paired time difference | Paired range |
|---|---:|---:|---:|---:|---:|
| Enabled | 1 | 1.886 us | 1.898 us | +0.417% | -1.037% to +1.172% |
| Enabled | 25 | 44.443 us | 44.458 us | -0.001% | -0.108% to +0.257% |
| Disabled | 1 | 1.732 us | 1.732 us | +0.014% | -0.090% to +0.136% |
| Disabled | 25 | 43.308 us | 43.296 us | -0.025% | -0.109% to +0.073% |

Positive time differences mean auto is slower. Paired differences are computed
within each repetition before taking their median; this need not equal the
ratio of the two independently computed median times. The ranges are observed
sample ranges, not confidence intervals. The small-work enabled case shows a
positive median difference, but its sign also varies across pairs.

Measured source commit: `a05225f`. See `worker/samples.json`, `summary.json`,
per-process logs, and `manifest.json` for compiler/machine information and
source, binary, build-configuration, and dependency-archive SHA-256 hashes.

## Netbench RPC throughput

Seven paired runs with alternating server order. Each server is the production
netbench binary, `nocontrol ... server msem`, with normal 25-iteration memory
requests. Four Linux-socket clients each keep a batch of 16 requests in flight.
After warmup, each case is timed for two seconds plus completion of its last
batch. Every response is checked for correct echoed request fields; a dropped
or malformed response fails the run.

| Hardcoded median | Auto median | Median paired throughput difference | Paired range |
|---:|---:|---:|---:|
| 22,231 requests/s | 22,073 requests/s | -0.871% | -1.539% to +1.788% |

Negative throughput differences mean auto is slower. This includes Python,
kernel TCP, dtap, and the existing no-PCI iokernel. It is a local, closed-loop,
single-core result, not the paper's NIC-backed testbed. Neither this test nor
the worker timing establishes multi-core scaling or the cost of parking and
waking contended semaphore waiters. No latency percentiles were measured.

Runner source commit: `fc08d6a`. The server code is unchanged from checkpoint
`7fd7876`; server binary hashes are recorded in `rpc/manifest.json`.
Raw throughput samples and server logs are in `rpc/`.

## Reproduce

From `breakwater/apps/netbench/src` on the prepared Linux machine:

```sh
make -f Makefile.parity MODE=hardcoded bench parity
make -f Makefile.parity MODE=auto bench parity
python3 bench_parity.py --source-commit LOCAL_SOURCE_COMMIT --output /tmp/worker-perf
python3 bench_netbench_rpc.py --source-commit LOCAL_SOURCE_COMMIT --output /tmp/rpc-perf
```

Pass the local source commit that was rsynced, not the old copied remote Git
metadata. Each output directory must be new. Run the two benchmarks separately
so they do not interfere with each other.
