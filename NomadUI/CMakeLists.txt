cmake_minimum_required(VERSION 3.15)
project(NomadUI VERSION 0.1.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Options
# ============================================================================

option(NOMADUI_BUILD_EXAMPLES "Build example applications" ON)
option(NOMADUI_BUILD_OPENGL "Build OpenGL renderer" ON)
option(NOMADUI_BUILD_VULKAN "Build Vulkan renderer" OFF)

# ============================================================================
# Core Library
# ============================================================================

set(NOMADUI_CORE_SOURCES
    Core/NUITypes.h
    Core/NUIComponent.h
    Core/NUIComponent.cpp
    Core/NUITheme.h
    Core/NUITheme.cpp
    # Core/NUIApp.h  # Requires renderer
    # Core/NUIApp.cpp
)

set(NOMADUI_GRAPHICS_SOURCES
    Graphics/NUIRenderer.h
    Graphics/NUIRenderer.cpp
)

# Text rendering removed - using external MSDF text renderer

# Create core library
add_library(NomadUI_Core STATIC
    ${NOMADUI_CORE_SOURCES}
    ${NOMADUI_GRAPHICS_SOURCES}
)

target_include_directories(NomadUI_Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# FreeType dependency removed - handled by MSDF text renderer

# ============================================================================
# GLAD - OpenGL Loader
# ============================================================================

set(GLAD_SOURCES
    External/glad/src/glad.c
    External/glad/include/glad/glad.h
)

add_library(glad STATIC ${GLAD_SOURCES})
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_include_directories(glad PUBLIC External/glad/include)

# ============================================================================
# FreeType - Text Rendering
# ============================================================================

include(FetchContent)

message(STATUS "Fetching FreeType...")

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2  # Latest stable version
    GIT_SHALLOW TRUE
)

# Disable unnecessary FreeType features
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(freetype)

message(STATUS "✓ FreeType loaded")

# ============================================================================
# OpenGL Renderer (Optional)
# ============================================================================

if(NOMADUI_BUILD_OPENGL)
    find_package(OpenGL REQUIRED)
    
    set(NOMADUI_OPENGL_SOURCES
        Graphics/OpenGL/NUIRendererGL.h
        Graphics/OpenGL/NUIRendererGL.cpp
    )
    
    add_library(NomadUI_OpenGL STATIC
        ${NOMADUI_OPENGL_SOURCES}
    )
    
    target_link_libraries(NomadUI_OpenGL PUBLIC
        NomadUI_Core
        OpenGL::GL
        glad
    )
    
    target_compile_definitions(NomadUI_OpenGL PUBLIC
        NOMADUI_OPENGL
    )
    
    target_include_directories(NomadUI_OpenGL PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# ============================================================================
# Vulkan Renderer (Optional)
# ============================================================================

if(NOMADUI_BUILD_VULKAN)
    find_package(Vulkan REQUIRED)
    
    set(NOMADUI_VULKAN_SOURCES
        Graphics/Vulkan/NUIRendererVK.h
        Graphics/Vulkan/NUIRendererVK.cpp
    )
    
    add_library(NomadUI_Vulkan STATIC
        ${NOMADUI_VULKAN_SOURCES}
    )
    
    target_link_libraries(NomadUI_Vulkan PUBLIC
        NomadUI_Core
        Vulkan::Vulkan
    )
    
    target_include_directories(NomadUI_Vulkan PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# ============================================================================
# Platform Layer
# ============================================================================

if(WIN32)
    # Ensure 64-bit build
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Nomad UI requires 64-bit Windows (x64)")
    endif()
    
    set(NOMADUI_PLATFORM_SOURCES
        Platform/Windows/NUIPlatformWindows.h
        # Platform/Windows/NUIPlatformWindows.cpp  # TODO: Implement
    )
    set(PLATFORM_NAME "Windows x64")
elseif(APPLE)
    # Universal binary (ARM64 + x64)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    
    set(NOMADUI_PLATFORM_SOURCES
        Platform/macOS/NUIPlatformMacOS.h
        # Platform/macOS/NUIPlatformMacOS.mm  # TODO: Implement
    )
    set(PLATFORM_NAME "macOS Universal (ARM64 + x64)")
elseif(UNIX)
    # Ensure 64-bit build
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Nomad UI requires 64-bit Linux (x64)")
    endif()
    
    set(NOMADUI_PLATFORM_SOURCES
        Platform/Linux/NUIPlatformLinux.h
        # Platform/Linux/NUIPlatformLinux.cpp  # TODO: Implement
    )
    set(PLATFORM_NAME "Linux x64")
endif()

add_library(NomadUI_Platform STATIC
    ${NOMADUI_PLATFORM_SOURCES}
)

target_link_libraries(NomadUI_Platform PUBLIC
    NomadUI_Core
)

# ============================================================================
# Main Library (Combines all modules)
# ============================================================================

add_library(NomadUI INTERFACE)

target_link_libraries(NomadUI INTERFACE
    NomadUI_Core
    NomadUI_Platform
)

if(NOMADUI_BUILD_OPENGL)
    target_link_libraries(NomadUI INTERFACE NomadUI_OpenGL)
endif()

if(NOMADUI_BUILD_VULKAN)
    target_link_libraries(NomadUI INTERFACE NomadUI_Vulkan)
endif()

# ============================================================================
# Examples
# ============================================================================

if(NOMADUI_BUILD_EXAMPLES)
    # Minimal Core Test (no OpenGL/Platform required)
    add_executable(NomadUI_MinimalTest
        Test/MinimalTest.cpp
    )
    
    target_link_libraries(NomadUI_MinimalTest PRIVATE
        NomadUI_Core
    )
    
    set_target_properties(NomadUI_MinimalTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # Window Demo (Windows only, requires OpenGL + Win32)
    if(WIN32 AND NOMADUI_BUILD_OPENGL)
        add_executable(NomadUI_WindowDemo
            Examples/WindowDemo.cpp
            Platform/Windows/NUIWindowWin32.cpp
            Core/NUIComponent.cpp
            Core/NUITheme.cpp
            Graphics/NUIRenderer.cpp
            Graphics/OpenGL/NUIRendererGL.cpp
        )
        
        # Add glad.c as a separate C file
        target_sources(NomadUI_WindowDemo PRIVATE External/glad/src/glad.c)
        set_source_files_properties(External/glad/src/glad.c PROPERTIES LANGUAGE C)

        target_include_directories(NomadUI_WindowDemo PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/External/glad/include
        )

        target_link_libraries(NomadUI_WindowDemo PRIVATE 
            opengl32
        )

        set_target_properties(NomadUI_WindowDemo PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS NomadUI_Core NomadUI_Platform
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY Core/ Graphics/ Platform/
    DESTINATION include/NomadUI
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "=================================")
message(STATUS "  Nomad UI Framework")
message(STATUS "=================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Examples: ${NOMADUI_BUILD_EXAMPLES}")
message(STATUS "  OpenGL:   ${NOMADUI_BUILD_OPENGL}")
message(STATUS "  Vulkan:   ${NOMADUI_BUILD_VULKAN}")
message(STATUS "")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Architecture: ${CMAKE_SIZEOF_VOID_P}-bit")
message(STATUS "=================================")
message(STATUS "")
