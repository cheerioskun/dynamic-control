#include "dynamic_control.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

extern "C" {
std::uint64_t plain(std::uint64_t) noexcept;
std::uint64_t controlled(std::uint64_t) noexcept;
void notify(std::uint64_t *) noexcept;
std::uint64_t recursive(std::uint64_t) noexcept;
std::uint64_t disable_self(std::uint64_t) noexcept;
std::uint64_t sum10(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t,
                    std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t,
                    std::uint64_t, std::uint64_t) noexcept;
double floating(double, double) noexcept;
std::uint64_t other_module(std::uint64_t) noexcept;
}

namespace {
std::atomic<unsigned> acquires{0}, releases{0};
const DcFunction *disable_current = nullptr;

void check(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "smoke failed: %s\n", message);
    std::abort();
  }
}

const DcFunction *find(const char *name) {
  auto *function = dc_find(name);
  check(function != nullptr, name);
  return function;
}
}

// Audit backend replaces the mock only in this correctness fixture.
extern "C" void dc_acquire(const DcFunction *function) noexcept {
  check(function && function->body && function->guard && function->target,
        "descriptor ABI");
  acquires.fetch_add(1, std::memory_order_relaxed);
}
extern "C" void dc_release(const DcFunction *) noexcept {
  releases.fetch_add(1, std::memory_order_relaxed);
}
extern "C" void fixture_disable_current() noexcept {
  if (disable_current)
    dc_disable(disable_current);
}

int main(int argc, char **) {
  check(plain(7) == 22 && controlled(7) == 22, "initial results");
  check(dc_find("plain") == nullptr, "unannotated function absent");
  check(dc_find("missing") == nullptr, "missing name");
  if (argc > 1) {
    check(dc_find("controlled") == nullptr, "pass disabled");
    check(acquires == 0 && releases == 0, "disabled pass has no hooks");
    std::puts("smoke: compiler switch off leaves annotations inert");
    return 0;
  }

  const auto *function = find("controlled");
  check(acquires == 0 && releases == 0, "starts disabled");
  dc_enable(function);
  check(controlled(7) == 22 && acquires == 1 && releases == 1, "enabled direct call");
  auto *address = &controlled;
  check(address(8) == 25 && acquires == 2 && releases == 2, "function address");
  dc_disable(function);
  check(controlled(7) == 22 && acquires == 2 && releases == 2, "disabled again");

  auto *notification = find("notify");
  dc_enable(notification);
  std::uint64_t value = 10;
  notify(&value);
  check(value == 11 && acquires == 3 && releases == 3, "void/pointer forwarding");
  dc_disable(notification);

  auto *recursion = find("recursive");
  dc_enable(recursion);
  auto before = acquires.load();
  check(recursive(5) == 120 && acquires > before && acquires == releases,
        "recursive result and balanced hooks");
  dc_disable(recursion);

  auto *sum = find("sum10");
  dc_enable(sum);
  check(sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 55,
        "stack argument forwarding");
  dc_disable(sum);
  auto *fp = find("floating");
  dc_enable(fp);
  check(floating(1.25, 2.5) == 3.75, "floating-point forwarding");
  dc_disable(fp);
  auto *other = find("other_module");
  dc_enable(other);
  check(other_module(7) == 9, "second module registration");
  dc_disable(other);

  disable_current = find("disable_self");
  dc_enable(disable_current);
  before = acquires.load();
  check(disable_self(7) == 8, "disable inside active invocation");
  check(acquires == before + 1 && releases == acquires,
        "active guard still releases after disable");
  check(disable_self(7) == 8 && acquires == before + 1, "next entry unhooked");
  disable_current = nullptr;

  std::thread toggler([&] {
    for (unsigned i = 0; i < 100000; ++i) {
      dc_enable(function);
      dc_disable(function);
    }
  });
  for (unsigned i = 0; i < 100000; ++i)
    check(controlled(i) == i * 3 + 1, "call racing with toggle");
  toggler.join();
  check(acquires == releases, "balanced permits after concurrent toggling");
  std::puts("smoke: annotations, two modules, direct/indirect calls, recursion, ABI, toggles passed");
}
