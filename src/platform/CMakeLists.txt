# Platform Module CMakeLists.txt
# OS abstraction layer - interfaces and platform-specific implementations

# Platform interface library (headers only)
add_library(nomad_platform_interface INTERFACE)

target_include_directories(nomad_platform_interface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nomad_platform_interface INTERFACE
    nomad_core
)

target_sources(nomad_platform_interface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/interface/platform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interface/window.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interface/audio_device.hpp
)

# Platform implementation library
add_library(nomad_platform STATIC)

target_link_libraries(nomad_platform
    PUBLIC nomad_platform_interface
    PUBLIC nomad_core
)

# Platform-specific source files
if(WIN32)
    target_sources(nomad_platform PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/windows/platform_win.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/windows/window_win.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/windows/audio_device_win.cpp
    )
    target_compile_definitions(nomad_platform PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_link_libraries(nomad_platform PRIVATE
        dwmapi
        bcrypt
    )
elseif(APPLE)
    target_sources(nomad_platform PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/macos/platform_mac.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/macos/window_mac.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/macos/audio_device_mac.mm
    )
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    target_link_libraries(nomad_platform PRIVATE
        ${COCOA_FRAMEWORK}
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    )
elseif(UNIX)
    target_sources(nomad_platform PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/linux/platform_linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/linux/window_linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/linux/audio_device_linux.cpp
    )
    find_package(X11 REQUIRED)
    find_package(ALSA)
    target_link_libraries(nomad_platform PRIVATE
        ${X11_LIBRARIES}
        pthread
    )
    if(ALSA_FOUND)
        target_link_libraries(nomad_platform PRIVATE ALSA::ALSA)
        target_compile_definitions(nomad_platform PRIVATE NOMAD_HAS_ALSA)
    endif()
endif()

# Audio subsystem (separate for audio thread management)
add_library(nomad_audio STATIC)

target_sources(nomad_audio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_thread.hpp
)

target_link_libraries(nomad_audio
    PUBLIC nomad_platform_interface
    PUBLIC nomad_core
)

# Aliases
add_library(Nomad::Platform ALIAS nomad_platform)
add_library(Nomad::PlatformInterface ALIAS nomad_platform_interface)
add_library(Nomad::Audio ALIAS nomad_audio)
