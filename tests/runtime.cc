#include "dynamic_control.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

// Handwritten compiler ABI fixture. This tests the runtime before the real pass
// exists; it is not evidence that compiler-generated dispatch works.
namespace {
std::atomic<unsigned> acquires{0}, releases{0};
bool disable_in_body = false;
int body(int);
int guard(int);
DcEntry target = reinterpret_cast<DcEntry>(body);
const DcFunction functions[] = {{"fixture", reinterpret_cast<DcEntry>(body),
                                reinterpret_cast<DcEntry>(guard), &target}};
DcModule module{functions, 1, nullptr};

void check(bool condition) {
  if (!condition)
    std::abort();
}

int body(int x) {
  if (disable_in_body)
    dc_disable(functions);
  return x + 1;
}

int guard(int x) {
  dc_acquire(functions);
  int result = body(x);
  dc_release(functions);
  return result;
}

int dispatch(int x) {
  auto destination = __atomic_load_n(&target, __ATOMIC_ACQUIRE);
  return reinterpret_cast<int (*)(int)>(destination)(x);
}
}

extern "C" void dc_acquire(const DcFunction *function) noexcept {
  check(function == functions);
  acquires.fetch_add(1, std::memory_order_relaxed);
}
extern "C" void dc_release(const DcFunction *function) noexcept {
  check(function == functions);
  releases.fetch_add(1, std::memory_order_relaxed);
}

int main() {
  check(dc_find("fixture") == nullptr);
  __dc_register_module(&module);
  check(dc_find(nullptr) == nullptr && dc_find("absent") == nullptr);
  const auto *function = dc_find("fixture");
  check(function == functions);
  check(dispatch(41) == 42 && acquires == 0 && releases == 0);
  dc_enable(function);
  check(dispatch(41) == 42 && acquires == 1 && releases == 1);
  dc_disable(function);
  check(dispatch(41) == 42 && acquires == 1 && releases == 1);

  dc_enable(function);
  disable_in_body = true;
  check(dispatch(41) == 42 && acquires == 2 && releases == 2);
  disable_in_body = false;
  check(dispatch(41) == 42 && acquires == 2 && releases == 2);

  std::thread toggler([&] {
    for (unsigned i = 0; i < 100000; ++i) {
      dc_enable(function);
      dc_disable(function);
    }
  });
  for (unsigned i = 0; i < 100000; ++i)
    check(dispatch(41) == 42);
  toggler.join();
  check(acquires == releases);

  DcModule duplicate{functions, 1, nullptr};
  __dc_register_module(&duplicate);
  check(dc_find("fixture") == nullptr);
  std::puts("runtime: lookup, ambiguity, toggling, release after disable, concurrent dispatch passed");
}
