#include "dynamic_control.h"

#include <cstring>
#include <type_traits>

static_assert(std::is_standard_layout_v<DcFunction>);
static_assert(sizeof(DcEntry) == sizeof(void *));
static_assert(sizeof(DcFunction) == 4 * sizeof(void *));
static_assert(offsetof(DcFunction, body) == sizeof(void *));
static_assert(offsetof(DcFunction, guard) == 2 * sizeof(void *));
static_assert(offsetof(DcFunction, target) == 3 * sizeof(void *));
static_assert(sizeof(DcModule) == 3 * sizeof(void *));
static_assert(offsetof(DcModule, count) == sizeof(void *));
static_assert(offsetof(DcModule, next) == 2 * sizeof(void *));
static_assert(__atomic_always_lock_free(sizeof(DcEntry), nullptr));

namespace {
// Module constructors register their tables before main.
DcModule *modules = nullptr;
}

extern "C" void __dc_register_module(DcModule *module) noexcept {
  module->next = modules;
  modules = module;
}

extern "C" const DcFunction *dc_find(const char *name) noexcept {
  for (auto *module = modules; module; module = module->next) {
    for (size_t i = 0; i < module->count; ++i) {
      const auto *function = &module->functions[i];
      if (std::strcmp(name, function->name) == 0)
        return function;
    }
  }
  return nullptr;
}

extern "C" void dc_enable(const DcFunction *function) noexcept {
  // Match compiler-emitted LLVM atomic pointer loads without pretending the
  // compiler's raw pointer global is a constructed std::atomic object.
  __atomic_store_n(function->target, function->guard, __ATOMIC_RELEASE);
}

extern "C" void dc_disable(const DcFunction *function) noexcept {
  __atomic_store_n(function->target, function->body, __ATOMIC_RELEASE);
}
