#pragma once

#include "AudioDriver.h"
#include "NativeAudioDriver.h"
#include "AudioDriverTypes.h"
#include "ASIODriverInfo.h"
#include <memory>
#include <functional>
#include <vector>
#include <chrono>

namespace Nomad {
namespace Audio {

/**
 * @brief Manages audio devices and streams
 * 
 * Provides high-level interface for audio I/O:
 * - Device enumeration and selection
 * - Stream configuration
 * - Callback management
 */
class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    /**
     * @brief Initialize audio system
     */
    bool initialize();

    /**
     * @brief Shutdown audio system
     */
    void shutdown();

    /**
     * @brief Get available audio devices
     */
    std::vector<AudioDeviceInfo> getDevices() const;

    /**
     * @brief Get default output device
     */
    AudioDeviceInfo getDefaultOutputDevice() const;

    /**
     * @brief Get default input device
     */
    AudioDeviceInfo getDefaultInputDevice() const;

    /**
     * @brief Open audio stream with configuration
     */
    bool openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData);

    /**
     * @brief Close current audio stream
     */
    void closeStream();

    /**
     * @brief Start audio processing
     */
    bool startStream();

    /**
     * @brief Stop audio processing
     */
    void stopStream();

    /**
     * @brief Check if stream is active
     */
    bool isStreamRunning() const;

    /**
     * @brief Get current stream latency
     */
    double getStreamLatency() const;
    
    /**
     * @brief Get latency compensation values for recording
     * @param[out] inputLatencyMs Input latency in milliseconds
     * @param[out] outputLatencyMs Output latency in milliseconds
     * 
     * Call this after opening a stream to get latency compensation values.
     * These should be passed to Track::setLatencyCompensation() for recording.
     */
    void getLatencyCompensationValues(double& inputLatencyMs, double& outputLatencyMs) const;

    /**
     * @brief Get current configuration
     */
    const AudioStreamConfig& getCurrentConfig() const { return m_currentConfig; }

    /**
     * @brief Switch to a different audio device
     * @param deviceId New device ID to switch to
     * @return true if device switch was successful
     * 
     * This will stop the current stream, reconfigure with the new device,
     * and restart the stream if it was running.
     */
    bool switchDevice(uint32_t deviceId);

    /**
     * @brief Update sample rate
     * @param sampleRate New sample rate in Hz
     * @return true if sample rate was updated successfully
     * 
     * This will restart the stream with the new sample rate.
     */
    bool setSampleRate(uint32_t sampleRate);

    /**
     * @brief Update buffer size
     * @param bufferSize New buffer size in frames
     * @return true if buffer size was updated successfully
     * 
     * This will restart the stream with the new buffer size.
     */
    bool setBufferSize(uint32_t bufferSize);

    /**
     * @brief Validate if a device supports the given configuration
     * @param deviceId Device ID to validate
     * @param sampleRate Desired sample rate
     * @return true if the device supports the configuration
     */
    bool validateDeviceConfig(uint32_t deviceId, uint32_t sampleRate) const;

    /**
     * @brief Get active driver type
     */
    AudioDriverType getActiveDriverType() const;

    /**
     * @brief Set preferred driver type
     * @param type Preferred driver type (WASAPI_EXCLUSIVE or WASAPI_SHARED)
     * @return true if driver type was set successfully
     * 
     * This will attempt to reopen the stream with the specified driver.
     * Falls back to next best driver if the preferred one fails.
     */
    bool setPreferredDriverType(AudioDriverType type);

    /**
     * @brief Check if a specific driver type is currently available
     * @param type Driver type to check
     * @return true if the driver can be opened (not blocked by another app)
     * 
     * This performs a quick test-open to check availability without changing
     * the active stream. Useful for UI to grey out unavailable options.
     */
    bool isDriverTypeAvailable(AudioDriverType type) const;

    /**
     * @brief Get list of available driver types
     */
    std::vector<AudioDriverType> getAvailableDriverTypes() const;

    /**
     * @brief Check if we're using a fallback driver (not the preferred one)
     * @return true if active driver differs from preferred driver
     * 
     * This is useful for showing warnings in the UI when Exclusive mode
     * was requested but Shared mode is active (e.g., due to conflicts).
     */
    bool isUsingFallbackDriver() const;

    /**
     * @brief Get ASIO drivers for display purposes
     */
    std::vector<ASIODriverInfo> getASIODrivers() const;

    /**
     * @brief Get active driver statistics
     */
    DriverStatistics getDriverStatistics() const;

    /**
     * @brief Get ASIO driver info message
     */
    std::string getASIOInfo() const;

    /**
     * @brief Enable/disable auto-buffer scaling on underruns
     * @param enable True to enable auto-scaling, false to disable
     * @param underrunsPerMinuteThreshold Number of underruns/min before scaling (default: 10)
     */
    void setAutoBufferScaling(bool enable, uint32_t underrunsPerMinuteThreshold = 10);

    /**
     * @brief Check underrun rate and auto-scale buffer if needed
     * Call this periodically (e.g., every second) to monitor performance
     */
    void checkAndAutoScaleBuffer();

private:
    // Driver management
    std::unique_ptr<NativeAudioDriver> m_exclusiveDriver;
    std::unique_ptr<NativeAudioDriver> m_sharedDriver;
    NativeAudioDriver* m_activeDriver = nullptr;
    AudioDriverType m_preferredDriverType = AudioDriverType::WASAPI_EXCLUSIVE;  // Prefer Exclusive, auto-fallback to Shared if blocked
    
    // Legacy RtAudio support (fallback)
    std::unique_ptr<AudioDriver> m_rtAudioDriver;
    
    AudioStreamConfig m_currentConfig;
    AudioCallback m_currentCallback;
    void* m_currentUserData;
    bool m_initialized;
    bool m_wasRunning;
    
    // Auto-buffer scaling
    bool m_autoBufferScalingEnabled = false;
    uint32_t m_underrunThreshold = 10;  // Underruns per minute
    uint64_t m_lastUnderrunCount = 0;
    std::chrono::steady_clock::time_point m_lastUnderrunCheck;
    
    // Helper methods
    bool tryDriver(NativeAudioDriver* driver, const AudioStreamConfig& config, 
                   AudioCallback callback, void* userData);
};

} // namespace Audio
} // namespace Nomad
