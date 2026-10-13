# SPDX-License-Identifier: Apache-2.0
#
# Compiler/toolchain discovery for gridot.
#
# Defines the language standard once, validates the minimum CMake version, and resolves the
# generator-independent build settings every preset inherits.

include_guard(GLOBAL)

if(CMAKE_VERSION VERSION_LESS 3.16)
  message(FATAL_ERROR "gridot requires CMake >= 3.16, found ${CMAKE_VERSION}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(GRIDOT_VERSION_MAJOR 0)
set(GRIDOT_VERSION_MINOR 1)
set(GRIDOT_VERSION_PATCH 0)
set(GRIDOT_VERSION "${GRIDOT_VERSION_MAJOR}.${GRIDOT_VERSION_MINOR}.${GRIDOT_VERSION_PATCH}")

option(GRIDOT_INSTALL "Generate install rules for gridot" ON)
