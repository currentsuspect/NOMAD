cmake_minimum_required(VERSION 3.22)

project(NomadAudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional ASIO SDK path (not committed to repo). If provided, RtAudio will be
# built/compiled with ASIO support by defining __WINDOWS_ASIO__ and adding the
# SDK include path. The ASIO SDK is proprietary and should not be checked into
# the repository. Example: -DDASIO_SDK_ROOT="C:/ASIOSDK"
option(ASIO_SDK_ROOT "Path to ASIO SDK root directory (optional). Defaults to NomadAudio/External/ASIOSDK if present." "${CMAKE_CURRENT_SOURCE_DIR}/External/ASIOSDK")

if(ASIO_SDK_ROOT)
    message(STATUS "ASIO SDK root provided: ${ASIO_SDK_ROOT}")

    # Try to detect common SDK layouts and pick the include folder containing asio.h
    set(ASIO_INC_DIR "${ASIO_SDK_ROOT}")
    if(EXISTS "${ASIO_SDK_ROOT}/asio.h")
        set(ASIO_INC_DIR "${ASIO_SDK_ROOT}")
    elseif(EXISTS "${ASIO_SDK_ROOT}/include/asio.h")
        set(ASIO_INC_DIR "${ASIO_SDK_ROOT}/include")
    elseif(EXISTS "${ASIO_SDK_ROOT}/ASIOSDK/asio.h")
        set(ASIO_INC_DIR "${ASIO_SDK_ROOT}/ASIOSDK")
    elseif(EXISTS "${ASIO_SDK_ROOT}/ASIOSDK/include/asio.h")
        set(ASIO_INC_DIR "${ASIO_SDK_ROOT}/ASIOSDK/include")
    elseif(EXISTS "${ASIO_SDK_ROOT}/ASIOSDK/common/asio.h")
        set(ASIO_INC_DIR "${ASIO_SDK_ROOT}/ASIOSDK/common")
    elseif(EXISTS "${ASIO_SDK_ROOT}/common/asio.h")
        set(ASIO_INC_DIR "${ASIO_SDK_ROOT}/common")
    endif()

    # Verify required ASIO headers are present (asio.h and asiodrivers.h).
    set(ASIO_HEADERS_OK FALSE)

    # Check for asiodrivers.h anywhere inside the SDK root (common SDK layouts put it under 'host' or similar).
    file(GLOB_RECURSE ASIO_DRIVER_HDRS RELATIVE "${ASIO_SDK_ROOT}" "${ASIO_SDK_ROOT}/*/asiodrivers.h" "${ASIO_SDK_ROOT}/**/asiodrivers.h")

    set(ASIO_DRIVER_DIR "")
    if(ASIO_DRIVER_HDRS)
        list(GET ASIO_DRIVER_HDRS 0 ASIO_DRIVER_PATH)
        get_filename_component(ASIO_DRIVER_DIR "${ASIO_SDK_ROOT}/${ASIO_DRIVER_PATH}" DIRECTORY)
    endif()

    if(EXISTS "${ASIO_INC_DIR}/asio.h" AND ASIO_DRIVER_DIR AND EXISTS "${ASIO_DRIVER_DIR}/asiodrivers.h")
        set(ASIO_HEADERS_OK TRUE)
    endif()

    if(ASIO_HEADERS_OK)
        # Add both the primary include dir and the driver header dir (they may differ)
        include_directories(${ASIO_INC_DIR} ${ASIO_DRIVER_DIR})
        message(STATUS "ASIO include dir: ${ASIO_INC_DIR}")
        message(STATUS "ASIO driver header dir: ${ASIO_DRIVER_DIR}")
        add_definitions(-D__WINDOWS_ASIO__)
    else()
        message(WARNING "ASIO SDK found at ${ASIO_SDK_ROOT} but required headers (asio.h + asiodrivers.h) were not both located under the detected include paths. ASIO support will be disabled.")
    endif()
else()
    message(STATUS "ASIO SDK not provided. ASIO support will be disabled in RtAudio unless configured externally.")
endif()

# =============================================================================
# RtAudio Integration (MIT License)
# =============================================================================

# RtAudio source files
set(RTAUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External/rtaudio)

set(RTAUDIO_SOURCES
    ${RTAUDIO_DIR}/RtAudio.cpp
)

set(RTAUDIO_HEADERS
    ${RTAUDIO_DIR}/RtAudio.h
)

# If the ASIO SDK was detected above (we set ASIO_INC_DIR), include the
# IASIO thiscall resolver source so RtAudio's ASIO path can compile on
# non-MSVC compilers. The resolver is only required when asio.h is present.
if(DEFINED ASIO_INC_DIR AND ASIO_HEADERS_OK)
    list(APPEND RTAUDIO_SOURCES
        ${RTAUDIO_DIR}/iasiothiscallresolver.cpp
    )
    list(APPEND RTAUDIO_HEADERS
        ${RTAUDIO_DIR}/iasiothiscallresolver.h
    )
    message(STATUS "Including IASIO thiscall resolver in build (ASIO SDK detected)")
endif()

# Platform-specific audio APIs
if(WIN32)
    # Windows: WASAPI (primary), DirectSound (fallback)
    add_definitions(-D__WINDOWS_WASAPI__)
    set(RTAUDIO_LIBS dsound ole32 winmm ksuser mfplat mfuuid wmcodecdspuuid)
    
    # Optional ASIO support (requires ASIO SDK)
    # add_definitions(-D__WINDOWS_ASIO__)
    
elseif(APPLE)
    # macOS: CoreAudio
    add_definitions(-D__MACOSX_CORE__)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    set(RTAUDIO_LIBS ${COREAUDIO_LIBRARY} ${COREFOUNDATION_LIBRARY})
    
elseif(UNIX)
    # Linux: ALSA (primary), JACK (optional), PulseAudio (optional)
    add_definitions(-D__LINUX_ALSA__)
    find_package(ALSA REQUIRED)
    set(RTAUDIO_LIBS ${ALSA_LIBRARIES} pthread)
    
    # Optional JACK support
    # add_definitions(-D__UNIX_JACK__)
    # find_package(Jack)
    # if(JACK_FOUND)
    #     set(RTAUDIO_LIBS ${RTAUDIO_LIBS} ${JACK_LIBRARIES})
    # endif()
endif()

# =============================================================================
# NomadAudio Library
# =============================================================================

set(NOMAD_AUDIO_SOURCES
    src/AudioDeviceManager.cpp
    src/NomadASIOBackend.cpp
    src/RtAudioBackend.cpp
    src/AudioProcessor.cpp
    src/MixerBus.cpp
    src/Oscillator.cpp
)

set(NOMAD_AUDIO_HEADERS
    include/NomadAudio.h
    include/AudioDeviceManager.h
    include/AudioDriver.h
    src/NomadASIOBackend.h
    include/AudioProcessor.h
    include/MixerBus.h
    include/Oscillator.h
)

# Create NomadAudio library
add_library(NomadAudio STATIC
    ${NOMAD_AUDIO_SOURCES}
    ${NOMAD_AUDIO_HEADERS}
    ${RTAUDIO_SOURCES}
    ${RTAUDIO_HEADERS}
)

target_include_directories(NomadAudio
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${RTAUDIO_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(NomadAudio
    PUBLIC
        NomadCore
    PRIVATE
        ${RTAUDIO_LIBS}
)

# =============================================================================
# Test Applications
# =============================================================================

# Basic audio test
add_executable(NomadAudioTest
    test/AudioTest.cpp
)

target_link_libraries(NomadAudioTest
    PRIVATE
        NomadAudio
        NomadCore
)

# Device manager test
add_executable(NomadDeviceManagerTest
    test/DeviceManagerTest.cpp
)

target_link_libraries(NomadDeviceManagerTest
    PRIVATE
        NomadAudio
        NomadCore
)

# Audio callback test
add_executable(NomadAudioCallbackTest
    test/AudioCallbackTest.cpp
)

target_link_libraries(NomadAudioCallbackTest
    PRIVATE
        NomadAudio
        NomadCore
)

# Mixer bus test
add_executable(NomadMixerBusTest
    test/MixerBusTest.cpp
)

target_link_libraries(NomadMixerBusTest
    PRIVATE
        NomadAudio
        NomadCore
)

# Oscillator test
add_executable(NomadOscillatorTest
    test/OscillatorTest.cpp
)

target_link_libraries(NomadOscillatorTest
    PRIVATE
        NomadAudio
        NomadCore
)

# =============================================================================
# Status
# =============================================================================

message(STATUS "")
message(STATUS "=================================")
message(STATUS "  NomadAudio v${PROJECT_VERSION}")
message(STATUS "=================================")
message(STATUS "RtAudio: Integrated (MIT)")
if(WIN32)
    message(STATUS "Audio API: WASAPI")
elseif(APPLE)
    message(STATUS "Audio API: CoreAudio")
elseif(UNIX)
    message(STATUS "Audio API: ALSA")
endif()
message(STATUS "=================================")
message(STATUS "")
