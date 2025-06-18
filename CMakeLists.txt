cmake_minimum_required(VERSION 3.20)

# Project configuration
project(Neo-CPP 
    VERSION 1.0.0
    DESCRIPTION "Production-ready Neo blockchain node implementation in C++"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler-specific options
if(MSVC)
    # Visual Studio specific options
    add_compile_options(/W4 /WX /permissive-)
    add_compile_definitions(_WIN32_WINNT=0x0A00) # Windows 10
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    
    # Enable parallel compilation
    add_compile_options(/MP)
    
    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Ob2 /DNDEBUG)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
else()
    # GCC/Clang options
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    
    # Release optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG -march=native)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    
    # Debug options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# vcpkg integration
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
else()
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Find required packages
find_package(Boost REQUIRED COMPONENTS system filesystem thread chrono regex program_options)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

# Optional packages for testing
find_package(GTest QUIET)
if(GTest_FOUND)
    enable_testing()
    set(BUILD_TESTING ON)
else()
    set(BUILD_TESTING OFF)
    message(WARNING "GTest not found, testing disabled")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core library sources
file(GLOB_RECURSE NEO_CORE_SOURCES
    "src/cryptography/*.cpp"
    "src/io/*.cpp"
    "src/ledger/*.cpp"
    "src/network/*.cpp"
    "src/persistence/*.cpp"
    "src/smartcontract/*.cpp"
    "src/vm/*.cpp"
    "src/consensus/*.cpp"
    "src/rpc/*.cpp"
    "src/logging/*.cpp"
    "src/wallets/*.cpp"
    "src/extensions/*.cpp"
    "src/console_service/*.cpp"
    "src/cli/*.cpp"
)

# Filter out temporary and test files
list(FILTER NEO_CORE_SOURCES EXCLUDE REGEX ".*/tmp/.*\\.cpp$")
list(FILTER NEO_CORE_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")
list(FILTER NEO_CORE_SOURCES EXCLUDE REGEX ".*example.*\\.cpp$")

# Core library headers
file(GLOB_RECURSE NEO_CORE_HEADERS
    "include/neo/*.h"
)

# Create core library
add_library(neo-core STATIC ${NEO_CORE_SOURCES} ${NEO_CORE_HEADERS})

# Link libraries to core
target_link_libraries(neo-core
    PUBLIC
        Boost::system
        Boost::filesystem
        Boost::thread
        Boost::chrono
        Boost::regex
        Boost::program_options
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        fmt::fmt
)

# Set target properties
set_target_properties(neo-core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Add node subdirectory
add_subdirectory(src/node)

# CLI executable
add_executable(neo-cli tools/cli/main.cpp)
target_link_libraries(neo-cli PRIVATE neo-core)

# Set executable properties
set_target_properties(neo-cli PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Installation configuration
install(TARGETS neo-node-main neo-cli neo-core neo-node
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/neo
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(FILES config/production_config.json
    DESTINATION etc
    RENAME config.json
)

# Testing configuration
if(BUILD_TESTING)
    # Unit tests
    file(GLOB_RECURSE TEST_SOURCES "tests/unit/*.cpp")
    if(TEST_SOURCES)
        add_executable(neo-unit-tests ${TEST_SOURCES})
        target_link_libraries(neo-unit-tests 
            PRIVATE 
                neo-core 
                GTest::gtest 
                GTest::gtest_main
        )
        
        # Add test discovery
        include(GoogleTest)
        gtest_discover_tests(neo-unit-tests)
    endif()
    
    # Integration tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/neo_integration_tests.cpp")
        add_executable(neo-integration-tests tests/integration/neo_integration_tests.cpp)
        target_link_libraries(neo-integration-tests 
            PRIVATE 
                neo-core 
                GTest::gtest 
                GTest::gtest_main
        )
        
        # Add integration test
        add_test(NAME integration_tests COMMAND neo-integration-tests)
    endif()
    
    # Benchmark tests
    file(GLOB_RECURSE BENCHMARK_SOURCES "tests/benchmarks/*.cpp")
    if(BENCHMARK_SOURCES)
        add_executable(neo-benchmarks ${BENCHMARK_SOURCES})
        target_link_libraries(neo-benchmarks PRIVATE neo-core)
    endif()
endif()

# Documentation generation
find_package(Doxygen QUIET)
if(Doxygen_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    if(EXISTS ${DOXYGEN_IN})
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "Neo-CPP")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Neo C++ Development Team")
set(CPACK_PACKAGE_CONTACT "neo-cpp@neo.org")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Neo C++ Node")
    set(CPACK_NSIS_PACKAGE_NAME "Neo-CPP")
    set(CPACK_NSIS_CONTACT "neo-cpp@neo.org")
    set(CPACK_NSIS_HELP_LINK "https://github.com/neo-project/neo-cpp")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Neo C++ Development Team")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
endif()

include(CPack)

# Custom targets for development
add_custom_target(format
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include 
            -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code"
)

add_custom_target(lint
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include 
            -name "*.cpp" -o -name "*.h" | xargs clang-tidy
    COMMENT "Running static analysis"
)

# Performance profiling target
add_custom_target(profile
    COMMAND valgrind --tool=callgrind --callgrind-out-file=callgrind.out 
            $<TARGET_FILE:neo-node-main> --config config.json
    DEPENDS neo-node-main
    COMMENT "Running performance profiling"
)

# Memory leak detection target
add_custom_target(memcheck
    COMMAND valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all 
            $<TARGET_FILE:neo-node-main> --config config.json
    DEPENDS neo-node-main
    COMMENT "Running memory leak detection"
)

# Print configuration summary
message(STATUS "")
message(STATUS "Neo C++ Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Testing enabled: ${BUILD_TESTING}")
if(Doxygen_FOUND)
    message(STATUS "  Documentation: Enabled")
else()
    message(STATUS "  Documentation: Disabled (Doxygen not found)")
endif()
message(STATUS "")

# Feature summary
message(STATUS "Features:")
message(STATUS "  Boost version: ${Boost_VERSION}")
message(STATUS "  OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "  JSON library: nlohmann/json")
message(STATUS "  Logging: spdlog")
if(GTest_FOUND)
    message(STATUS "  Testing framework: Google Test")
endif()
message(STATUS "")

# Build targets summary
message(STATUS "Build targets:")
message(STATUS "  neo-node - Main blockchain node executable")
message(STATUS "  neo-cli - Command-line interface")
message(STATUS "  neo-core - Core library")
if(BUILD_TESTING)
    message(STATUS "  neo-unit-tests - Unit test suite")
    message(STATUS "  neo-integration-tests - Integration test suite")
    if(BENCHMARK_SOURCES)
        message(STATUS "  neo-benchmarks - Performance benchmarks")
    endif()
endif()
if(Doxygen_FOUND)
    message(STATUS "  docs - API documentation")
endif()
message(STATUS "")

# Development targets
message(STATUS "Development targets:")
message(STATUS "  format - Format source code with clang-format")
message(STATUS "  lint - Run static analysis with clang-tidy")
message(STATUS "  profile - Run performance profiling with valgrind")
message(STATUS "  memcheck - Run memory leak detection with valgrind")
message(STATUS "")

# Warnings for production deployment
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Debug build detected - not suitable for production deployment")
endif()

if(NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(WARNING "Link-time optimization disabled - consider enabling for production")
endif()

message(STATUS "Configuration complete. Run 'make' or 'cmake --build .' to build.")
message(STATUS "")
