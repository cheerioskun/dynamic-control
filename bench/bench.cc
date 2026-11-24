#include "dynamic_control.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" std::uint64_t bench_plain(std::uint64_t) noexcept;
extern "C" std::uint64_t bench_controlled(std::uint64_t) noexcept;
extern "C" const char *bench_target_compiler() noexcept;

template <auto Target>
static void BM_Call(benchmark::State &state) {
  std::uint64_t value = 1;
  for (auto _ : state) {
    value = Target(value);
    benchmark::DoNotOptimize(value);
  }
}

int main(int argc, char **argv) {
  const char *mode = nullptr;
  int remaining = 1;
  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--dc-mode=", 10) == 0) {
      mode = argv[i] + 10;
    } else {
      argv[remaining++] = argv[i];
    }
  }
  argc = remaining;
  argv[argc] = nullptr;
  if (!mode || (std::strcmp(mode, "baseline") != 0 &&
                std::strcmp(mode, "unhooked") != 0 &&
                std::strcmp(mode, "hooked") != 0)) {
    std::fprintf(stderr, "Usage: %s --dc-mode=baseline|unhooked|hooked [benchmark options]\n", argv[0]);
    return 1;
  }
  const DcFunction *function = dc_find("bench_controlled");
  if (!function || dc_find("bench_plain")) {
    std::fprintf(stderr, "Expected exactly the annotated target to be registered\n");
    return 1;
  }
  if (std::strcmp(mode, "hooked") == 0)
    dc_enable(function);
  else
    dc_disable(function);

  auto *loop = std::strcmp(mode, "baseline") == 0 ? &BM_Call<bench_plain>
                                                 : &BM_Call<bench_controlled>;
  benchmark::RegisterBenchmark(mode, loop)->Unit(benchmark::kNanosecond);
  benchmark::AddCustomContext("dc_mode", mode);
  benchmark::AddCustomContext("hook_backend", "empty");
  benchmark::AddCustomContext("target_compiler", bench_target_compiler());
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
