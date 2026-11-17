#ifndef DYNAMIC_CONTROL_H
#define DYNAMIC_CONTROL_H

#include <stddef.h>

#ifdef __cplusplus
#define DC_NOEXCEPT noexcept
extern "C" {
#else
#define DC_NOEXCEPT
#endif

// Erased addresses are stored, never called through this type. Dispatchers
// restore the original signature. Slot accesses must always be atomic.
typedef void (*DcEntry)(void);

// Compiler ABI: four pointer-sized fields, in this order. Descriptors are
// immutable; each target points to a separate, pointer-aligned writable slot.
typedef struct DcFunction {
  const char *name;
  DcEntry body;
  DcEntry guard;
  DcEntry *target;
} DcFunction;

// One writable registration node per module. The compiler's constructor calls
// __dc_register_module exactly once. Nodes and descriptors live until exit;
// unloading a registered shared library is not supported in this first slice.
typedef struct DcModule {
  const DcFunction *functions;
  size_t count;
  struct DcModule *next;
} DcModule;

void __dc_register_module(DcModule *module) DC_NOEXCEPT;

// Names are exact LLVM/linkage names. Missing or ambiguous names return NULL.
// Call after module constructors have run. Handles remain valid until exit.
const DcFunction *dc_find(const char *name) DC_NOEXCEPT;
// Require a non-null handle returned by dc_find. Future entries select the
// published destination; calls racing with a toggle can take either path.
void dc_enable(const DcFunction *function) DC_NOEXCEPT;
void dc_disable(const DcFunction *function) DC_NOEXCEPT;

// The initial implementation always allows entry. Both calls are nonthrowing.
// Rejected acquisition and nested-permit policies are intentionally unspecified.
void dc_acquire(const DcFunction *function) DC_NOEXCEPT;
void dc_release(const DcFunction *function) DC_NOEXCEPT;

#ifdef __cplusplus
}
#endif
#undef DC_NOEXCEPT
#endif
