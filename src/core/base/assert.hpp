/**
 * @file assert.hpp
 * @brief Debug assertion and runtime checking utilities for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides assertion macros and runtime checks for debugging
 * and validation. Assertions are stripped in release builds for performance.
 */

#pragma once

#include "config.hpp"
#include <cstdlib>
#include <iostream>

namespace nomad {

//=============================================================================
// Assertion Handler
//=============================================================================

/// Function pointer type for custom assertion handlers
using AssertHandler = void(*)(const char* expression, const char* message, 
                               const char* file, int line, const char* function);

/// Default assertion handler that prints to stderr and aborts
inline void defaultAssertHandler(const char* expression, const char* message,
                                  const char* file, int line, const char* function) {
    std::cerr << "\n"
              << "========== ASSERTION FAILED ==========\n"
              << "Expression: " << expression << "\n"
              << "Message:    " << (message ? message : "(none)") << "\n"
              << "File:       " << file << "\n"
              << "Line:       " << line << "\n"
              << "Function:   " << function << "\n"
              << "======================================\n"
              << std::endl;
    
    std::abort();
}

/// Global assertion handler (can be replaced for custom handling)
inline AssertHandler& getAssertHandler() {
    static AssertHandler handler = defaultAssertHandler;
    return handler;
}

/// Set a custom assertion handler
inline void setAssertHandler(AssertHandler handler) {
    getAssertHandler() = handler ? handler : defaultAssertHandler;
}

/// Trigger an assertion failure
inline void assertFailed(const char* expression, const char* message,
                         const char* file, int line, const char* function) {
    getAssertHandler()(expression, message, file, line, function);
}

} // namespace nomad

//=============================================================================
// Assertion Macros
//=============================================================================

#ifdef NOMAD_BUILD_DEBUG

/// Standard debug assertion - compiled out in release
#define NOMAD_ASSERT(expr) \
    do { \
        if (NOMAD_UNLIKELY(!(expr))) { \
            ::nomad::assertFailed(#expr, nullptr, __FILE__, __LINE__, __func__); \
        } \
    } while (false)

/// Debug assertion with custom message
#define NOMAD_ASSERT_MSG(expr, msg) \
    do { \
        if (NOMAD_UNLIKELY(!(expr))) { \
            ::nomad::assertFailed(#expr, msg, __FILE__, __LINE__, __func__); \
        } \
    } while (false)

/// Assertion that always fails (for unreachable code paths)
#define NOMAD_UNREACHABLE() \
    do { \
        ::nomad::assertFailed("UNREACHABLE", "Code path should never be executed", \
                              __FILE__, __LINE__, __func__); \
    } while (false)

/// Assert that code is running on the expected thread
#define NOMAD_ASSERT_THREAD(thread_id) \
    NOMAD_ASSERT_MSG(std::this_thread::get_id() == (thread_id), \
                     "Called from wrong thread")

/// Assert that we're on the audio thread (requires audio thread ID to be set)
#define NOMAD_ASSERT_AUDIO_THREAD() \
    NOMAD_ASSERT_MSG(::nomad::threading::isAudioThread(), \
                     "Must be called from audio thread")

/// Assert that we're NOT on the audio thread
#define NOMAD_ASSERT_NOT_AUDIO_THREAD() \
    NOMAD_ASSERT_MSG(!::nomad::threading::isAudioThread(), \
                     "Must NOT be called from audio thread")

#else // Release builds

#define NOMAD_ASSERT(expr)              ((void)0)
#define NOMAD_ASSERT_MSG(expr, msg)     ((void)0)
#define NOMAD_ASSERT_THREAD(thread_id)  ((void)0)
#define NOMAD_ASSERT_AUDIO_THREAD()     ((void)0)
#define NOMAD_ASSERT_NOT_AUDIO_THREAD() ((void)0)

// Portable NOMAD_UNREACHABLE() for release builds
// Tells the compiler this code path is never reached, enabling optimizations
#if defined(_MSC_VER)
    #define NOMAD_UNREACHABLE() __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
    #define NOMAD_UNREACHABLE() __builtin_unreachable()
#else
    #define NOMAD_UNREACHABLE() ((void)0)
#endif

#endif // NOMAD_BUILD_DEBUG

//=============================================================================
// Runtime Checks (Always Enabled)
//=============================================================================

/// Verify condition - always checked, even in release builds
#define NOMAD_VERIFY(expr) \
    do { \
        if (NOMAD_UNLIKELY(!(expr))) { \
            ::nomad::assertFailed(#expr, "Runtime verification failed", \
                                  __FILE__, __LINE__, __func__); \
        } \
    } while (false)

/// Verify with custom message
#define NOMAD_VERIFY_MSG(expr, msg) \
    do { \
        if (NOMAD_UNLIKELY(!(expr))) { \
            ::nomad::assertFailed(#expr, msg, __FILE__, __LINE__, __func__); \
        } \
    } while (false)

//=============================================================================
// Static Assertions
//=============================================================================

#define NOMAD_STATIC_ASSERT(expr, msg) static_assert(expr, msg)

#define NOMAD_ASSERT_SIZE(type, expected_size) \
    NOMAD_STATIC_ASSERT(sizeof(type) == expected_size, \
                        "Type " #type " has unexpected size")

#define NOMAD_ASSERT_ALIGNMENT(type, expected_alignment) \
    NOMAD_STATIC_ASSERT(alignof(type) == expected_alignment, \
                        "Type " #type " has unexpected alignment")

#define NOMAD_ASSERT_TRIVIALLY_COPYABLE(type) \
    NOMAD_STATIC_ASSERT(std::is_trivially_copyable_v<type>, \
                        "Type " #type " must be trivially copyable")

#define NOMAD_ASSERT_STANDARD_LAYOUT(type) \
    NOMAD_STATIC_ASSERT(std::is_standard_layout_v<type>, \
                        "Type " #type " must be standard layout")

//=============================================================================
// Debug Utilities
//=============================================================================

#ifdef NOMAD_BUILD_DEBUG

#if defined(NOMAD_COMPILER_MSVC)
    #define NOMAD_DEBUG_BREAK() __debugbreak()
#elif defined(NOMAD_COMPILER_CLANG) || defined(NOMAD_COMPILER_GCC)
    #if defined(NOMAD_ARCH_X64) || defined(NOMAD_ARCH_X86)
        #define NOMAD_DEBUG_BREAK() __asm__ volatile("int $0x03")
    #elif defined(NOMAD_ARCH_ARM64)
        #define NOMAD_DEBUG_BREAK() __builtin_trap()
    #else
        #define NOMAD_DEBUG_BREAK() __builtin_trap()
    #endif
#else
    #define NOMAD_DEBUG_BREAK() std::abort()
#endif

#define NOMAD_DEBUG_LOG(msg) \
    std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " - " << msg << std::endl

#define NOMAD_DEBUG_VAR(var) \
    std::cerr << "[DEBUG] " << #var << " = " << (var) << std::endl

#else // Release builds

#define NOMAD_DEBUG_BREAK()   ((void)0)
#define NOMAD_DEBUG_LOG(msg)  ((void)0)
#define NOMAD_DEBUG_VAR(var)  ((void)0)

#endif // NOMAD_BUILD_DEBUG

//=============================================================================
// Precondition & Postcondition Macros
//=============================================================================

#define NOMAD_PRECONDITION(expr) NOMAD_ASSERT_MSG(expr, "Precondition violated")
#define NOMAD_POSTCONDITION(expr) NOMAD_ASSERT_MSG(expr, "Postcondition violated")
#define NOMAD_INVARIANT(expr) NOMAD_ASSERT_MSG(expr, "Invariant violated")
