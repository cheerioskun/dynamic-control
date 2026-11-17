#include "dynamic_control.h"

#include <atomic>
#include <cstdlib>
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
// Constant initialization: safe before compiler-generated module constructors.
std::atomic<DcModule *> modules{nullptr};
}

extern "C" void __dc_register_module(DcModule *module) noexcept {
  auto *head = modules.load(std::memory_order_relaxed);
  do {
    module->next = head;
  } while (!modules.compare_exchange_weak(head, module,
                                        std::memory_order_release,
                                        std::memory_order_relaxed));
}

extern "C" const DcFunction *dc_find(const char *name) noexcept {
  if (!name)
    return nullptr;
  const DcFunction *found = nullptr;
  for (auto *module = modules.load(std::memory_order_acquire); module;
       module = module->next) {
    for (size_t i = 0; i < module->count; ++i) {
      const auto *function = &module->functions[i];
      if (std::strcmp(name, function->name) == 0) {
        if (found)
          return nullptr;
        found = function;
      }
    }
  }
  return found;
}

extern "C" void dc_enable(const DcFunction *function) noexcept {
  if (!function)
    std::abort();
  // Match compiler-emitted LLVM atomic pointer loads without pretending the
  // compiler's raw pointer global is a constructed std::atomic object.
  __atomic_store_n(function->target, function->guard, __ATOMIC_RELEASE);
}

extern "C" void dc_disable(const DcFunction *function) noexcept {
  if (!function)
    std::abort();
  __atomic_store_n(function->target, function->body, __ATOMIC_RELEASE);
}
