add_library(dynamic_control STATIC dynamic_control.cc)
target_compile_features(dynamic_control PUBLIC cxx_std_17)
target_include_directories(dynamic_control PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(dynamic_control PRIVATE -Wall -Wextra -Werror)
