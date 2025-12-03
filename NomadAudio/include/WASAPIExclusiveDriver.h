// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "NativeAudioDriver.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <thread>
#include <atomic>
#include <mutex>

namespace Nomad {
namespace Audio {

/**
 * @brief WASAPI Exclusive Mode Driver
 * 
 * Professional low-latency mode for NOMAD.
 * Features:
 * - Exclusive device access (no mixing)
 * - Event-driven callbacks
 * - Sample-rate matching
 * - Ultra-low latency (~3-5ms achievable)
 * - Direct hardware control
 */
class WASAPIExclusiveDriver : public NativeAudioDriver {
public:
    WASAPIExclusiveDriver();
    ~WASAPIExclusiveDriver() override;

    // NativeAudioDriver interface
    AudioDriverType getDriverType() const override { return AudioDriverType::WASAPI_EXCLUSIVE; }
    DriverCapability getCapabilities() const override;
    DriverState getState() const override { return m_state; }
    DriverError getLastError() const override { return m_lastError; }
    std::string getErrorMessage() const override { return m_errorMessage; }
    DriverStatistics getStatistics() const override { return m_statistics; }
    void resetStatistics() override { m_statistics.reset(); }
    bool initialize() override;
    void shutdown() override;
    bool isAvailable() const override;
    float getTypicalLatencyMs() const override { return 5.0f; }
    void setErrorCallback(ErrorCallback callback) override { m_errorCallback = callback; }

    // AudioDriver interface
    std::vector<AudioDeviceInfo> getDevices() override;
    /**
     * Retrieve the identifier of the system's default output audio device.
     * @returns The device ID of the default output device, or 0 if none is available.
     */
    /**
     * Retrieve the identifier of the system's default input audio device.
     * @returns The device ID of the default input device, or 0 if none is available.
     */
    /**
     * Open an exclusive-mode audio stream with the specified configuration and callback.
     * @param config Configuration for the audio stream (sample rate, channels, buffer size, etc.).
     * @param callback User-provided audio processing callback invoked for each buffer.
     * @param userData Opaque pointer forwarded to the user callback.
     * @returns `true` if the stream was opened successfully, `false` otherwise.
     */
    /**
     * Close the currently opened audio stream and release associated resources.
     */
    /**
     * Start audio processing on the opened stream.
     * @returns `true` if the stream was started successfully, `false` otherwise.
     */
    /**
     * Stop audio processing on the currently running stream.
     */
    /**
     * Query whether the audio stream is currently running.
     * @returns `true` if the stream is running, `false` otherwise.
     */
    uint32_t getDefaultOutputDevice() override;
    uint32_t getDefaultInputDevice() override;
    bool openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) override;
    void closeStream() override;
    bool startStream() override;
    void stopStream() override;
    bool isStreamRunning() const override { return m_isRunning; }
    /**
     * Get the current stream latency in milliseconds.
     *
     * @returns Latency of the currently opened stream in milliseconds.
     */
    /**
     * Get the sample rate used by the current stream.
     *
     * @returns Sample rate in samples per second (Hz), or `0` if no wave format is available.
     */
    double getStreamLatency() const override;
    uint32_t getStreamSampleRate() const override { return m_waveFormat ? m_waveFormat->nSamplesPerSec : 0; }

    /**
     * @brief Check if exclusive mode is available for a device
     */
    bool isExclusiveModeAvailable(uint32_t deviceId) const;

    /**
     * @brief Get supported sample rates in exclusive mode
     */
    std::vector<uint32_t> getSupportedExclusiveSampleRates(uint32_t deviceId) const;

private:
    // COM interfaces
    IMMDeviceEnumerator* m_deviceEnumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioRenderClient* m_renderClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;

    // Thread management
    std::thread m_audioThread;
    std::atomic<bool> m_isRunning{ false };
    std::atomic<bool> m_shouldStop{ false };
    HANDLE m_audioEvent = nullptr;

    // State
    DriverState m_state = DriverState::UNINITIALIZED;
    DriverError m_lastError = DriverError::NONE;
    std::string m_errorMessage;
    DriverStatistics m_statistics;
    ErrorCallback m_errorCallback;

    // Stream configuration
    AudioStreamConfig m_config;
    AudioCallback m_userCallback = nullptr;
    void* m_userData = nullptr;

    // Format information
    WAVEFORMATEX* m_waveFormat = nullptr;
    uint32_t m_bufferFrameCount = 0;
    uint32_t m_actualSampleRate = 0;
    
    // Soft-start ramp to prevent harsh audio on initialization
    uint32_t m_rampSampleCount = 0;
    uint32_t m_rampDurationSamples = 0;
    bool m_isRamping = false;

    // Performance monitoring
    LARGE_INTEGER m_perfFreq;
    LARGE_INTEGER m_lastCallbackTime;

    // Internal methods
    bool initializeCOM();
    void shutdownCOM();
    bool enumerateDevices(std::vector<AudioDeviceInfo>& devices);
    bool openDevice(uint32_t deviceId);
    void closeDevice();
    bool initializeAudioClient();
    bool findBestExclusiveFormat(WAVEFORMATEX** format);
    bool testExclusiveFormat(uint32_t sampleRate, uint32_t channels, WAVEFORMATEX** format);
    bool testExclusiveFormatPCM(uint32_t sampleRate, uint32_t channels, uint32_t bitsPerSample, WAVEFORMATEX** format);
    void audioThreadProc();
    void setError(DriverError error, const std::string& message);
    void updateStatistics(double callbackTimeUs);
    bool setThreadPriority();
};

} // namespace Audio
} // namespace Nomad