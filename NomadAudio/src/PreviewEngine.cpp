// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#include "PreviewEngine.h"
#include "NomadLog.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <codecvt>
#include <locale>

#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#endif

namespace Nomad {
namespace Audio {

namespace {
    /**
     * @brief Mixes interleaved multichannel audio samples down to a stereo interleaved buffer.
     *
     * The function reads frames from `input` (interleaved by channel), applies fixed mixing
     * coefficients for common channel roles (center, LFE, surrounds and additional channels),
     * writes left/right pairs into `output`, and clamps samples to the [-1.0, 1.0] range.
     *
     * @param input Interleaved source samples (size must be a multiple of `inChannels`).
     * @param inChannels Number of channels in `input`.
     * @param output Destination vector which will be resized to contain stereo interleaved samples
     *               (two floats per frame) and filled with the mixed results.
     */
    void downmixToStereoImpl(const std::vector<float>& input, uint32_t inChannels, std::vector<float>& output) {
        if (inChannels == 0) return;
        const size_t frames = input.size() / inChannels;
        output.assign(frames * 2, 0.0f);

        for (size_t i = 0; i < frames; ++i) {
            float left = 0.0f;
            float right = 0.0f;
            const float* frame = &input[i * inChannels];

            if (inChannels >= 1) left += frame[0];
            if (inChannels >= 2) right += frame[1];
            if (inChannels >= 3) {
                float c = frame[2] * 0.7071f;
                left += c; right += c;
            }
            if (inChannels >= 4) {
                float lfe = frame[3] * 0.5f;
                left += lfe; right += lfe;
            }
            if (inChannels >= 5) left += frame[4] * 0.7071f;
            if (inChannels >= 6) right += frame[5] * 0.7071f;
            for (uint32_t ch = 6; ch < inChannels; ++ch) {
                float v = frame[ch] * 0.5f;
                left += v;
                right += v;
            }

            output[i * 2] = std::max(-1.0f, std::min(1.0f, left));
            output[i * 2 + 1] = std::max(-1.0f, std::min(1.0f, right));
        }
    }

    /**
     * @brief Convert interleaved audio samples to stereo.
     *
     * Converts the provided interleaved sample buffer from its current channel count to 2 channels.
     * If the input is already stereo, the call is a no-op. If the input is mono, each sample is duplicated
     * to produce left and right channels. For inputs with more than two channels, the function mixes
     * channels down to stereo.
     *
     * @param buffer Interleaved audio samples; interpreted with the channel count given by `channelCount`.
     *               On return this buffer contains interleaved stereo samples (2 channels). The function
     *               may reallocate and swap the internal storage.
     * @param channelCount Number of channels in `buffer` on entry; updated to 2 on successful conversion.
     */
    void forceStereo(std::vector<float>& buffer, uint32_t& channelCount) {
        if (channelCount == 2) return;
        if (channelCount == 1) {
            const size_t frames = buffer.size();
            std::vector<float> stereo;
            stereo.reserve(frames * 2);
            for (size_t i = 0; i < frames; ++i) {
                float s = buffer[i];
                stereo.push_back(s);
                stereo.push_back(s);
            }
            buffer.swap(stereo);
            channelCount = 2;
            return;
        }
        std::vector<float> stereo;
        downmixToStereoImpl(buffer, channelCount, stereo);
        buffer.swap(stereo);
        channelCount = 2;
    }

    struct WavInfo {
        uint32_t sampleRate{0};
        uint16_t channels{0};
        uint16_t bitsPerSample{0};
        uint32_t dataOffset{0};
        uint32_t dataSize{0};
        uint16_t audioFormat{0};
    };

    /**
     * @brief Loads and decodes a WAV file into a normalized interleaved float buffer.
     *
     * Reads RIFF/WAVE files supporting PCM (format 1) and IEEE float (format 3) sample data,
     * with 16-, 24-, or 32-bit sample sizes. Decoded samples are converted to host-endian
     * floats in the range [-1.0, 1.0] and stored interleaved in @p audioData.
     *
     * @param filePath Path to the WAV file to load.
     * @param[out] audioData Filled with interleaved audio samples as normalized floats.
     * @param[out] sampleRate Set to the file's sample rate (Hz) on success.
     * @param[out] numChannels Set to the file's channel count on success.
     * @return true if the file was successfully parsed and decoded; false on any parse or decode error.
     */
    bool loadWav(const std::string& filePath, std::vector<float>& audioData, uint32_t& sampleRate, uint32_t& numChannels) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            Log::warning("PreviewEngine: Failed to open WAV file: " + filePath);
            return false;
        }

        char riffId[4], waveId[4];
        uint32_t riffSize = 0;
        if (!file.read(riffId, 4) || !file.read(reinterpret_cast<char*>(&riffSize), 4) || !file.read(waveId, 4)) {
            return false;
        }
        if (std::strncmp(riffId, "RIFF", 4) != 0 || std::strncmp(waveId, "WAVE", 4) != 0) {
            return false;
        }

        bool fmtFound = false, dataFound = false;
        uint16_t audioFormat = 1;
        uint32_t channelCount = 2;
        uint32_t sr = 44100;
        uint16_t bitsPerSample = 16;
        uint32_t dataSize = 0;
        std::streampos dataPos{};

        while (file && !(fmtFound && dataFound)) {
            char chunkId[4];
            uint32_t chunkSize = 0;
            if (!file.read(chunkId, 4) || !file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;

            if (std::strncmp(chunkId, "fmt ", 4) == 0) {
                fmtFound = true;
                if (chunkSize < 16) return false;
                file.read(reinterpret_cast<char*>(&audioFormat), sizeof(uint16_t));
                file.read(reinterpret_cast<char*>(&channelCount), sizeof(uint16_t));
                file.read(reinterpret_cast<char*>(&sr), sizeof(uint32_t));
                uint32_t byteRate = 0;
                uint16_t blockAlign = 0;
                file.read(reinterpret_cast<char*>(&byteRate), sizeof(uint32_t));
                file.read(reinterpret_cast<char*>(&blockAlign), sizeof(uint16_t));
                file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(uint16_t));
                if (chunkSize > 16) file.seekg(chunkSize - 16, std::ios::cur);
            } else if (std::strncmp(chunkId, "data", 4) == 0) {
                dataFound = true;
                dataSize = chunkSize;
                dataPos = file.tellg();
                file.seekg(chunkSize, std::ios::cur);
            } else {
                file.seekg(chunkSize, std::ios::cur);
            }
            if (chunkSize % 2 == 1) file.seekg(1, std::ios::cur);
        }

        if (!(fmtFound && dataFound)) return false;
        if (audioFormat != 1 && audioFormat != 3) return false;
        if (bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) return false;

        file.seekg(dataPos);
        size_t bytesPerSample = bitsPerSample / 8;
        audioData.resize(dataSize / bytesPerSample);

        if (bitsPerSample == 16) {
            std::vector<int16_t> raw(dataSize / sizeof(int16_t));
            file.read(reinterpret_cast<char*>(raw.data()), dataSize);
            for (size_t i = 0; i < raw.size(); ++i) {
                audioData[i] = raw[i] / 32768.0f;
            }
        } else if (bitsPerSample == 24) {
            std::vector<uint8_t> raw(dataSize);
            file.read(reinterpret_cast<char*>(raw.data()), dataSize);
            for (size_t i = 0; i < audioData.size(); ++i) {
                uint32_t sample24 = raw[i * 3] | (raw[i * 3 + 1] << 8) | (raw[i * 3 + 2] << 16);
                if (sample24 & 0x800000) sample24 |= 0xFF000000;
                audioData[i] = static_cast<int32_t>(sample24) / 8388608.0f;
            }
        } else if (bitsPerSample == 32) {
            std::vector<float> raw(dataSize / sizeof(float));
            file.read(reinterpret_cast<char*>(raw.data()), dataSize);
            for (size_t i = 0; i < raw.size(); ++i) {
                audioData[i] = std::max(-1.0f, std::min(1.0f, raw[i]));
            }
        }

        sampleRate = sr;
        numChannels = channelCount;
        return true;
    }

#ifdef _WIN32
    /**
     * @brief Loads and decodes an audio file via Windows Media Foundation into a float PCM buffer.
     *
     * Decodes the specified file to 32-bit float interleaved samples and fills `audioData` with the raw samples.
     * On success, `sampleRate` and `numChannels` are set to the decoded stream's sample rate and channel count.
     *
     * @param filePath Path to the audio file to decode (UTF-8).
     * @param audioData Output vector that will be populated with interleaved 32-bit float samples.
     * @param sampleRate Output sample rate of the decoded audio.
     * @param numChannels Output number of channels in the decoded audio.
     * @return bool `true` if decoding produced one or more float samples and outputs were set, `false` otherwise.
     */
    bool loadWithMediaFoundation(const std::string& filePath,
                                 std::vector<float>& audioData,
                                 uint32_t& sampleRate,
                                 uint32_t& numChannels) {
        using Microsoft::WRL::ComPtr;

        static std::once_flag initFlag;
        static HRESULT initResult = E_FAIL;
        std::call_once(initFlag, []() {
            initResult = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        });
        if (!SUCCEEDED(initResult)) return false;

        ComPtr<IMFAttributes> attributes;
        if (FAILED(MFCreateAttributes(&attributes, 1))) return false;
#if defined(MF_SOURCE_READER_ENABLE_AUDIO_PROCESSING)
        attributes->SetUINT32(MF_SOURCE_READER_ENABLE_AUDIO_PROCESSING, TRUE);
#elif defined(MF_READWRITE_ENABLE_AUDIO_PROCESSING)
        attributes->SetUINT32(MF_READWRITE_ENABLE_AUDIO_PROCESSING, TRUE);
#endif

        ComPtr<IMFSourceReader> reader;
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring widePath = converter.from_bytes(filePath);
        HRESULT hr = MFCreateSourceReaderFromURL(widePath.c_str(), attributes.Get(), &reader);
        if (FAILED(hr)) return false;

        ComPtr<IMFMediaType> audioType;
        hr = MFCreateMediaType(&audioType);
        if (FAILED(hr)) return false;
        audioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        audioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
        audioType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
        hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioType.Get());
        if (FAILED(hr)) return false;

        ComPtr<IMFMediaType> currentType;
        hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &currentType);
        if (FAILED(hr)) return false;

        UINT32 sr = 0, ch = 0;
        if (FAILED(currentType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sr))) return false;
        if (FAILED(currentType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &ch))) return false;
        sampleRate = sr;
        numChannels = ch;

        audioData.clear();
        while (true) {
            DWORD flags = 0;
            ComPtr<IMFSample> sample;
            hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
            if (FAILED(hr)) return false;
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
            if (!sample) continue;

            ComPtr<IMFMediaBuffer> buffer;
            if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) continue;

            BYTE* data = nullptr;
            DWORD maxLen = 0, curLen = 0;
            if (FAILED(buffer->Lock(&data, &maxLen, &curLen))) continue;
            size_t prev = audioData.size();
            audioData.resize(prev + (curLen / sizeof(float)));
            std::memcpy(audioData.data() + prev, data, curLen);
            buffer->Unlock();
        }

        return !audioData.empty();
    }
#endif
} /**
     * @brief Initialize PreviewEngine with default playback state.
     *
     * Initializes internal state: no active voice, output sample rate set to 48000 Hz,
     * and global preview gain set to -6 dB.
     */

PreviewEngine::PreviewEngine()
    : m_activeVoice(nullptr)
    , m_outputSampleRate(48000.0)
    , m_globalGainDb(-6.0f) {}

/**
 * @brief Ensures any active preview playback is stopped and associated resources are released before destruction.
 *
 * The destructor stops ongoing playback so the engine can be safely destroyed without leaving an active voice or pending audio processing.
 */
PreviewEngine::~PreviewEngine() {
    stop();
}

/**
 * @brief Convert a gain value in decibels to a linear amplitude multiplier.
 *
 * @param db Gain in decibels.
 * @return float Linear amplitude multiplier equal to 10^(db / 20).
 */
float PreviewEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

/**
 * @brief Loads an audio file into a pooled AudioBuffer and returns the buffer.
 *
 * Attempts to decode the file at the given path, converts the decoded data to stereo,
 * and populates a pooled AudioBuffer with the audio samples, sample rate, channel count,
 * frame count, and source path. Supported input paths include WAV files; on Windows other
 * formats may be decoded via Media Foundation.
 *
 * @param path Filesystem path to the audio file to load.
 * @param sampleRate Output parameter set to the decoded sample rate (Hz) on success.
 * @param channels Output parameter set to the decoded channel count on success (after forcing stereo).
 * @return std::shared_ptr<AudioBuffer> Shared pointer to the acquired AudioBuffer on success,
 *         or `nullptr` if loading or decoding failed.
 */
std::shared_ptr<AudioBuffer> PreviewEngine::loadBuffer(const std::string& path, uint32_t& sampleRate, uint32_t& channels) {
    auto loader = [path, &sampleRate, &channels](AudioBuffer& out) -> bool {
        std::string extension;
        if (auto dotPos = path.find_last_of('.'); dotPos != std::string::npos && dotPos + 1 < path.size()) {
            extension = path.substr(dotPos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        }

        std::vector<float> decoded;
        uint32_t sr = 48000;
        uint32_t ch = 2;
        bool ok = false;

        if (extension == "wav") {
            ok = loadWav(path, decoded, sr, ch);
        } else {
#ifdef _WIN32
            ok = loadWithMediaFoundation(path, decoded, sr, ch);
#else
            ok = false;
#endif
        }

        if (!ok) return false;
        forceStereo(decoded, ch);
        out.data.swap(decoded);
        out.sampleRate = sr;
        out.channels = ch;
        out.numFrames = out.channels ? out.data.size() / out.channels : 0;
        out.sourcePath = path;
        sampleRate = sr;
        channels = ch;
        return true;
    };

    return SamplePool::getInstance().acquire(path, loader);
}

/**
 * @brief Starts playback of an audio file as a preview and registers it as the active voice.
 *
 * Loads or retrieves a cached audio buffer for the given path, configures a PreviewVoice
 * with playback parameters (sample rate, channel count, duration, gain, and optional maximum
 * play time), and makes it the engine's active preview.
 *
 * @param path Filesystem path to the audio file to preview.
 * @param gainDb Per-preview gain in decibels; combined with the engine's global preview volume.
 * @param maxSeconds Maximum playback duration in seconds; if <= 0, the full buffer duration is used.
 * @return PreviewResult `Success` if playback was started and an active voice was set, `Failed` if loading the buffer failed.
 */
PreviewResult PreviewEngine::play(const std::string& path, float gainDb, double maxSeconds) {
    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    auto buffer = loadBuffer(path, sampleRate, channels);
    if (!buffer || !buffer->ready.load()) {
        Log::warning("PreviewEngine: Failed to load preview for " + path);
        return PreviewResult::Failed;
    }
    // Cache hit path: populate metadata from buffer if loader didn't run
    if (sampleRate == 0 && buffer->sampleRate > 0) {
        sampleRate = buffer->sampleRate;
    }
    if (channels == 0 && buffer->channels > 0) {
        channels = buffer->channels;
    }

    auto voice = std::make_shared<PreviewVoice>();
    voice->buffer = buffer;
    voice->path = path;
    voice->sampleRate = sampleRate > 0 ? static_cast<double>(sampleRate) : 48000.0;
    voice->channels = channels == 0 ? 2u : channels;
    voice->durationSeconds = (sampleRate > 0 && buffer->numFrames > 0) ? (static_cast<double>(buffer->numFrames) / sampleRate) : 0.0;
    voice->maxPlaySeconds = maxSeconds;
    voice->gain = dbToLinear(gainDb + m_globalGainDb.load(std::memory_order_relaxed));
    voice->phaseFrames = 0.0;
    voice->elapsedSeconds = 0.0;
    voice->fadeInPos = 0.0;
    voice->fadeOutPos = 0.0;
    voice->stopRequested.store(false, std::memory_order_release);
    voice->fadeOutActive = false;
    voice->playing.store(true, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(m_voiceMutex);
        m_activeVoice = voice;
    }
    Log::info("PreviewEngine: Playing '" + path + "' (" + std::to_string(sampleRate) + " Hz, " +
              std::to_string(voice->durationSeconds) + " sec)");
    return PreviewResult::Success;
}

/**
 * @brief Signals the currently active preview to stop and begins a fade-out.
 *
 * If a preview is active, marks it as requested to stop and activates its fade-out envelope,
 * resetting the fade position. This operation is thread-safe.
 */
void PreviewEngine::stop() {
    std::lock_guard<std::mutex> lock(m_voiceMutex);
    if (m_activeVoice) {
        m_activeVoice->stopRequested.store(true, std::memory_order_release);
        m_activeVoice->fadeOutActive = true;
        m_activeVoice->fadeOutPos = 0.0;
    }
}

/**
 * @brief Set the engine's output sample rate used for resampling during rendering.
 *
 * If `sr` is less than or equal to zero, the call is ignored and the sample rate remains unchanged.
 *
 * @param sr Output sample rate in hertz (Hz).
 */
void PreviewEngine::setOutputSampleRate(double sr) {
    if (sr <= 0.0) return;
    m_outputSampleRate.store(sr, std::memory_order_relaxed);
}

/**
 * @brief Renders the current preview voice into an interleaved stereo output buffer and advances playback state.
 *
 * Mixes the active preview buffer into the provided interleaved stereo float buffer using linear interpolation,
 * per-sample gain, and a short fade-in/fade-out envelope. Advances the voice playback phase and elapsed time,
 * triggers a fade-out when the buffer end or max-play duration is reached, and invokes the completion callback
 * and clears the active voice when playback has finished.
 *
 * @param interleavedOutput Pointer to an interleaved stereo float buffer (L,R,L,R...) that will be mixed into;
 *                         existing contents are preserved and added to.
 * @param numFrames Number of output frames to render into the buffer.
 */
void PreviewEngine::process(float* interleavedOutput, uint32_t numFrames) {
    std::shared_ptr<PreviewVoice> voice;
    {
        std::lock_guard<std::mutex> lock(m_voiceMutex);
        voice = m_activeVoice;
    }
    if (!voice || !voice->playing.load(std::memory_order_acquire) || !interleavedOutput) {
        return;
    }
    auto buffer = voice->buffer;
    if (!buffer || buffer->data.empty() || buffer->sampleRate == 0) {
        return;
    }

    const double streamRate = (m_outputSampleRate.load(std::memory_order_relaxed) > 0.0) ? m_outputSampleRate.load() : 48000.0;
    const double fadeInSamples = streamRate * 0.02;  // 20ms fade-in
    const double fadeOutSamples = streamRate * 0.05; // 50ms fade-out
    const double ratio = voice->sampleRate / streamRate;
    const uint64_t totalFrames = buffer->numFrames;
    const float* data = buffer->data.data();
    double phase = voice->phaseFrames;
    const float gain = voice->gain;

    for (uint32_t i = 0; i < numFrames; ++i) {
        if (static_cast<uint64_t>(phase) >= totalFrames) {
            voice->stopRequested.store(true, std::memory_order_release);
            voice->fadeOutActive = true;
            break;
        }
        uint64_t idx = static_cast<uint64_t>(phase);
        uint64_t idx1 = std::min<uint64_t>(idx + 1, totalFrames - 1);
        float frac = static_cast<float>(phase - idx);

        float l0 = data[idx * 2];
        float l1 = data[idx1 * 2];
        float r0 = data[idx * 2 + 1];
        float r1 = data[idx1 * 2 + 1];

        float outL = l0 + frac * (l1 - l0);
        float outR = r0 + frac * (r1 - r0);

        float envelope = 1.0f;
        if (voice->fadeInPos < fadeInSamples) {
            envelope = static_cast<float>(voice->fadeInPos / fadeInSamples);
            voice->fadeInPos += 1.0;
        }
        if (voice->stopRequested.load(std::memory_order_acquire) || voice->fadeOutActive) {
            voice->fadeOutActive = true;
            double remaining = std::max(0.0, (fadeOutSamples - voice->fadeOutPos) / fadeOutSamples);
            envelope *= static_cast<float>(remaining);
            voice->fadeOutPos += 1.0;
        }

        interleavedOutput[i * 2] += outL * gain * envelope;
        interleavedOutput[i * 2 + 1] += outR * gain * envelope;

        phase += ratio;
    }

    voice->phaseFrames = phase;
    voice->elapsedSeconds += static_cast<double>(numFrames) / streamRate;
    if (voice->maxPlaySeconds > 0.0 && voice->elapsedSeconds >= voice->maxPlaySeconds) {
        voice->stopRequested.store(true, std::memory_order_release);
        voice->fadeOutActive = true;
    }

    bool finished = voice->fadeOutActive && (voice->fadeOutPos >= fadeOutSamples);
    if (finished) {
        if (m_onComplete) {
            m_onComplete(voice->path);
        }
        std::lock_guard<std::mutex> lock(m_voiceMutex);
        m_activeVoice.reset();
    }
}

/**
 * @brief Report whether a preview is currently playing.
 *
 * @return `true` if there is an active voice and it is marked as playing, `false` otherwise.
 */
bool PreviewEngine::isPlaying() const {
    std::lock_guard<std::mutex> lock(m_voiceMutex);
    auto voice = m_activeVoice;
    return voice && voice->playing.load(std::memory_order_acquire);
}

/**
 * @brief Set a callback to be invoked when a preview finishes playback.
 *
 * Stores a function that will be called with the previewed file path when playback completes.
 *
 * @param callback Function called with the completed preview's file path.
 */
void PreviewEngine::setOnComplete(std::function<void(const std::string& path)> callback) {
    m_onComplete = std::move(callback);
}

/**
 * @brief Set the global preview volume used for all previews.
 *
 * Updates the engine's global gain in decibels which is applied to subsequent playback.
 *
 * @param gainDb Global gain in decibels.
 */
void PreviewEngine::setGlobalPreviewVolume(float gainDb) {
    m_globalGainDb.store(gainDb, std::memory_order_relaxed);
}

/**
 * @brief Accesses the current global preview gain setting.
 *
 * @return float Current global preview volume in decibels.
 */
float PreviewEngine::getGlobalPreviewVolume() const {
    return m_globalGainDb.load(std::memory_order_relaxed);
}

} // namespace Audio
} // namespace Nomad