// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "SamplePool.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace Nomad {
namespace Audio {

enum class PreviewResult {
    Success,
    Failed
};

class PreviewEngine {
public:
    PreviewEngine();
    ~PreviewEngine();

    PreviewResult play(const std::string& path, float gainDb = -6.0f, double maxSeconds = 5.0);
    void stop();
    void setOutputSampleRate(double sr);
    void process(float* interleavedOutput, uint32_t numFrames);
    bool isPlaying() const;
    void setOnComplete(std::function<void(const std::string& path)> callback);
    void setGlobalPreviewVolume(float gainDb);
    float getGlobalPreviewVolume() const;

private:
    struct PreviewVoice {
        std::shared_ptr<AudioBuffer> buffer;
        std::string path;
        double phaseFrames{0.0};
        double sampleRate{48000.0};
        /**
 * Number of audio channels in the preview buffer.
 *
 * Defaults to 2 (stereo).
 */
uint32_t channels{2};
        float gain{0.5f};
        double durationSeconds{0.0};
        double maxPlaySeconds{0.0};
        double elapsedSeconds{0.0};
        double fadeInPos{0.0};
        /**
 * Load audio file data into an AudioBuffer and obtain its sample rate and channel count.
 * @param path Filesystem path to the audio file to load.
 * @param[out] sampleRate Set to the loaded audio's sample rate on success.
 * @param[out] channels Set to the loaded audio's channel count on success.
 * @returns Shared pointer to an AudioBuffer containing interleaved float samples, or `nullptr` if loading failed.
 */
/**
 * Convert an input audio buffer to stereo in-place.
 * If the input channel count differs from 2, the function modifies `data` so it contains interleaved stereo samples.
 * @param data Interleaved audio samples; modified in-place to contain 2-channel (stereo) interleaved samples.
 * @param inChannels Number of channels currently represented in `data`.
 */
/**
 * Convert a gain value in decibels to a linear amplitude multiplier.
 * @param db Gain in decibels.
 * @returns Linear amplitude multiplier corresponding to `db`.
 */
double fadeOutPos{0.0};
        std::atomic<bool> stopRequested{false};
        bool fadeOutActive{false};
        std::atomic<bool> playing{false};
    };

    std::shared_ptr<AudioBuffer> loadBuffer(const std::string& path, uint32_t& sampleRate, uint32_t& channels);
    void downmixToStereo(std::vector<float>& data, uint32_t inChannels);
    float dbToLinear(float db) const;

    // Use shared_ptr with a mutex guard instead of atomic (std::atomic does not support non-trivial types)
    mutable std::mutex m_voiceMutex;
    std::shared_ptr<PreviewVoice> m_activeVoice;
    std::atomic<double> m_outputSampleRate;
    std::atomic<float> m_globalGainDb;
    std::function<void(const std::string&)> m_onComplete;
};

} // namespace Audio
} // namespace Nomad