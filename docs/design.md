# Design sketch

## Agreed starting point

- Toggle future entries only. Racing entries may select either complete path.
- Start with a guard wrapping the shared body. Acquire a memory-semaphore
  permit before the body and release it afterward, including supported unwind
  paths. An admitted invocation owes release even if the function is disabled.
- Measure the wrapper before deciding whether body duplication is worthwhile.
- Keep Google Benchmark fixtures and their runner under `bench/`.
- Prepare only definitions annotated `[[clang::annotate("dc")]]`, behind one
  compiler switch. Keep `dc_find`, `dc_enable`, and `dc_disable` as the public
  controls; everything starts disabled.
- Initially measure three paths in one binary: an unannotated baseline target,
  and disabled/enabled calls to an identically implemented annotated target.
  This is an untouched-function baseline, not an independent stock-Clang build.
  Keep targets noinline and use an always-allow semaphore mock initially.

## Open questions

1. Does “any function” mean every eligible emitted function, or every source
   function including calls that would otherwise be inlined or eliminated?
2. Is a six-level limit a static call-graph selection policy or a runtime depth
   policy? Which roots count, including threads and callbacks?
3. What initial target architecture, OS, ABI, and function signatures should the
   demonstration cover? Unsupported cases must be reported explicitly.
4. Where must the compiler transform run so direct calls, recursion, function
   addresses, and LTO preserve dispatch without unnecessarily restricting body
   optimization?
5. Do nested guarded functions acquire independently or share an outer permit?
6. Should unavailable permits block or reject? Rejection requires an explicit
   application contract; an arbitrary function cannot invent a failure return.
7. Which nonlocal exits and cancellation mechanisms must release permits?

Start with an LLVM IR transformation if it can meet the selected contract.
Introduce frame-lowering changes only for a concrete requirement that the IR
approach cannot satisfy. Validate ABI forwarding and generated machine code;
LLVM's `musttail` has calling-convention, signature, and attribute constraints.

Reference: [LLVM call instruction](https://llvm.org/docs/LangRef.html#call-instruction).

## Initial implementation choices

The first pass runs before optimization, consumes `llvm.global.annotations`,
and preserves selected entry boundaries with noinline. It keeps the original
function symbol/address as the dispatcher; recursive calls and existing function
pointers therefore continue to enter it. The dispatcher performs an acquire
load followed by a musttail call. No frame-lowering changes are involved.

Each module emits a constant descriptor array `{name, body, guard, slot}` and a
writable registration node `{table, count, next}`. A constructor registers the
node. Each function's separate writable, pointer-aligned slot starts at its body.
The runtime publishes guard/body using release stores. The registry needs no
heap allocation and does not participate in the function-entry path.

The prototype targets ordinary fixed-arity C ABI functions with 64-bit pointers
and nounwind. Broader ABI coverage and hardening are deferred. Tables/nodes stay
loaded until exit. Exact linkage names are used; strong C linkage names keep
fixtures simple.
