cmake_minimum_required(VERSION 3.20)

# Set CMake policies to avoid warnings
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)  # Use new FindBoost module behavior
endif()

project(neo-cpp VERSION 1.2.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(NEO_BUILD_TESTS "Build tests" ON)
option(NEO_BUILD_EXAMPLES "Build examples" ON)
option(NEO_BUILD_TOOLS "Build CLI tools" ON)
option(NEO_BUILD_APPS "Build applications" ON)
option(NEO_USE_VCPKG "Use vcpkg for dependencies" ON)

# Production build options
option(NEO_PRODUCTION_BUILD "Enable production optimizations" OFF)
option(NEO_ENABLE_PROFILING "Enable profiling support" OFF)
option(NEO_ENABLE_SANITIZERS "Enable address and UB sanitizers" OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Ccache)          # Enable ccache for faster builds
include(BuildOptions)    # Include build configuration options
include(SetupCompiler)
include(FindDependencies)
include(SetupTesting)
include(CodeCoverage)    # Code coverage configuration
include(Docker)          # Docker integration targets

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include GoogleTest if not found
if(NEO_BUILD_TESTS AND NOT TARGET GTest::gtest)
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/googletest/CMakeLists.txt)
        message(STATUS "Building GoogleTest from source")
        add_subdirectory(third_party/googletest)
    endif()
endif()

# Add subdirectories
add_subdirectory(src)

if(NEO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
option(NEO_BUILD_BENCHMARKS "Build performance benchmarks" ON)
if(NEO_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Examples can be disabled to stabilize builds
if(NEO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tools subdirectory
if(NEO_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(NEO_BUILD_APPS)
    add_subdirectory(apps)
endif()

# SDK subdirectory
option(NEO_BUILD_SDK "Build Neo C++ SDK" ON)
if(NEO_BUILD_SDK)
    add_subdirectory(sdk)
endif()

# Plugin system (temporarily disabled due to namespace conflicts)
# add_subdirectory(plugins)

# Installation
include(GNUInstallDirs)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install configuration files
install(FILES config/neo.config config/production_config.json
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/neo
)

# Create package config
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/neo-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/neo-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/neo"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/neo-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/neo-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/neo-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/neo"
)

# Include custom targets (Makefile compatibility)
include(CustomTargets)

# Summary
message(STATUS "Neo C++ Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests: ${NEO_BUILD_TESTS}")
message(STATUS "  Examples: ${NEO_BUILD_EXAMPLES}")
message(STATUS "  Tools: ${NEO_BUILD_TOOLS}")
message(STATUS "  Apps: ${NEO_BUILD_APPS}")
message(STATUS "  SDK: ${NEO_BUILD_SDK}")