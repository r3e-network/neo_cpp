cmake_minimum_required(VERSION 3.20)

project(neo-cpp VERSION 1.0.0 LANGUAGES CXX)

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

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BuildOptions)    # Include build configuration options
include(SetupCompiler)
include(FindDependencies)
include(SetupTesting)

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

# Examples temporarily disabled due to missing dependencies
# if(NEO_BUILD_EXAMPLES)
#     add_subdirectory(examples)
# endif()

# Tools subdirectory
if(NEO_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(NEO_BUILD_APPS)
    add_subdirectory(apps)
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

# Summary
message(STATUS "Neo C++ Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests: ${NEO_BUILD_TESTS}")
message(STATUS "  Examples: ${NEO_BUILD_EXAMPLES}")
message(STATUS "  Tools: ${NEO_BUILD_TOOLS}")
message(STATUS "  Apps: ${NEO_BUILD_APPS}")