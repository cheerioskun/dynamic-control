# Initial design questions

These are unresolved choices, not an implementation contract.

1. Does enabling instrumentation affect future entries only, or must it also
   affect already-running invocations?
2. Does `foo_guard` wrap `foo_body` with entry/exit hooks, or select a separately
   instrumented implementation? What information should the instrumentation see?
3. Does “any function” mean every eligible emitted function, or every source
   function including calls that would otherwise be inlined or eliminated?
4. Is a six-level limit a static call-graph selection policy or a runtime depth
   policy? Which roots count, including threads and callbacks?
5. What initial target architecture, OS, ABI, and function signatures should the
   demonstration cover? Unsupported cases must be reported explicitly.
6. Where must the compiler transform run so direct calls, recursion, function
   addresses, and LTO preserve dispatch without unnecessarily restricting body
   optimization?

Start with an LLVM IR transformation if it can meet the selected contract.
Introduce frame-lowering changes only for a concrete requirement that the IR
approach cannot satisfy. Validate ABI forwarding and generated machine code;
LLVM's `musttail` has calling-convention, signature, and attribute constraints.

Reference: [LLVM call instruction](https://llvm.org/docs/LangRef.html#call-instruction).
