#include "dynamic_control.h"

extern "C" __attribute__((noinline))
void dc_acquire(const DcFunction *) noexcept {}

extern "C" __attribute__((noinline))
void dc_release(const DcFunction *) noexcept {}
