# Netbench on Shenango

We wanted to know whether our LLVM extension could replace hardcoded
instrumentation in a real application:

1. **Correctness:** does generated instrumentation behave like an explicit
   `Wait() → work → Post()` wrapper?
2. **Performance:** does it perform similarly to that wrapper?

We used Svalinn's Shenango netbench, keeping the workload and memory semaphore
the same in both versions. The hardcoded version acquires and releases around
the memory work. The generated version annotates that work and lets our pass
insert the guard; its hooks call the same semaphore's `Wait()` and `Post()`.

We deliberately used blocking `Wait()`, replacing netbench's `TryWait()` and
congestion-based rejection at this boundary. Neither version uses permit TLS:
an OS thread's TLS would not follow a migrating Shenango user thread.

## What we ran

The machine was `hemantp@amd134.utah.cloudlab.us`: an AMD EPYC 7302P with
16 physical cores and 32 hardware threads, running Linux 6.8 and our patched
Clang 23.1.6. The working application was under
`/users/hemantp/svalinn-shenango-dc`; the compiler was in the sibling
`dynamic-control` checkout.

We reused the running Shenango iokernel in `/local/shenango`. It ran in
no-PCI mode, so RPC traffic went through kernel TCP and dtap rather than a
physical NIC. These are local application comparisons, not paper-scale runs.

The memory worker uses a 32 KiB buffer and normally performs 25 iterations.
We timed it directly inside Shenango and through netbench RPCs. The real
adaptive semaphore and its counters stayed in place in both versions.

The main application changes are in [netbench.patch](netbench.patch), applied
on top of the upstream submodule in `third_party/svalinn-shenango`. Besides the
blocking wrapper, we selected the per-core worker after acquisition, since
waiting can move a user thread. We removed unsupported AVX-512 and user-interrupt
build flags for this AMD machine. The application uses the parent's dispatch
runtime and supplies its own semaphore hooks.

The AMD uncore driver needed loading to expose the memory counters. We also
noticed that the backend reads multiplexed counters without scaling them;
we left that unchanged because this comparison was about instrumentation parity.

## What happened

The correctness checks passed for both versions: 32 user threads completed
640 calls with the same argument totals, 640 acquisitions, 640 releases,
and no permits left active. We exercised contention and checked that disabling
instrumentation during a call still lets that call release its permit.
Disabled instrumentation bypassed the hooks and still executed the work.
Both servers also returned all 1,024 responses in the initial RPC check.

On one scheduler thread, the normal worker took a median **44.443 us** with
the hardcoded wrapper and **44.458 us** with generated instrumentation.
Nine paired runs put their differences between roughly -0.11% and +0.26%.
The smaller one-iteration worker measured 1.886 us versus 1.898 us.

We then used 2, 4, 8, and 16 Shenango scheduler threads. These counts include
SMT siblings: the observed 8-thread placement used four physical cores.
Correctness passed at every count, and all configured scheduler threads were
observed executing work.

Median RPC throughput, with the semaphore enabled:

| Scheduler threads | Hardcoded | Generated |
|---:|---:|---:|
| 2 | 22,237 requests/s | 22,247 requests/s |
| 4 | 23,189 requests/s | 23,154 requests/s |
| 8 | 46,543 requests/s | 46,174 requests/s |
| 16 | 57,531 requests/s | 57,580 requests/s |

These points each used five paired runs, alternating which version ran first.
Four separate Linux client processes each kept batches of 16 requests in flight.
We switched from Python threads to processes to avoid a shared interpreter lock.

The concurrent worker-only runs were also close: at 16 scheduler threads,
hardcoded achieved about 70,540 calls/s and generated about 72,297 calls/s.

## What we take from it

The extension matched the hardcoded wrapper in the behavior we exercised,
and we saw similar performance. Multicore median paired RPC differences were
within 0.2%, but individual pairs varied by as much as 6.8%; the independent
medians in the table should not be read as a precise overhead measurement.
The slightly faster worker result for generated code is not evidence of a speedup.

This checked the instrumentation mechanism. It did not establish that Svalinn's
control policy benefits this workload. Capacity was adaptive, and we did not
record its trajectory, so the scaling curve is not a diagnosis of the controller.
We also did not measure request latency or open-loop overload behavior.
