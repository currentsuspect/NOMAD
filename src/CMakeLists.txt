# Nomad DAW - New Architecture Build System
# This is the new modular CMake configuration for the refactored architecture
#
# Module Dependency Rules (Enforced):
#   Core       → (no dependencies - foundation)
#   Platform   → Core
#   DSP        → Core
#   Audio      → Core, DSP, Platform
#   MIDI       → Core
#   Automation → Core
#   Plugins    → Core, Audio
#   Export     → Core, Audio
#   Project    → Core, Audio, Platform
#   UI         → Core, Platform
#   App        → All modules

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Project Configuration
# ============================================================================

project(NomadModular
    VERSION 0.1.0
    DESCRIPTION "Nomad DAW - Modular Architecture"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# ============================================================================
# Global Options
# ============================================================================

option(NOMAD_BUILD_TESTS "Build unit tests" ON)
option(NOMAD_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(NOMAD_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(NOMAD_ENABLE_TRACY "Enable Tracy profiler" OFF)
option(NOMAD_ENABLE_ASAN "Enable AddressSanitizer" OFF)

# ============================================================================
# Compiler Configuration
# ============================================================================

# Common warnings
if(MSVC)
    add_compile_options(
        /W4         # High warning level
        /WX         # Warnings as errors
        /permissive- # Strict conformance
        /Zc:__cplusplus # Report correct C++ version
        /utf-8      # UTF-8 source and execution charset
    )
    
    # SIMD flags for MSVC
    if(NOMAD_ENABLE_SIMD)
        add_compile_options(/arch:AVX2)
    endif()
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-unused-parameter
    )
    
    # SIMD flags for GCC/Clang
    if(NOMAD_ENABLE_SIMD)
        add_compile_options(-mavx2 -mfma)
    endif()
endif()

# Debug/Release specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(NOMAD_DEBUG=1)
    if(NOMAD_ENABLE_ASAN)
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
else()
    add_compile_definitions(NOMAD_RELEASE=1)
    if(MSVC)
        add_compile_options(/O2 /GL)
    else()
        add_compile_options(-O3 -flto)
    endif()
endif()

# ============================================================================
# Include Directories
# ============================================================================

# Root source directory for module includes
set(NOMAD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# ============================================================================
# Core Module (Foundation - No Dependencies)
# ============================================================================

add_library(nomad_core INTERFACE)
target_include_directories(nomad_core INTERFACE ${NOMAD_SRC_DIR})

# Core module contains header-only utilities:
# - base/types.hpp
# - base/config.hpp
# - base/assert.hpp
# - base/log.hpp
# - memory/allocator.hpp
# - memory/pool.hpp
# - memory/arena.hpp
# - threading/atomic.hpp
# - threading/spin_lock.hpp
# - threading/lock_free_queue.hpp
# - threading/thread_pool.hpp
# - math/simd.hpp
# - math/vector.hpp
# - data/uuid.hpp
# - serialization/json.hpp

# ============================================================================
# Platform Module (Depends: Core)
# ============================================================================

add_library(nomad_platform INTERFACE)
target_link_libraries(nomad_platform INTERFACE nomad_core)
target_include_directories(nomad_platform INTERFACE ${NOMAD_SRC_DIR})

# Platform-specific implementations will be added here

# ============================================================================
# DSP Module (Depends: Core)
# ============================================================================

add_library(nomad_dsp INTERFACE)
target_link_libraries(nomad_dsp INTERFACE nomad_core)
target_include_directories(nomad_dsp INTERFACE ${NOMAD_SRC_DIR})

# DSP module contains:
# - processors/filter.hpp
# - util/buffer.hpp
# - (more to be added)

# ============================================================================
# Audio Module (Depends: Core, DSP, Platform)
# ============================================================================

add_library(nomad_audio INTERFACE)
target_link_libraries(nomad_audio INTERFACE 
    nomad_core
    nomad_dsp
    nomad_platform
)
target_include_directories(nomad_audio INTERFACE ${NOMAD_SRC_DIR})

# Audio module contains:
# - engine/audio_thread.hpp
# - (more to be added)

# ============================================================================
# Aggregate Library (All Modules)
# ============================================================================

add_library(nomad_all INTERFACE)
target_link_libraries(nomad_all INTERFACE
    nomad_core
    nomad_platform
    nomad_dsp
    nomad_audio
)

# ============================================================================
# Tests
# ============================================================================

if(NOMAD_BUILD_TESTS)
    enable_testing()
    
    # Find or fetch Catch2
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
    
    # Test executable
    add_executable(nomad_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unit/core_tests/test_types.cpp
    )
    
    target_link_libraries(nomad_tests PRIVATE
        nomad_all
        Catch2::Catch2WithMain
    )
    
    # Register tests with CTest
    include(Catch)
    catch_discover_tests(nomad_tests)
endif()

# ============================================================================
# Benchmarks
# ============================================================================

if(NOMAD_BUILD_BENCHMARKS)
    # Benchmark configuration will be added here
endif()

# ============================================================================
# Installation
# ============================================================================

# Installation rules will be added here

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "Nomad DAW - Modular Architecture")
message(STATUS "================================")
message(STATUS "Version:      ${PROJECT_VERSION}")
message(STATUS "Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Tests:      ${NOMAD_BUILD_TESTS}")
message(STATUS "  Benchmarks: ${NOMAD_BUILD_BENCHMARKS}")
message(STATUS "  SIMD:       ${NOMAD_ENABLE_SIMD}")
message(STATUS "  Tracy:      ${NOMAD_ENABLE_TRACY}")
message(STATUS "  ASAN:       ${NOMAD_ENABLE_ASAN}")
message(STATUS "")
