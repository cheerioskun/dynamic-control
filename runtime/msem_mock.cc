#include "dynamic_control.h"

// Out-of-line calls with a compiler barrier: no permit accounting, counters,
// controller, or contention. These measure plumbing, not a real semaphore.
extern "C" __attribute__((noinline))
void dc_acquire(const DcFunction *) noexcept {
  asm volatile("" ::: "memory");
}

extern "C" __attribute__((noinline))
void dc_release(const DcFunction *) noexcept {
  asm volatile("" ::: "memory");
}
