cmake_minimum_required(VERSION 3.16)

project(
  gridot
  VERSION 0.1.0
  DESCRIPTION "Get Rid of That - find Debian/Ubuntu packages you no longer need"
  HOMEPAGE_URL "https://github.com/gridot/gridot"
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Compiler)
include(GNUInstallDirs)
include(CMakeDependentOption)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type" FORCE
  )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(Warnings)

set(GRIDOT_SOURCES
    src/main.cpp
    src/util.cpp
    src/graph.cpp
    src/groups.cpp
    src/sections.cpp
    src/report.cpp
    src/core/backend.cpp
    src/core/apt_backend.cpp)

set(GRIDOT_HEADERS
    src/include/apt_backend.h
    src/include/backend.h
    src/include/graph.h
    src/include/groups.h
    src/include/report.h
    src/include/sections.h
    src/include/util.h)

add_executable(gridot ${GRIDOT_SOURCES} ${GRIDOT_HEADERS})

target_include_directories(gridot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gridot PRIVATE gridot::warnings)

set_target_properties(
  gridot
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if(GRIDOT_INSTALL)
  install(
    TARGETS gridot
    EXPORT gridotTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/man/gridot.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    OPTIONAL)
  install(
    EXPORT gridotTargets
    NAMESPACE gridot::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gridot")

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gridotConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gridotConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gridotConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gridot")
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/gridotConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/gridotConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gridot")
endif()

if(GRIDOT_ENABLE_COVERAGE)
  find_program(LCOV_EXECUTABLE lcov)
  if(LCOV_EXECUTABLE)
    add_custom_target(
      coverage
      COMMAND ${LCOV_EXECUTABLE} --capture --directory "${CMAKE_BINARY_DIR}" --output-file lcov.info
      COMMAND ${LCOV_EXECUTABLE} --remove lcov.info '/usr/*' --output-file lcov.info
      COMMAND ${LCOV_EXECUTABLE} --list lcov.info
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMENT "Generating coverage report (lcov.info)"
      VERBATIM)
  endif()
endif()