#pragma once

#include "AudioDriver.h"
#include "RtAudio.h"
#include <memory>

namespace Nomad {
namespace Audio {

/**
 * @brief NomadASIOBackend - attempts to select ASIO (low-latency) via RtAudio when available.
 * Falls back to default RtAudio behavior if ASIO isn't available in the RtAudio build.
 */
class NomadASIOBackend : public AudioDriver {
public:
    NomadASIOBackend();
    ~NomadASIOBackend() override;

    std::vector<AudioDeviceInfo> getDevices() override;
    uint32_t getDefaultOutputDevice() override;
    uint32_t getDefaultInputDevice() override;
    bool openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) override;
    void closeStream() override;
    bool startStream() override;
    void stopStream() override;
    bool isStreamRunning() const override;
    double getStreamLatency() const override;

private:
    uint32_t getDefaultOutputDeviceFallback();
    uint32_t getDefaultInputDeviceFallback();
    std::vector<AudioDeviceInfo> getDevicesFallback();
    bool isAsioAvailable();

private:
    std::unique_ptr<RtAudio> m_rtAudio;
    AudioCallback m_userCallback;
    void* m_userData;
    bool m_requestedAsio;

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
