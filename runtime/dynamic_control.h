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

// One registration node per module, linked by its constructor before main.
// Nodes and descriptors live until exit.
typedef struct DcModule {
  const DcFunction *functions;
  size_t count;
  struct DcModule *next;
} DcModule;

void __dc_register_module(DcModule *module) DC_NOEXCEPT;

// Exact, unique LLVM/linkage names. Missing names return NULL.
// Call after startup registration. Handles remain valid until exit.
const DcFunction *dc_find(const char *name) DC_NOEXCEPT;
// Require a non-null handle returned by dc_find. Future entries select the
// published destination; calls racing with a toggle can take either path.
void dc_enable(const DcFunction *function) DC_NOEXCEPT;
void dc_disable(const DcFunction *function) DC_NOEXCEPT;

// The synthetic Svalinn mock always allows entry. Both calls are nonthrowing.
void dc_acquire(const DcFunction *function) DC_NOEXCEPT;
void dc_release(const DcFunction *function) DC_NOEXCEPT;

#ifdef __cplusplus
}
#endif
#undef DC_NOEXCEPT
#endif
