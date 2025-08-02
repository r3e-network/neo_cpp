# Build configuration options for Neo C++

# Build type options
set(NEO_BUILD_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "Available build types")

# Feature options
option(NEO_BUILD_TESTS "Build unit tests" ON)
option(NEO_BUILD_EXAMPLES "Build example applications" ON)
option(NEO_BUILD_TOOLS "Build command line tools" ON)
option(NEO_BUILD_DOCS "Build documentation" OFF)
option(NEO_BUILD_BENCHMARKS "Build performance benchmarks" OFF)

# Performance options
option(NEO_ENABLE_LTO "Enable Link Time Optimization" OFF)
option(NEO_ENABLE_NATIVE_ARCH "Build for native architecture" OFF)
option(NEO_ENABLE_PROFILING "Enable profiling support" OFF)
option(NEO_ENABLE_SANITIZERS "Enable address and UB sanitizers" OFF)
option(NEO_ENABLE_COVERAGE "Enable code coverage" OFF)

# Storage backend options
option(NEO_USE_ROCKSDB "Use RocksDB storage backend" OFF)
option(NEO_USE_LEVELDB "Use LevelDB storage backend" OFF)
option(NEO_USE_MEMORY_STORE "Use in-memory storage (default)" ON)

# Network options
option(NEO_ENABLE_TLS "Enable TLS support" ON)
option(NEO_ENABLE_COMPRESSION "Enable network compression" ON)

# VM options
option(NEO_VM_ENABLE_JIT "Enable JIT compilation (experimental)" OFF)
option(NEO_VM_ENABLE_DEBUG "Enable VM debugging features" OFF)

# Deployment options
option(NEO_BUILD_STATIC "Build static libraries" OFF)
option(NEO_BUILD_SHARED "Build shared libraries" ON)
option(NEO_INSTALL_HEADERS "Install header files" ON)
option(NEO_INSTALL_CMAKE "Install CMake package files" ON)

# Security options
option(NEO_ENABLE_HARDENING "Enable security hardening flags" ON)
option(NEO_ENABLE_PIE "Build position independent executables" ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${NEO_BUILD_TYPES})
endif()

# Platform-specific options
if(WIN32)
    option(NEO_USE_WINDOWS_CRYPTO "Use Windows crypto APIs" ON)
elseif(APPLE)
    option(NEO_USE_SECURITY_FRAMEWORK "Use macOS Security Framework" ON)
else()
    option(NEO_USE_OPENSSL "Use OpenSSL for cryptography" ON)
endif()

# Configure compiler flags based on options
function(neo_configure_target target)
    # C++ standard
    target_compile_features(${target} PUBLIC cxx_std_20)
    
    # Common flags
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
            -Wno-unused-function
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /wd4100  # unreferenced formal parameter
            /wd4458  # declaration hides class member
            /wd4996  # deprecated functions
        >
    )
    
    # Debug flags
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Debug>:
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-g3 -O0>
            $<$<CXX_COMPILER_ID:MSVC>:/Od /RTC1>
        >
    )
    
    # Release flags
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Release>:
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O3 -DNDEBUG>
            $<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>
        >
    )
    
    # Native architecture optimization
    if(NEO_ENABLE_NATIVE_ARCH)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-march=native>
        )
    endif()
    
    # Link Time Optimization
    if(NEO_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT lto_supported OUTPUT error)
        if(lto_supported)
            set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(WARNING "LTO not supported: ${error}")
        endif()
    endif()
    
    # Sanitizers
    if(NEO_ENABLE_SANITIZERS)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            >
        )
        target_link_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
                -fsanitize=address,undefined
            >
        )
    endif()
    
    # Security hardening
    if(NEO_ENABLE_HARDENING)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
                -D_FORTIFY_SOURCE=2
                -fstack-protector-strong
                -fstack-clash-protection
                -fcf-protection
            >
            $<$<CXX_COMPILER_ID:MSVC>:
                /GS /guard:cf
            >
        )
        
        target_compile_definitions(${target} PRIVATE
            $<$<PLATFORM_ID:Linux>:_GLIBCXX_ASSERTIONS>
        )
    endif()
    
    # Position Independent Executable
    if(NEO_ENABLE_PIE)
        set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    
    # Coverage
    if(NEO_ENABLE_COVERAGE)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
                --coverage -fprofile-arcs -ftest-coverage
            >
        )
        target_link_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
                --coverage
            >
        )
    endif()
endfunction()

# Print configuration summary
function(neo_print_configuration)
    message(STATUS "Neo C++ Build Configuration:")
    message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "  Tests: ${NEO_BUILD_TESTS}")
    message(STATUS "  Examples: ${NEO_BUILD_EXAMPLES}")
    message(STATUS "  Tools: ${NEO_BUILD_TOOLS}")
    message(STATUS "  Documentation: ${NEO_BUILD_DOCS}")
    message(STATUS "  Benchmarks: ${NEO_BUILD_BENCHMARKS}")
    message(STATUS "  LTO: ${NEO_ENABLE_LTO}")
    message(STATUS "  Native arch: ${NEO_ENABLE_NATIVE_ARCH}")
    message(STATUS "  Sanitizers: ${NEO_ENABLE_SANITIZERS}")
    message(STATUS "  Coverage: ${NEO_ENABLE_COVERAGE}")
    message(STATUS "  Hardening: ${NEO_ENABLE_HARDENING}")
    message(STATUS "  Storage backend: ${NEO_STORAGE_BACKEND}")
endfunction()