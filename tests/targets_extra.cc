#include <cstdint>

// A separate translation unit exercises constructor registration of two tables.
extern "C" [[clang::annotate("dc")]] __attribute__((noinline))
std::uint64_t other_module(std::uint64_t x) noexcept { return x + 2; }
