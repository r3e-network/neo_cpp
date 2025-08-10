#pragma once

/**
 * @file compiler_config.h
 * @brief Compiler configuration and feature detection for Neo N3 C++ Node
 */

// Check C++20 support
#if __cplusplus < 202002L
#error "Neo N3 C++ Node requires C++20 support"
#endif

// Compiler identification
#if defined(_MSC_VER)
#define NEO_COMPILER_MSVC 1
#define NEO_COMPILER_VERSION _MSC_VER

// MSVC-specific configurations
#pragma warning(push)
#pragma warning(disable : 4996)  // Disable deprecated function warnings

// Enable C++20 features
#if _MSVC_LANG < 202002L
#error "MSVC C++20 mode not enabled. Use /std:c++20 or /std:c++latest"
#endif

#elif defined(__GNUC__)
#define NEO_COMPILER_GCC 1
#define NEO_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// GCC-specific configurations
#if __GNUC__ < 10
#error "Neo N3 C++ Node requires GCC 10 or later for C++20 support"
#endif

#elif defined(__clang__)
#define NEO_COMPILER_CLANG 1
#define NEO_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)

// Clang-specific configurations
#if __clang_major__ < 12
#error "Neo N3 C++ Node requires Clang 12 or later for C++20 support"
#endif

#else
#warning "Unknown compiler - Neo N3 C++ Node may not compile correctly"
#endif

// Platform identification
#if defined(_WIN32) || defined(_WIN64)
#define NEO_PLATFORM_WINDOWS 1

// Windows-specific includes and definitions
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#elif defined(__linux__)
#define NEO_PLATFORM_LINUX 1

#elif defined(__APPLE__)
#define NEO_PLATFORM_MACOS 1

#else
#warning "Unknown platform - Neo N3 C++ Node may not work correctly"
#endif

// C++20 feature testing
#include <version>

// Concepts support
#ifdef __cpp_concepts
#define NEO_HAS_CONCEPTS 1
#else
#define NEO_HAS_CONCEPTS 0
#warning "C++20 concepts not available"
#endif

// Modules support
#ifdef __cpp_modules
#define NEO_HAS_MODULES 1
#else
#define NEO_HAS_MODULES 0
#endif

// Coroutines support
#ifdef __cpp_impl_coroutine
#define NEO_HAS_COROUTINES 1
#else
#define NEO_HAS_COROUTINES 0
#endif

// Ranges support
#ifdef __cpp_lib_ranges
#define NEO_HAS_RANGES 1
#else
#define NEO_HAS_RANGES 0
#endif

// Format library support
#ifdef __cpp_lib_format
#define NEO_HAS_FORMAT 1
#else
#define NEO_HAS_FORMAT 0
#endif

// Debugging macros
#ifdef DEBUG
#define NEO_DEBUG 1
#define NEO_ASSERT(condition, message)                                                                       \
    do                                                                                                       \
    {                                                                                                        \
        if (!(condition))                                                                                    \
        {                                                                                                    \
            std::cerr << "Assertion failed: " << #condition << " - " << message << " at " << __FILE__ << ":" \
                      << __LINE__ << std::endl;                                                              \
            std::abort();                                                                                    \
        }                                                                                                    \
    } while (0)
#else
#define NEO_DEBUG 0
#define NEO_ASSERT(condition, message) ((void)0)
#endif

// Likely/unlikely hints for better optimization
#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(likely)
#define NEO_LIKELY [[likely]]
#else
#define NEO_LIKELY
#endif

#if __has_cpp_attribute(unlikely)
#define NEO_UNLIKELY [[unlikely]]
#else
#define NEO_UNLIKELY
#endif
#else
#define NEO_LIKELY
#define NEO_UNLIKELY
#endif

// Force inline
#if defined(NEO_COMPILER_MSVC)
#define NEO_FORCE_INLINE __forceinline
#elif defined(NEO_COMPILER_GCC) || defined(NEO_COMPILER_CLANG)
#define NEO_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define NEO_FORCE_INLINE inline
#endif

// No inline
#if defined(NEO_COMPILER_MSVC)
#define NEO_NO_INLINE __declspec(noinline)
#elif defined(NEO_COMPILER_GCC) || defined(NEO_COMPILER_CLANG)
#define NEO_NO_INLINE __attribute__((noinline))
#else
#define NEO_NO_INLINE
#endif

// Thread local storage
#if defined(NEO_COMPILER_MSVC)
#define NEO_THREAD_LOCAL __declspec(thread)
#elif defined(NEO_COMPILER_GCC) || defined(NEO_COMPILER_CLANG)
#define NEO_THREAD_LOCAL __thread
#else
#define NEO_THREAD_LOCAL thread_local
#endif

// Export/import for shared libraries
#if defined(NEO_PLATFORM_WINDOWS)
#ifdef NEO_EXPORTS
#define NEO_API __declspec(dllexport)
#else
#define NEO_API __declspec(dllimport)
#endif
#else
#define NEO_API __attribute__((visibility("default")))
#endif

// Cleanup MSVC warnings
#if defined(NEO_COMPILER_MSVC)
#pragma warning(pop)
#endif