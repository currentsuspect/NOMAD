// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "AudioDriver.h"
#include "RtAudio.h"
#include <memory>

namespace Nomad {
/**
 * RtAudio-based implementation of the AudioDriver interface.
 *
 * Provides device enumeration, stream lifecycle management, and a thin
 * wrapper around RtAudio for audio I/O.
 */

/**
 * Construct a new RtAudioBackend instance and initialize internal state.
 */

/**
 * Destroy the RtAudioBackend and release any opened resources.
 */

/**
 * Retrieve a list of available audio devices.
 *
 * @returns A vector of AudioDeviceInfo describing each available device.
 */

/**
 * Get the index/identifier of the system default output device.
 *
 * @returns The default output device index.
 */

/**
 * Get the index/identifier of the system default input device.
 *
 * @returns The default input device index.
 */

/**
 * Open an audio stream using the provided configuration and register a user callback.
 *
 * @param config Stream configuration parameters (format, channels, sample rate, buffers, etc.).
 * @param callback User callback invoked to process audio buffers.
 * @param userData Opaque pointer forwarded to the user callback.
 * @returns `true` if the stream was opened successfully, `false` otherwise.
 */

/**
 * Close the currently opened audio stream and release associated resources.
 */

/**
 * Start the opened audio stream.
 *
 * @returns `true` if the stream was started successfully, `false` otherwise.
 */

/**
 * Stop the active audio stream.
 */

/**
 * Query whether the audio stream is currently running.
 *
 * @returns `true` if the stream is running, `false` otherwise.
 */

/**
 * Get the current latency of the open stream in seconds.
 *
 * @returns The stream latency in seconds.
 */

/**
 * Get the sample rate currently in use by the open stream.
 *
 * @returns The stream sample rate in Hz.
 */

/**
 * RtAudio callback wrapper that forwards RtAudio buffer calls to the stored user callback.
 *
 * @param outputBuffer Pointer to the output buffer provided by RtAudio (may be null for input-only streams).
 * @param inputBuffer Pointer to the input buffer provided by RtAudio (may be null for output-only streams).
 * @param numFrames Number of frames to process for this callback invocation.
 * @param streamTime Stream time in seconds as reported by RtAudio.
 * @param status RtAudio stream status flags for this callback.
 * @param userData Opaque pointer expected to reference the RtAudioBackend instance.
 * @returns An integer status code interpreted by RtAudio (0 for continue, non-zero to stop under some APIs).
 */
namespace Audio {

/**
 * @brief RtAudio implementation of AudioDriver
 */
class RtAudioBackend : public AudioDriver {
public:
    RtAudioBackend();
    ~RtAudioBackend() override;

    std::vector<AudioDeviceInfo> getDevices() override;
    uint32_t getDefaultOutputDevice() override;
    uint32_t getDefaultInputDevice() override;
    bool openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) override;
    void closeStream() override;
    bool startStream() override;
    void stopStream() override;
    bool isStreamRunning() const override;
    double getStreamLatency() const override;
    uint32_t getStreamSampleRate() const override;
    
    // Get the actual API being used
    RtAudio::Api getCurrentApi() const { return m_rtAudio ? m_rtAudio->getCurrentApi() : RtAudio::UNSPECIFIED; }

private:
    std::unique_ptr<RtAudio> m_rtAudio;
    AudioCallback m_userCallback;
    void* m_userData;

    // RtAudio callback wrapper
    static int rtAudioCallback(
        void* outputBuffer,
        void* inputBuffer,
        unsigned int numFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData
    );
};

} // namespace Audio
} // namespace Nomad