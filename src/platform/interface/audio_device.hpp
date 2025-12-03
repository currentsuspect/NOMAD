/**
 * @file audio_device.hpp
 * @brief Audio device abstraction for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file defines the audio device interface for real-time audio I/O.
 * Platform-specific implementations use native APIs:
 * - Windows: WASAPI, ASIO
 * - macOS: CoreAudio
 * - Linux: ALSA, JACK, PulseAudio
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace nomad::platform {

//=============================================================================
// Audio Device Types
//=============================================================================

/**
 * @brief Audio driver/API type
 */
enum class AudioDriverType : u8 {
    Auto,           ///< Let the system choose
    WASAPI,         ///< Windows Audio Session API
    ASIO,           ///< Steinberg ASIO
    CoreAudio,      ///< macOS CoreAudio
    ALSA,           ///< Linux ALSA
    JACK,           ///< JACK Audio Connection Kit
    PulseAudio      ///< PulseAudio (Linux)
};

/**
 * @brief Audio device information
 */
struct AudioDeviceInfo {
    std::string id;                  ///< Unique device identifier
    std::string name;                ///< Human-readable name
    AudioDriverType driver = AudioDriverType::Auto;
    u32 maxInputChannels = 0;
    u32 maxOutputChannels = 0;
    std::vector<u32> sampleRates;    ///< Supported sample rates
    std::vector<u32> bufferSizes;    ///< Supported buffer sizes
    u32 defaultSampleRate = 44100;
    u32 defaultBufferSize = 256;
    bool isDefault = false;
};

/**
 * @brief Audio stream configuration
 */
struct AudioStreamConfig {
    std::string inputDeviceId;       ///< Empty = no input
    std::string outputDeviceId;      ///< Required
    u32 sampleRate = 44100;
    u32 bufferSize = 256;
    u32 numInputChannels = 2;
    u32 numOutputChannels = 2;
    AudioDriverType preferredDriver = AudioDriverType::Auto;
};

/**
 * @brief Audio stream state
 */
enum class AudioStreamState : u8 {
    Closed,
    Stopped,
    Running,
    Error
};

/**
 * @brief Audio callback function type
 * 
 * @param inputBuffer Interleaved input samples (may be nullptr)
 * @param outputBuffer Interleaved output samples (must write to this)
 * @param numFrames Number of frames to process
 * @param inputChannels Number of input channels
 * @param outputChannels Number of output channels
 * @param userData User-provided context
 * 
 * @rt-safety This callback is called from the audio thread and must be RT-safe.
 */
using AudioDeviceCallback = void(*)(
    const f32* inputBuffer,
    f32* outputBuffer,
    u32 numFrames,
    u32 inputChannels,
    u32 outputChannels,
    void* userData
);

/**
 * @brief Callback for device changes (connect/disconnect)
 */
using DeviceChangeCallback = std::function<void()>;

/**
 * @brief Callback for xruns (buffer underrun/overrun)
 */
using XrunCallback = std::function<void()>;

//=============================================================================
// Audio Device Interface
//=============================================================================

/**
 * @brief Audio device manager interface
 * 
 * Provides enumeration and management of audio devices,
 * and audio stream creation.
 */
class IAudioDevice {
public:
    virtual ~IAudioDevice() = default;
    
    //=========================================================================
    // Device Enumeration
    //=========================================================================
    
    /**
     * @brief Get list of available audio drivers
     */
    [[nodiscard]] virtual std::vector<AudioDriverType> getAvailableDrivers() const = 0;
    
    /**
     * @brief Get list of available input devices
     * @param driver Driver type (Auto = all drivers)
     */
    [[nodiscard]] virtual std::vector<AudioDeviceInfo> getInputDevices(
        AudioDriverType driver = AudioDriverType::Auto
    ) const = 0;
    
    /**
     * @brief Get list of available output devices
     * @param driver Driver type (Auto = all drivers)
     */
    [[nodiscard]] virtual std::vector<AudioDeviceInfo> getOutputDevices(
        AudioDriverType driver = AudioDriverType::Auto
    ) const = 0;
    
    /**
     * @brief Get default input device
     */
    [[nodiscard]] virtual AudioDeviceInfo getDefaultInputDevice() const = 0;
    
    /**
     * @brief Get default output device
     */
    [[nodiscard]] virtual AudioDeviceInfo getDefaultOutputDevice() const = 0;
    
    /**
     * @brief Refresh device list
     */
    virtual void refreshDevices() = 0;
    
    //=========================================================================
    // Stream Management
    //=========================================================================
    
    /**
     * @brief Open audio stream
     * @param config Stream configuration
     * @return true if successful
     */
    virtual bool openStream(const AudioStreamConfig& config) = 0;
    
    /**
     * @brief Close audio stream
     */
    virtual void closeStream() = 0;
    
    /**
     * @brief Start audio processing
     * @return true if successful
     */
    virtual bool startStream() = 0;
    
    /**
     * @brief Stop audio processing
     */
    virtual void stopStream() = 0;
    
    /**
     * @brief Get current stream state
     */
    [[nodiscard]] virtual AudioStreamState getStreamState() const = 0;
    
    /**
     * @brief Check if stream is running
     */
    [[nodiscard]] virtual bool isStreamRunning() const = 0;
    
    /**
     * @brief Get current stream configuration
     */
    [[nodiscard]] virtual const AudioStreamConfig& getStreamConfig() const = 0;
    
    //=========================================================================
    // Callbacks
    //=========================================================================
    
    /**
     * @brief Set audio processing callback
     * @param callback Function to call for audio processing
     * @param userData User context passed to callback
     */
    virtual void setCallback(AudioDeviceCallback callback, void* userData) = 0;
    
    /**
     * @brief Set device change notification callback
     */
    virtual void setDeviceChangeCallback(DeviceChangeCallback callback) = 0;
    
    /**
     * @brief Set xrun notification callback
     */
    virtual void setXrunCallback(XrunCallback callback) = 0;
    
    //=========================================================================
    // Statistics
    //=========================================================================
    
    /**
     * @brief Get current CPU load (0.0 - 1.0)
     */
    [[nodiscard]] virtual f32 getCpuLoad() const = 0;
    
    /**
     * @brief Get number of xruns since stream started
     */
    [[nodiscard]] virtual u64 getXrunCount() const = 0;
    
    /**
     * @brief Get current latency in samples
     */
    [[nodiscard]] virtual u32 getLatency() const = 0;
    
    /**
     * @brief Get current latency in milliseconds
     */
    [[nodiscard]] virtual f64 getLatencyMs() const = 0;
};

/**
 * @brief Create audio device manager
 */
[[nodiscard]] std::unique_ptr<IAudioDevice> createAudioDevice();

//=============================================================================
// ASIO-specific interface (Windows only)
//=============================================================================

#if defined(NOMAD_PLATFORM_WINDOWS)

/**
 * @brief ASIO control panel interface
 */
class IAsioControl {
public:
    virtual ~IAsioControl() = default;
    
    /**
     * @brief Show ASIO driver control panel
     * @param deviceId ASIO device identifier
     */
    virtual void showControlPanel(const std::string& deviceId) = 0;
    
    /**
     * @brief Get ASIO buffer size constraints
     */
    virtual void getBufferSizeRange(
        const std::string& deviceId,
        u32& minSize,
        u32& maxSize,
        u32& preferredSize,
        u32& granularity
    ) = 0;
};

#endif // NOMAD_PLATFORM_WINDOWS

} // namespace nomad::platform
