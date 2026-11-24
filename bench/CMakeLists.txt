set(DC_CLANG "${PROJECT_SOURCE_DIR}/build/llvm/bin/clang++" CACHE FILEPATH
    "Patched LLVM 23 compiler used only for benchmark targets")
option(DC_LTO "Build targets, harness, and runtime with full LTO (use DC_CLANG as CXX)" OFF)
if(NOT EXISTS "${DC_CLANG}")
  message(FATAL_ERROR "Patched clang++ not found; run make compiler first or set DC_CLANG")
endif()

include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.10.0.tar.gz
  URL_HASH SHA256=acc5c7c0fbed855c4aaa2d7c3c36fb82b0c15c28f4e151e3f679c2fc0aa404af)
FetchContent_MakeAvailable(googlebenchmark)

# Only targets.cc goes through the extension. Harness, runtime, and
# benchmark library share the same fixed host compiler and build settings.
set(target_object "${CMAKE_CURRENT_BINARY_DIR}/targets.o")
set(target_flags -std=c++17 -O2 -fno-exceptions -fno-lto -mllvm -dc-enable)
if(DC_LTO)
  list(REMOVE_ITEM target_flags -fno-lto)
  list(APPEND target_flags -flto=full)
endif()
if(APPLE)
  execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE dc_sdk
                  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
  list(APPEND target_flags -isysroot "${dc_sdk}")
endif()
add_custom_command(OUTPUT "${target_object}"
  COMMAND "${DC_CLANG}" ${target_flags} -c "${CMAKE_CURRENT_SOURCE_DIR}/targets.cc"
          -o "${target_object}"
  DEPENDS targets.cc "${DC_CLANG}"
  VERBATIM)
set_source_files_properties("${target_object}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
add_executable(dc_bench bench.cc empty_hooks.cc "${target_object}")
set_target_properties(dc_bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
  INTERPROCEDURAL_OPTIMIZATION FALSE)
target_link_libraries(dc_bench PRIVATE dynamic_control benchmark::benchmark)
if(DC_LTO)
  target_compile_options(dynamic_control PRIVATE -flto=full)
  target_compile_options(dc_bench PRIVATE -flto=full)
  target_link_options(dc_bench PRIVATE -flto=full)
  get_filename_component(dc_tools "${DC_CLANG}" DIRECTORY)
  if(APPLE)
    target_link_options(dc_bench PRIVATE "-Wl,-lto_library,${dc_tools}/../lib/libLTO.dylib")
  else()
    target_link_options(dc_bench PRIVATE "-fuse-ld=${dc_tools}/ld.lld")
  endif()
endif()
