#include <cstdint>

extern "C" {
void fixture_disable_current() noexcept;

__attribute__((noinline))
std::uint64_t plain(std::uint64_t x) noexcept { return x * 3 + 1; }

[[clang::annotate("dc")]] __attribute__((noinline))
std::uint64_t controlled(std::uint64_t x) noexcept { return x * 3 + 1; }

[[clang::annotate("dc")]] __attribute__((noinline))
void notify(std::uint64_t *value) noexcept { ++*value; }

[[clang::annotate("dc")]] __attribute__((noinline))
std::uint64_t recursive(std::uint64_t n) noexcept {
  return n <= 1 ? 1 : n * recursive(n - 1);
}

[[clang::annotate("dc")]] __attribute__((noinline))
std::uint64_t disable_self(std::uint64_t x) noexcept {
  fixture_disable_current();
  return x + 1;
}

[[clang::annotate("dc")]] __attribute__((noinline))
std::uint64_t sum10(std::uint64_t a, std::uint64_t b, std::uint64_t c,
                    std::uint64_t d, std::uint64_t e, std::uint64_t f,
                    std::uint64_t g, std::uint64_t h, std::uint64_t i,
                    std::uint64_t j) noexcept {
  return a + b + c + d + e + f + g + h + i + j;
}

[[clang::annotate("dc")]] __attribute__((noinline))
double floating(double x, double y) noexcept { return x + y; }
}
