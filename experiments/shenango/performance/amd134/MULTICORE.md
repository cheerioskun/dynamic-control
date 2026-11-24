# Multicore blocking-control parity

Both variants passed multicore correctness checks and ran with 2, 4, 8, and
16 Shenango scheduler threads. Their median paired RPC throughput differences
were all within 0.2%. Individual pairs varied substantially more, so these
results do not establish a precise overhead bound or a speedup.

There is **no permit TLS** in either variant. Both use the existing
`MemSemaphore::Wait()` and `Post()`. The compiler registry, function-selection
mechanism, adaptive permit controller, and AMD counter backend are unchanged.
Only the correctness build uses global atomic acquisition/release counters;
those counters are absent from timed production binaries.

## Counts and setup

Machine: EPYC 7302P, 16 physical cores / 32 hardware threads; existing no-PCI
iokernel and patched Clang 23.1.6. Counts below mean Shenango scheduler threads,
not distinct physical cores. A live `ps -eLo pid,tid,psr,pcpu,comm,args` inspection
of an 8-thread worker run showed CPUs 1,17,2,3,18,19,4,20: the SMT siblings of
four physical cores. The 16-thread point extends the SMT-inclusive sweep.

Each generated config sets maximum, guaranteed, and spinning scheduler threads
to the requested count. Every timed worker run observed every configured
scheduler index (`scheduler_mask` in the raw samples). This proves work ran
across those scheduler threads; it does not imply all of them held permits
simultaneously. Semaphore capacity remains adaptive, bounded by that count.

Both variants use the same 32 KiB netbench memory worker and 25 work iterations.
Each point has five sequential pairs, alternating hardcoded/auto order.
No other benchmark was deliberately run alongside them. All sources and binary
hashes are tied to source commit `37f0de4` in the saved manifests.

## Correctness

At each count, both variants ran 32 user threads making 640 total calls.
The test body yields while holding its permit, exercising parking and
rescheduling with contending waiters. Every case completed with argument sum
10560, 640 acquisitions, 640 releases, no active permits, and a drained waiter
queue. The body checked that the global active count was positive and no larger
than the configured maximum. It does not use OS-thread-local permit state or
claim to verify ownership through such state.

## Concurrent worker throughput

The harness launches four user threads per configured scheduler thread. It
warms up for one second, then times two seconds through the actual netbench
memory-work boundary, including real semaphore acquisition/release. Calls
already waiting when the deadline expires complete and are included in both
the call count and elapsed time. Setup and allocations are outside timing.

| Scheduler threads | Hardcoded calls/s | Auto calls/s | Median paired difference | Paired range |
|---:|---:|---:|---:|---:|
| 2 | 22,306 | 22,298 | -0.017% | -0.067% to +0.013% |
| 4 | 23,807 | 23,917 | +0.464% | -0.932% to +1.225% |
| 8 | 48,428 | 48,384 | -1.026% | -4.618% to +1.054% |
| 16 | 70,540 | 72,297 | +1.358% | -0.850% to +5.423% |

## Netbench RPC throughput

The server binary and memory-only request workload are unchanged. Four Linux
client **processes**, each with one connection and batches of 16 requests,
replace the earlier threaded Python client to avoid a shared interpreter lock.
Each response's request fields are checked; a malformed or dropped response
fails the run. Timing follows ten warmup batches per client and lasts two
seconds plus completion of the final batches. These are closed-loop results.

| Scheduler threads | Hardcoded requests/s | Auto requests/s | Median paired difference | Paired range |
|---:|---:|---:|---:|---:|
| 2 | 22,237 | 22,247 | -0.022% | -0.792% to +0.243% |
| 4 | 23,189 | 23,154 | -0.162% | -0.480% to +1.343% |
| 8 | 46,543 | 46,174 | -0.097% | -2.855% to +1.238% |
| 16 | 57,531 | 57,580 | +0.152% | -6.807% to +0.587% |

Client CPU time was recorded. The busiest case used less than 0.87 CPU-seconds
per elapsed second summed across all four client processes; they did not
saturate their four process execution slots. This does not rule out other
client or networking limits. The path still includes kernel TCP, dtap, and
the no-PCI iokernel rather than the paper's physical NIC testbed.

All throughput columns are independent medians. Paired differences are
computed within a repetition before taking the median, so their sign need
not match the ratio of the independent medians. Ranges are observed sample
ranges, not confidence intervals. In particular, the 16-thread RPC comparison
contains a -6.8% pair despite its near-zero median difference.

The results support behavioral parity and show no consistent throughput penalty
across these tested configurations. They do not isolate the compiler guard's
cycle cost: the controller may choose different capacity histories as timings
and measured bandwidth change. Capacity trajectories were not recorded. Slow
scaling at the smaller counts is an observation, not a diagnosis of its cause.
No request latency percentiles or open-loop overload curves were measured.
The earlier single-core RPC result used a different client implementation and
should not be treated as a directly comparable scaling baseline.

## Artifacts and reproduction

`multicore/` contains the 2/4/8-thread runs; `multicore-16/` contains the
16-thread extension. Each includes correctness logs, configs, raw worker and
RPC samples, summaries, and manifests with source/binary SHA-256 hashes.

From `breakwater/apps/netbench/src` on the prepared remote working copy:

```sh
make -f Makefile.parity MODE=hardcoded scaling
make -f Makefile.parity MODE=auto scaling
make -f Makefile.parity MODE=hardcoded VERIFY=1 selftest
make -f Makefile.parity MODE=auto VERIFY=1 selftest
python3 bench_multicore.py --source-commit LOCAL_SOURCE_COMMIT --cores 2 4 8 16 --output /tmp/multicore-results
```

Both production netbench binaries must already be built (`parity` target).
Use the actual local source commit rsynced to the remote copy. Output paths
must be new. Run benchmarks sequentially to avoid interference.
