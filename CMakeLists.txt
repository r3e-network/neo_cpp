cmake_minimum_required(VERSION 3.15)
project(neo_cpp VERSION 0.1.0 LANGUAGES CXX)

# Use vcpkg toolchain
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Try multiple common locations for vcpkg
    set(VCPKG_PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
    )
    
    foreach(VCPKG_PATH ${VCPKG_PATHS})
        if(EXISTS ${VCPKG_PATH})
            set(CMAKE_TOOLCHAIN_FILE ${VCPKG_PATH} CACHE STRING "")
            message(STATUS "Found vcpkg toolchain at: ${VCPKG_PATH}")
            break()
        endif()
    endforeach()
    
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(WARNING "vcpkg toolchain file not found. Please install vcpkg and set CMAKE_TOOLCHAIN_FILE manually.")
    endif()
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific configuration
if(MSVC)
    # MSVC-specific flags
    add_compile_options(
        /W4                     # Warning level 4
        /WX-                    # Don't treat warnings as errors (for now)
        /permissive-            # Disable non-conforming code
        /Zc:__cplusplus         # Enable correct __cplusplus macro
        /Zc:preprocessor        # Enable conforming preprocessor
        /EHsc                   # Exception handling model
        /bigobj                 # Allow large object files
        /utf-8                  # Set source and execution character sets to UTF-8
    )
    
    # MSVC-specific definitions
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS      # Disable CRT security warnings
        -D_SCL_SECURE_NO_WARNINGS      # Disable SCL security warnings
        -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
    
    # Debug configuration
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Od /Zi /RTC1")
    
    # Release configuration
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2 /DNDEBUG")
    
    # RelWithDebInfo configuration
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /O2 /Ob1 /Zi")
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC-specific flags
    add_compile_options(
        -Wall -Wextra -Wpedantic   # Enable warnings
        -Wno-unused-parameter      # Disable unused parameter warnings
        -fexceptions               # Enable exceptions
        -frtti                     # Enable RTTI
    )
    
    # Debug configuration
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    
    # Release configuration
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang-specific flags
    add_compile_options(
        -Wall -Wextra -Wpedantic   # Enable warnings
        -Wno-unused-parameter      # Disable unused parameter warnings
        -fexceptions               # Enable exceptions
        -frtti                     # Enable RTTI
    )
    
    # Debug configuration
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    
    # Release configuration  
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Windows-specific definitions
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 target
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    
    # Additional Windows definitions for Neo
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DSTRICT)
endif()

# Platform-independent definitions
add_definitions(-DNEO_VERSION_MAJOR=0)
add_definitions(-DNEO_VERSION_MINOR=1)
add_definitions(-DNEO_VERSION_PATCH=0)

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Function to configure common target properties
function(configure_neo_target target_name)
    # Set C++ standard for this target
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    
    # Set compile definitions
    target_compile_definitions(${target_name} PRIVATE
        $<$<CONFIG:Debug>:DEBUG _DEBUG>
        $<$<CONFIG:Release>:RELEASE NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:RELEASE>
        $<$<CONFIG:MinSizeRel>:RELEASE NDEBUG>
    )
    
    # MSVC-specific target configuration
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Debug>:/MDd>
            $<$<CONFIG:Release>:/MD>
            $<$<CONFIG:RelWithDebInfo>:/MD>
            $<$<CONFIG:MinSizeRel>:/MD>
        )
    endif()
    
    # Set target properties
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
    
    # Windows-specific target configuration
    if(WIN32)
        set_target_properties(${target_name} PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()
endfunction()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages
message(STATUS "Finding required packages...")

# Boost - required for networking and system utilities
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
if(Boost_FOUND)
    message(STATUS "Found Boost ${Boost_VERSION}")
else()
    message(FATAL_ERROR "Boost not found. Please install boost via vcpkg: vcpkg install boost-system boost-filesystem boost-thread")
endif()

# OpenSSL - required for cryptography
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")
endif()

# nlohmann_json - required for JSON processing
find_package(nlohmann_json CONFIG REQUIRED)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json")
endif()

# spdlog - required for logging
find_package(spdlog CONFIG REQUIRED)
if(spdlog_FOUND)
    message(STATUS "Found spdlog")
endif()

# GTest - required for testing
find_package(GTest CONFIG REQUIRED)
if(GTest_FOUND)
    message(STATUS "Found GTest")
endif()

# RocksDB - optional for persistent storage
find_package(RocksDB CONFIG)
if(RocksDB_FOUND)
    message(STATUS "Found RocksDB - persistent storage enabled")
    add_definitions(-DWITH_ROCKSDB)
else()
    message(WARNING "RocksDB not found - using memory store only. Install with: vcpkg install rocksdb")
    # Create a dummy target for compatibility
    add_library(RocksDB::rocksdb INTERFACE IMPORTED)
    add_definitions(-DNO_ROCKSDB)
endif()

# Include directories
include_directories(include)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(tests)
