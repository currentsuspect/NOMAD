/**
 * @file config.hpp
 * @brief Build configuration and feature flags for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides compile-time configuration options and feature flags.
 * It should be included early in the include chain to configure behavior.
 */

#pragma once

#if defined(__has_include)
    #if __has_include("types.hpp")
        #include "types.hpp"
    #else
        #include <cstdint>
        #include <cstddef>

        // Minimal fallback type definitions used by config.hpp when types.hpp
        // is not available (e.g. editor/configuration issues). These mirror the
        // common definitions expected by the rest of the codebase.
        using u8 = std::uint8_t;
        using u16 = std::uint16_t;
        using u32 = std::uint32_t;
        using u64 = std::uint64_t;
        using s32 = std::int32_t;
        using usize = std::size_t;
        using f32 = float;

        // ChannelCount is used in this header; choose a sensible integer alias.
        using ChannelCount = u32;
    #endif
#else
    #include "types.hpp"
#endif

//=============================================================================
// Platform Detection
//=============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define NOMAD_PLATFORM_WINDOWS 1
    #define NOMAD_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define NOMAD_PLATFORM_MACOS 1
        #define NOMAD_PLATFORM_NAME "macOS"
    #endif
#elif defined(__linux__)
    #define NOMAD_PLATFORM_LINUX 1
    #define NOMAD_PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif

//=============================================================================
// Compiler Detection
//=============================================================================

#if defined(_MSC_VER)
    #define NOMAD_COMPILER_MSVC 1
    #define NOMAD_COMPILER_NAME "MSVC"
    #define NOMAD_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
    #define NOMAD_COMPILER_CLANG 1
    #define NOMAD_COMPILER_NAME "Clang"
    #define NOMAD_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
    #define NOMAD_COMPILER_GCC 1
    #define NOMAD_COMPILER_NAME "GCC"
    #define NOMAD_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
    #error "Unsupported compiler"
#endif

//=============================================================================
// Architecture Detection
//=============================================================================

#if defined(_M_X64) || defined(__x86_64__)
    #define NOMAD_ARCH_X64 1
    #define NOMAD_ARCH_NAME "x64"
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define NOMAD_ARCH_ARM64 1
    #define NOMAD_ARCH_NAME "ARM64"
#elif defined(_M_IX86) || defined(__i386__)
    #define NOMAD_ARCH_X86 1
    #define NOMAD_ARCH_NAME "x86"
#else
    #error "Unsupported architecture"
#endif

//=============================================================================
// Build Configuration
//=============================================================================

#if defined(NOMAD_DEBUG) || defined(_DEBUG) || defined(DEBUG)
    #define NOMAD_BUILD_DEBUG 1
    #define NOMAD_BUILD_NAME "Debug"
#elif defined(NOMAD_RELEASE)
    #define NOMAD_BUILD_RELEASE 1
    #define NOMAD_BUILD_NAME "Release"
#else
    // Default to release in unknown configurations
    #define NOMAD_BUILD_RELEASE 1
    #define NOMAD_BUILD_NAME "Release"
#endif

//=============================================================================
// Feature Flags
//=============================================================================

namespace nomad::config {

//-----------------------------------------------------------------------------
// Version Information
//-----------------------------------------------------------------------------

inline constexpr u32 kVersionMajor = 0;
inline constexpr u32 kVersionMinor = 1;
inline constexpr u32 kVersionPatch = 0;
inline constexpr const char* kVersionString = "0.1.0-alpha";
inline constexpr const char* kVersionCodename = "Foundation";

//-----------------------------------------------------------------------------
// Audio Configuration
//-----------------------------------------------------------------------------

/// Maximum number of audio channels supported
inline constexpr ChannelCount kMaxAudioChannels = 128;

/// Maximum number of tracks
inline constexpr u32 kMaxTracks = 1000;

/// Maximum number of plugins per track
inline constexpr u32 kMaxPluginsPerTrack = 32;

/// Maximum automation points per parameter
inline constexpr u32 kMaxAutomationPoints = 100000;

/// Audio processing block alignment (for SIMD)
inline constexpr usize kAudioBlockAlignment = 64;

//-----------------------------------------------------------------------------
// MIDI Configuration
//-----------------------------------------------------------------------------

/// Maximum MIDI events per buffer
inline constexpr u32 kMaxMidiEventsPerBuffer = 4096;

/// Maximum MIDI ports
inline constexpr u32 kMaxMidiPorts = 32;

//-----------------------------------------------------------------------------
// UI Configuration
//-----------------------------------------------------------------------------

/// Target frame rate for UI rendering
inline constexpr f32 kTargetFrameRate = 60.0f;

/// Maximum number of undo steps
inline constexpr u32 kMaxUndoSteps = 1000;

//-----------------------------------------------------------------------------
// Memory Configuration
//-----------------------------------------------------------------------------

/// Default memory pool size for real-time allocator (32 MB)
inline constexpr usize kDefaultRTPoolSize = 32 * 1024 * 1024;

/// Maximum sample pool size (2 GB)
inline constexpr usize kMaxSamplePoolSize = 2ULL * 1024 * 1024 * 1024;

//-----------------------------------------------------------------------------
// Threading Configuration
//-----------------------------------------------------------------------------

/// Enable thread safety checks in debug builds
#ifdef NOMAD_BUILD_DEBUG
inline constexpr bool kEnableThreadSafetyChecks = true;
#else
inline constexpr bool kEnableThreadSafetyChecks = false;
#endif

/// Enable performance profiling
#ifndef NOMAD_ENABLE_PROFILING
    #ifdef NOMAD_BUILD_DEBUG
        #define NOMAD_ENABLE_PROFILING 1
    #else
        #define NOMAD_ENABLE_PROFILING 0
    #endif
#endif

//-----------------------------------------------------------------------------
// Feature Toggles
//-----------------------------------------------------------------------------

/// Enable VST3 plugin support
#ifndef NOMAD_ENABLE_VST3
    #define NOMAD_ENABLE_VST3 1
#endif

/// Enable CLAP plugin support
#ifndef NOMAD_ENABLE_CLAP
    #define NOMAD_ENABLE_CLAP 1
#endif

/// Enable Audio Units support (macOS only)
#ifndef NOMAD_ENABLE_AU
    #ifdef NOMAD_PLATFORM_MACOS
        #define NOMAD_ENABLE_AU 1
    #else
        #define NOMAD_ENABLE_AU 0
    #endif
#endif

/// Enable SIMD optimizations
#ifndef NOMAD_ENABLE_SIMD
    #define NOMAD_ENABLE_SIMD 1
#endif

/// Enable Tracy profiler integration
#ifndef NOMAD_ENABLE_TRACY
    #ifdef NOMAD_BUILD_DEBUG
        #define NOMAD_ENABLE_TRACY 1
    #else
        #define NOMAD_ENABLE_TRACY 0
    #endif
#endif

} // namespace nomad::config

//=============================================================================
// Compiler Attributes & Macros
//=============================================================================

// Force inline
#if defined(NOMAD_COMPILER_MSVC)
    #define NOMAD_FORCE_INLINE __forceinline
#else
    #define NOMAD_FORCE_INLINE __attribute__((always_inline)) inline
#endif

// No inline
#if defined(NOMAD_COMPILER_MSVC)
    #define NOMAD_NO_INLINE __declspec(noinline)
#else
    #define NOMAD_NO_INLINE __attribute__((noinline))
#endif

// Export/Import for shared libraries
#if defined(NOMAD_PLATFORM_WINDOWS)
    #ifdef NOMAD_BUILDING_SHARED
        #define NOMAD_API __declspec(dllexport)
    #else
        #define NOMAD_API __declspec(dllimport)
    #endif
#else
    #define NOMAD_API __attribute__((visibility("default")))
#endif

// Likely/Unlikely branch hints
#if defined(__cplusplus) && __cplusplus >= 202002L
    #define NOMAD_LIKELY(x)   (x) [[likely]]
    #define NOMAD_UNLIKELY(x) (x) [[unlikely]]
#elif defined(NOMAD_COMPILER_GCC) || defined(NOMAD_COMPILER_CLANG)
    #define NOMAD_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define NOMAD_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define NOMAD_LIKELY(x)   (x)
    #define NOMAD_UNLIKELY(x) (x)
#endif

// Alignment specifier
#define NOMAD_ALIGNAS(x) alignas(x)

// Cache line size (common default)
#define NOMAD_CACHE_LINE_SIZE 64

// Cache-aligned type
#define NOMAD_CACHE_ALIGNED NOMAD_ALIGNAS(NOMAD_CACHE_LINE_SIZE)

// Unused parameter/variable
#define NOMAD_UNUSED(x) (void)(x)

// Stringify macro
#define NOMAD_STRINGIFY_IMPL(x) #x
#define NOMAD_STRINGIFY(x) NOMAD_STRINGIFY_IMPL(x)

// Concatenate macro
#define NOMAD_CONCAT_IMPL(a, b) a##b
#define NOMAD_CONCAT(a, b) NOMAD_CONCAT_IMPL(a, b)
