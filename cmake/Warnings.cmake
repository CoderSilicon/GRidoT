# Compiler-warning and diagnostics helper.
#
# Included from the top-level CMakeLists.txt. Defines an interface target `gridot::warnings` that
# carries the warning set used for every source file in the project, so flags are configured exactly
# once.

include_guard(GLOBAL)

option(GRIDOT_ENABLE_WERROR "Treat compiler warnings as errors" OFF)
option(GRIDOT_ENABLE_SANITIZERS "Enable ASan/UBSan instrumentation" OFF)
option(GRIDOT_ENABLE_COVERAGE "Enable gcov/lcov coverage instrumentation" OFF)

add_library(gridot_warnings INTERFACE)
add_library(gridot::warnings ALIAS gridot_warnings)

if(MSVC)
  target_compile_options(gridot_warnings INTERFACE /W4 /permissive- /Zc:preprocessor)
  if(GRIDOT_ENABLE_WERROR)
    target_compile_options(gridot_warnings INTERFACE /WX)
  endif()
  target_compile_definitions(gridot_warnings INTERFACE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
  target_compile_options(
    gridot_warnings
    INTERFACE -Wall
              -Wextra
              -Wpedantic
              -Wnon-virtual-dtor
              -Wunused
              -Woverloaded-virtual
              -Wdouble-promotion
              -Wformat=2)
  if(GRIDOT_ENABLE_WERROR)
    target_compile_options(gridot_warnings INTERFACE -Werror)
  endif()
endif()

if(GRIDOT_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(
    gridot_warnings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer
                              -fno-optimize-sibling-calls)
  target_link_options(gridot_warnings INTERFACE -fsanitize=address,undefined)
endif()

if(GRIDOT_ENABLE_COVERAGE AND NOT MSVC)
  target_compile_options(gridot_warnings INTERFACE --coverage -O0 -g)
  target_link_options(gridot_warnings INTERFACE --coverage)
endif()
