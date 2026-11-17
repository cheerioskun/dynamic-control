# Synthetic memory-semaphore fixture

`msem_mock.cc` adapts the optimized controller in
[`svalinn-shenango/m-semaphore/bench/msem_bench.cc`](https://github.com/GT-ANSR-Lab/svalinn-shenango/blob/474e5d13c9dee4bdff2bdb7dc03c3d3dca90a0c9/m-semaphore/bench/msem_bench.cc),
using the portable adaptation from
[`mahoraga` exp-15](https://github.com/cheerioskun/mahoraga/blob/eb8efe4af269516570c082ade4368e66739444f9/experiments/015-hook-benchmarks/target-msem-noshenango.cc).
The imported code retains [Svalinn's Apache 2.0 license](LICENSE.svalinn-shenango).

It retains synthetic bandwidth of 175000, 45 capacity choices, a ten-sample
reward window, softmax exploration, and Thompson sampling from 2^18
pre-generated Gaussian values. Posterior parameters stay fixed, as in the
optimized benchmark. Seeds are zero. Each acquire and release checks the real
steady clock; controller work runs at most once every 3000 microseconds.

The benchmark calls `dc_msem_mock_init()` before timing. The hooks share one
state record and always grant entry, without permit accounting or locking.
This is the single-threaded portable fixture, with no PCM or Shenango dependency.
The controller's learned capacity is calculated but does not gate entry.

The adaptation replaces exp-15's external handle and wrappers with the
`dc_acquire`/`dc_release` ABI. It does not change the compiler pass or descriptor
layout. The earlier empty-hook measurements remain in `bench/RESULTS.md` and
their saved JSON files.
