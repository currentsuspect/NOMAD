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
