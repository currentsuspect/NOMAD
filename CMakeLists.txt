cmake_minimum_required(VERSION 3.15)
project(NOMAD VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Main application
juce_add_gui_app(NOMAD
    PRODUCT_NAME "NOMAD"
    COMPANY_NAME "NOMAD Audio"
    BUNDLE_ID "com.nomadaudio.nomad"
    ICON_BIG ""
    ICON_SMALL ""
)

juce_generate_juce_header(NOMAD)

target_sources(NOMAD PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/Audio/AudioEngine.h
    Source/Audio/AudioEngine.cpp
    Source/Audio/TransportController.h
    Source/Audio/TransportController.cpp
    Source/Audio/SequencerEngine.h
    Source/Audio/SequencerEngine.cpp
    Source/Audio/Mixer.h
    Source/Audio/Mixer.cpp
    Source/Audio/MixerChannel.h
    Source/Audio/MixerChannel.cpp
    Source/Audio/EffectsProcessor.h
    Source/Audio/EffectsProcessor.cpp
    Source/Models/Pattern.h
    Source/Models/Pattern.cpp
    Source/Models/PatternManager.h
    Source/Models/PatternManager.cpp
    Source/Models/AudioClip.h
    Source/Models/AudioClip.cpp
    Source/UI/AudioSettingsComponent.h
    Source/UI/AudioSettingsComponent.cpp
    Source/UI/TransportComponent.h
    Source/UI/TransportComponent.cpp
    Source/UI/NomadLookAndFeel.h
    Source/UI/NomadLookAndFeel.cpp
    Source/UI/IconButton.h
    Source/UI/IconButton.cpp
    Source/UI/WindowControlButton.h
    Source/UI/CustomResizer.h
    Source/UI/FileBrowserComponent.h
    Source/UI/FileBrowserComponent.cpp
    Source/UI/PlaylistComponent.h
    Source/UI/PlaylistComponent.cpp
    Source/UI/MinimalScrollbar.h
    Source/UI/MinimalScrollbar.cpp
    Source/UI/SequencerView.h
    Source/UI/SequencerView.cpp
    Source/UI/MixerComponent.h
    Source/UI/MixerComponent.cpp
    Source/UI/FloatingWindow.h
    Source/UI/FloatingWindow.cpp
    Source/UI/ThemeManager.h
    Source/UI/WindowManager.h
    Source/UI/EffectCache.h
)

target_include_directories(NOMAD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(NOMAD PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NOMAD,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:NOMAD,JUCE_VERSION>"
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(NOMAD PRIVATE
        JUCE_ASIO=0
        JUCE_DIRECTSOUND=1
    )
endif()

target_link_libraries(NOMAD PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_opengl
    juce::juce_dsp
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Test executable
juce_add_console_app(NOMAD_Tests
    PRODUCT_NAME "NOMAD Tests"
)

juce_generate_juce_header(NOMAD_Tests)

target_sources(NOMAD_Tests PRIVATE
    Source/Tests/TestRunner.cpp
    Source/Tests/PatternTests.h
    Source/Models/Pattern.h
    Source/Models/Pattern.cpp
)

target_include_directories(NOMAD_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(NOMAD_Tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(NOMAD_Tests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

# ============================================================================
# MSDF Text Renderer
# ============================================================================

# Find required packages
find_package(OpenGL REQUIRED)

# Find FreeType (Windows-compatible approach)
if(WIN32)
    # Use the FreeType we already have in NomadUI/External/freetype
    set(FREETYPE_FOUND TRUE)
    set(FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/NomadUI/External/freetype/include)
    set(FREETYPE_LIBRARIES freetype)
else()
    # Use pkg-config on Linux/macOS
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
    if(NOT FREETYPE_FOUND)
        message(FATAL_ERROR "FreeType2 is required but not found. Please install libfreetype6-dev (Ubuntu) or freetype-devel (CentOS)")
    endif()
endif()

# Find GLFW (optional for Windows, required for Linux/macOS)
if(WIN32)
    # GLFW is optional on Windows since we use Win32
    find_package(glfw3 QUIET)
    set(GLFW_AVAILABLE ${glfw3_FOUND})
else()
    find_package(glfw3 REQUIRED)
    if(NOT glfw3_FOUND)
        message(FATAL_ERROR "GLFW3 is required but not found. Please install libglfw3-dev (Ubuntu) or glfw-devel (CentOS)")
    endif()
    set(GLFW_AVAILABLE TRUE)
endif()

# ============================================================================
# Simple Math Library (GLM alternative)
# ============================================================================

# Create a simple math library to replace GLM
add_library(SimpleMath INTERFACE)

target_include_directories(SimpleMath INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math
)

# Create simple math headers
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/math/glm.hpp
"#pragma once
// Simple GLM-compatible math library
#include <cmath>

namespace glm {
    struct vec2 { float x, y; vec2(float x=0, float y=0) : x(x), y(y) {} };
    struct vec3 { float x, y, z; vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {} };
    struct vec4 { float x, y, z, w; vec4(float x=0, float y=0, float z=0, float w=0) : x(x), y(y), z(z), w(w) {} };
    struct mat4 { float m[16]; mat4() { for(int i=0; i<16; i++) m[i] = (i%5==0) ? 1.0f : 0.0f; } };
    
    // Basic functions
    inline float length(const vec2& v) { return std::sqrt(v.x*v.x + v.y*v.y); }
    inline vec2 normalize(const vec2& v) { float l = length(v); return l > 0 ? vec2(v.x/l, v.y/l) : vec2(0,0); }
    inline vec2 mix(const vec2& a, const vec2& b, float t) { return vec2(a.x + t*(b.x-a.x), a.y + t*(b.y-a.y)); }
    inline float clamp(float x, float min, float max) { return x < min ? min : (x > max ? max : x); }
}
")

message(STATUS "✓ Simple Math library created")

# ============================================================================
# MSDF Text Renderer Library
# ============================================================================
add_library(MSDFTextRenderer STATIC
    src/text/TextRenderer.h
    src/text/TextRenderer.cpp
    src/text/MSDFGenerator.h
    src/text/MSDFGenerator.cpp
    src/text/AtlasPacker.h
    src/text/AtlasPacker.cpp
)

target_include_directories(MSDFTextRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/NomadUI/External/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/NomadUI/External/freetype/include
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(MSDFTextRenderer PUBLIC
    OpenGL::GL
    ${FREETYPE_LIBRARIES}
    SimpleMath
)

target_compile_definitions(MSDFTextRenderer PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Text Demo (only if GLFW is available)
if(GLFW_AVAILABLE)
    add_executable(TEXT_DEMO
        demo/text_demo.cpp
    )

    target_link_libraries(TEXT_DEMO PRIVATE
        MSDFTextRenderer
        glfw
        ${FREETYPE_LIBRARIES}
    )

    target_include_directories(TEXT_DEMO PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FREETYPE_INCLUDE_DIRS}
    )
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Enable testing
enable_testing()
add_test(NAME PatternTests COMMAND NOMAD_Tests)

# Mixer Test Application
juce_add_gui_app(MixerTest
    PRODUCT_NAME "NOMAD Mixer Test"
    COMPANY_NAME "NOMAD Audio"
    BUNDLE_ID "com.nomadaudio.nomad.mixertest"
)

juce_generate_juce_header(MixerTest)

target_sources(MixerTest PRIVATE
    Source/Tests/MixerTest.h
    Source/Tests/MixerTestApp.h
    Source/Tests/MixerTestApp.cpp
    Source/Audio/Mixer.h
    Source/Audio/Mixer.cpp
    Source/Audio/MixerChannel.h
    Source/Audio/MixerChannel.cpp
    Source/Audio/EffectsProcessor.h
    Source/Audio/EffectsProcessor.cpp
)

target_include_directories(MixerTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(MixerTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MixerTest,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MixerTest,JUCE_VERSION>"
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(MixerTest PRIVATE
        JUCE_ASIO=0
        JUCE_DIRECTSOUND=1
    )
endif()

target_link_libraries(MixerTest PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_data_structures
    juce::juce_events
    juce::juce_core
    juce::juce_dsp
)
