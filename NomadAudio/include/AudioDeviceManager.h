#pragma once

#include "AudioDriver.h"
#include <memory>
#include <functional>

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

private:
    std::unique_ptr<AudioDriver> m_driver;
    AudioStreamConfig m_currentConfig;
    AudioCallback m_currentCallback;
    void* m_currentUserData;
    bool m_initialized;
    bool m_wasRunning;
};

} // namespace Audio
} // namespace Nomad
