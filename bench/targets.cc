#include <cstdint>

extern "C" {
// Identical work and explicit noinline on both sides of the comparison.
__attribute__((noinline))
std::uint64_t bench_plain(std::uint64_t x) noexcept { return x * 3 + 1; }

[[clang::annotate("dc")]] __attribute__((noinline))
std::uint64_t bench_controlled(std::uint64_t x) noexcept { return x * 3 + 1; }

const char *bench_target_compiler() noexcept { return __clang_version__; }
}
