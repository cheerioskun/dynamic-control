# Design

## Selection

`[[clang::annotate("dc")]]` marks a function for preparation.
`-mllvm -dc-enable` runs the pass before optimization. The pass preserves the
selected entry boundary with noinline. Unannotated functions have no dispatch
slot or redirection.

`dc_find`, `dc_enable`, and `dc_disable` are the runtime controls. Functions start
disabled. Names are exact linkage names; fixtures use C linkage.

## Dispatch and permits

The original symbol remains the dispatcher, so direct calls, function pointers,
and recursion enter the same slot. An acquire load selects the body or guard,
followed by a musttail call. No backend or frame-lowering patch is involved.

The guard calls acquire, the shared body, and release. Disabling during a guarded
invocation does not cancel its release. Entries racing with a toggle can select
either path.

The current mock always permits entry. Real permit failure, nesting, and unwind
handling are separate follow-up work. Body duplication is a later experiment if
the shared-body wrapper costs enough to matter.

## Address table

Each module emits:

- A constant `DcFunction` array: `{name, body, guard, slot}`.
- One writable, pointer-aligned dispatch slot per function.
- A writable `DcModule` registration node: `{table, count, next}`.
- A constructor that registers the module's node.

Registration links module tables without heap allocation. The registry is used
for control operations; each function entry loads its own slot directly. Enable
and disable use release stores. Tables and nodes remain loaded until exit.

The compiler/runtime ABI is defined in `runtime/dynamic_control.h` and checked
by static assertions and the execution fixtures.

## Benchmark

One binary contains two identical noinline targets: one plain and one annotated.
The three measurements are plain, annotated/disabled, and annotated/enabled.
Lookup and toggles happen before timing. The benchmark links the always-allow
mock; correctness fixtures substitute counting hooks.

See [commands](../bench/README.md) and [results](../bench/RESULTS.md).

## Later

NOP-padded entries and runtime code patching will address broad function coverage
without permanent redirection at every eligible entry. Broader ABI/LTO coverage
and the real semaphore's permit policy follow the annotated prototype.
