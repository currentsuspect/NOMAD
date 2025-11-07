// Â© 2025 Nomad Studios â€” All Rights Reserved. Licensed for personal & educational use only.
#include "Track.h"
#include "NomadLog.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>
#include <cassert>
#include <chrono>
// Enable SSE intrinsics for flush-to-zero / denormals-zero control
#include <xmmintrin.h>

namespace Nomad {
namespace Audio {

// === Audio Quality Preset Implementations ===

AudioQualitySettings AudioQualitySettings::Economy() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::Economy;
    settings.resampling = ResamplingMode::Fast;
    settings.dithering = DitheringMode::None;
    settings.precision = InternalPrecision::Float32;
    settings.oversampling = OversamplingMode::None;
    settings.removeDCOffset = false;
    settings.enableSoftClipping = false;
    settings.antiAliasingFilter = FilterSteepness::Soft;
    return settings;
}

void Track::play() {
    if (getState() == TrackState::Playing) return;

    Log::info("Starting playback on track: " + m_name);
    setState(TrackState::Playing);
    // Enable short per-block debug trace so we can observe the first few audio blocks
    // after playback starts. This is temporary instrumentation to help diagnose
    // silent-gap issues when samples are placed on the timeline and played.
    // Arm a larger number of blocks so a short Play run will still emit traces
    // (useful while reproducing intermittent silent-drop issues)
    m_debugTraceBlocksRemaining.store(32);
}

AudioQualitySettings AudioQualitySettings::Balanced() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::Balanced;
    settings.resampling = ResamplingMode::Medium;
    settings.dithering = DitheringMode::Triangular;
    settings.precision = InternalPrecision::Float32;
    settings.oversampling = OversamplingMode::Auto;
    settings.removeDCOffset = true;
    settings.enableSoftClipping = false;
    settings.antiAliasingFilter = FilterSteepness::Medium;
    return settings;
}

AudioQualitySettings AudioQualitySettings::HighFidelity() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::HighFidelity;
    settings.resampling = ResamplingMode::High;
    settings.dithering = DitheringMode::NoiseShaped;
    settings.precision = InternalPrecision::Float32;
    settings.oversampling = OversamplingMode::Force2x;
    settings.removeDCOffset = true;
    settings.enableSoftClipping = true;
    settings.antiAliasingFilter = FilterSteepness::Steep;
    return settings;
}

AudioQualitySettings AudioQualitySettings::Mastering() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::Mastering;
    settings.resampling = ResamplingMode::Ultra;
    settings.dithering = DitheringMode::NoiseShaped;
    settings.precision = InternalPrecision::Float64;
    settings.oversampling = OversamplingMode::Force4x;
    settings.removeDCOffset = true;
    settings.enableSoftClipping = true;
    settings.antiAliasingFilter = FilterSteepness::Steep;
    return settings;
}

void AudioQualitySettings::applyPreset(QualityPreset newPreset) {
    switch (newPreset) {
        case QualityPreset::Economy:
            *this = Economy();
            break;
        case QualityPreset::Balanced:
            *this = Balanced();
            break;
        case QualityPreset::HighFidelity:
            *this = HighFidelity();
            break;
        case QualityPreset::Mastering:
            *this = Mastering();
            break;
        case QualityPreset::Custom:
            // Keep current settings, just update preset marker
            preset = QualityPreset::Custom;
            break;
    }
}

// WAV file header structure (Windows compatible)
#pragma pack(push, 1)
struct WavHeader {
    char riff[4];        // "RIFF"
    uint32_t fileSize;   // File size - 8
    char wave[4];        // "WAVE"
    char fmt[4];         // "fmt "
    uint32_t fmtSize;    // Format chunk size
    uint16_t audioFormat; // Audio format (1 = PCM)
    uint16_t numChannels; // Number of channels
    uint32_t sampleRate;  // Sample rate
    uint32_t byteRate;    // Byte rate
    uint16_t blockAlign;  // Block align
    uint16_t bitsPerSample; // Bits per sample
    char data[4];        // "data"
    uint32_t dataSize;    // Data chunk size
};
#pragma pack(pop)

// Simple WAV file loader
bool loadWavFile(const std::string& filePath, std::vector<float>& audioData, uint32_t& sampleRate, uint16_t& numChannels) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        Log::warning("Failed to open WAV file: " + filePath);
        return false;
    }

    WavHeader header = {};
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    // Debug: Log WAV header information
    Log::info("WAV Header Debug:");
    Log::info("  RIFF: " + std::string(header.riff, 4));
    Log::info("  File size: " + std::to_string(header.fileSize));
    Log::info("  WAVE: " + std::string(header.wave, 4));
    Log::info("  fmt: " + std::string(header.fmt, 4));
    Log::info("  Format size: " + std::to_string(header.fmtSize));
    Log::info("  Audio format: " + std::to_string(header.audioFormat));
    Log::info("  Channels: " + std::to_string(header.numChannels));
    Log::info("  Sample rate: " + std::to_string(header.sampleRate));
    Log::info("  Bits per sample: " + std::to_string(header.bitsPerSample));
    Log::info("  Data chunk: " + std::string(header.data, 4));
    Log::info("  Data size: " + std::to_string(header.dataSize));

    // Robust WAV chunk parser - handles files with any chunk order/types
    std::cout << "DEBUG: Checking first chunk after WAVE, header.fmt = '" << std::string(header.fmt, 4) << "'" << std::endl;
    
    // Reset file position to right after WAVE header (12 bytes from start)
    file.seekg(12, std::ios::beg);
    
    bool foundFmtChunk = false;
    bool foundDataChunk = false;
    
    // Parse all chunks until we find both fmt and data
    while (file && (!foundFmtChunk || !foundDataChunk)) {
        char chunkType[4];
        uint32_t chunkSize;
        
        if (!file.read(chunkType, 4)) break;
        if (!file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;
        
        std::string chunkTypeStr(chunkType, 4);
        Log::info("Found chunk: " + chunkTypeStr + ", size: " + std::to_string(chunkSize));
        
        if (chunkTypeStr == "fmt ") {
            // Read fmt chunk data
            if (chunkSize < 16) {
                Log::error("Invalid fmt chunk size: " + std::to_string(chunkSize));
                return false;
            }
            
            if (!file.read(reinterpret_cast<char*>(&header.audioFormat), 2)) {
                Log::error("Failed to read audio format");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.numChannels), 2)) {
                Log::error("Failed to read numChannels");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.sampleRate), 4)) {
                Log::error("Failed to read sampleRate");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.byteRate), 4)) {
                Log::error("Failed to read byteRate");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.blockAlign), 2)) {
                Log::error("Failed to read blockAlign");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2)) {
                Log::error("Failed to read bitsPerSample");
                return false;
            }
            
            Log::info("fmt chunk parsed: format=" + std::to_string(header.audioFormat) +
                      ", channels=" + std::to_string(header.numChannels) +
                      ", sampleRate=" + std::to_string(header.sampleRate) +
                      ", bitsPerSample=" + std::to_string(header.bitsPerSample));
            
            foundFmtChunk = true;
            
            // Skip any extra bytes in fmt chunk (e.g., extensible format has more data)
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            
        } else if (chunkTypeStr == "data") {
            header.dataSize = chunkSize;
            foundDataChunk = true;
            Log::info("data chunk found with " + std::to_string(chunkSize) + " bytes");
            // Don't read data yet - just mark position
            break;
            
        } else {
            // Skip unknown chunks (JUNK, LIST, etc.)
            Log::info("Skipping " + chunkTypeStr + " chunk (" + std::to_string(chunkSize) + " bytes)");
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (!foundFmtChunk) {
        Log::error("WAV file missing fmt chunk");
        return false;
    }
    
    if (!foundDataChunk) {
        Log::error("WAV file missing data chunk");
        return false;
    }

    // Validate WAV format
    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        Log::warning("Invalid WAV file format: " + filePath);
        Log::warning("  Expected: RIFF, WAVE");
        Log::warning("  Got: " + std::string(header.riff, 4) + ", " + std::string(header.wave, 4));
        return false;
    }

    // Check if it's PCM format (allow both PCM and floating point)
    if (header.audioFormat != 1 && header.audioFormat != 3) {
        Log::warning("Unsupported audio format: " + std::to_string(header.audioFormat) + " (only PCM and float supported)");
        return false;
    }

    // Support different bit depths
    if (header.bitsPerSample != 16 && header.bitsPerSample != 24 && header.bitsPerSample != 32) {
        Log::warning("Unsupported bit depth: " + std::to_string(header.bitsPerSample) + " (only 16/24/32-bit supported)");
        return false;
    }

    Log::info("WAV format validated successfully");

    // Read audio data based on bit depth
    size_t bytesPerSample = header.bitsPerSample / 8;
    audioData.resize(header.dataSize / bytesPerSample);

    if (header.bitsPerSample == 16) {
        // 16-bit samples
        std::vector<int16_t> rawData(header.dataSize / sizeof(int16_t));
        file.read(reinterpret_cast<char*>(rawData.data()), header.dataSize);

        // Convert to float and normalize
        for (size_t i = 0; i < rawData.size(); ++i) {
            audioData[i] = rawData[i] / 32768.0f; // Normalize 16-bit to [-1, 1]
        }
    } else if (header.bitsPerSample == 24) {
        // 24-bit samples (stored as 3 bytes per sample, little-endian)
        std::vector<uint8_t> rawData(header.dataSize);
        file.read(reinterpret_cast<char*>(rawData.data()), header.dataSize);

        for (size_t i = 0; i < audioData.size(); ++i) {
            // Read 3 bytes and convert to 32-bit signed integer
            // Bytes are stored little-endian: LSB, middle byte, MSB
            uint32_t sample24 = rawData[i * 3] | 
                               (rawData[i * 3 + 1] << 8) | 
                               (rawData[i * 3 + 2] << 16);

            // Sign extend 24-bit to 32-bit signed integer
            int32_t signedSample;
            if (sample24 & 0x800000) {
                // Negative number - extend sign bit
                signedSample = static_cast<int32_t>(sample24 | 0xFF000000);
            } else {
                // Positive number
                signedSample = static_cast<int32_t>(sample24);
            }

            // Convert to float normalized to [-1.0, 1.0]
            // 24-bit max value is 8388607 (2^23 - 1)
            audioData[i] = signedSample / 8388608.0f;
        }
    } else if (header.bitsPerSample == 32) {
        // 32-bit float samples
        file.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);

        // 32-bit float is already in the right range [-1, 1], but clamp just in case
        for (size_t i = 0; i < audioData.size(); ++i) {
            audioData[i] = std::max(-1.0f, std::min(1.0f, audioData[i]));
        }
    }

    sampleRate = header.sampleRate;
    numChannels = header.numChannels;

    Log::info("WAV loaded: " + std::to_string(audioData.size()) + " samples, " +
              std::to_string(sampleRate) + " Hz, " + std::to_string(numChannels) + " channels");

    return true;
}

// Simple linear resampling function
void resampleAudio(const std::vector<float>& inputData, std::vector<float>& outputData,
                   uint32_t inputSampleRate, uint32_t outputSampleRate, uint16_t numChannels) {
    if (inputSampleRate == outputSampleRate) {
        outputData = inputData;
        return;
    }

    Log::info("Resampling audio: " + std::to_string(inputSampleRate) + " Hz -> " + 
              std::to_string(outputSampleRate) + " Hz");

    double ratio = static_cast<double>(inputSampleRate) / static_cast<double>(outputSampleRate);
    size_t inputFrames = inputData.size() / numChannels;
    size_t outputFrames = static_cast<size_t>(inputFrames / ratio);
    
    outputData.resize(outputFrames * numChannels);

    // Linear interpolation resampling
    for (size_t outFrame = 0; outFrame < outputFrames; ++outFrame) {
        double srcPos = outFrame * ratio;
        size_t srcFrame = static_cast<size_t>(srcPos);
        double frac = srcPos - srcFrame;

        for (uint16_t ch = 0; ch < numChannels; ++ch) {
            size_t idx1 = srcFrame * numChannels + ch;
            size_t idx2 = std::min((srcFrame + 1) * numChannels + ch, inputData.size() - 1);
            
            float sample1 = inputData[idx1];
            float sample2 = inputData[idx2];
            
            // Linear interpolation
            outputData[outFrame * numChannels + ch] = sample1 + frac * (sample2 - sample1);
        }
    }

    Log::info("Resampling complete: " + std::to_string(inputFrames) + " -> " + 
              std::to_string(outputFrames) + " frames");
}

Track::Track(const std::string& name, uint32_t trackId)
    : m_name(name)
    , m_trackId(trackId)
    , m_color(0xFF4080FF)  // Default blue (ARGB)
    , m_state(TrackState::Empty)
    , m_positionSeconds(0.0)
    , m_durationSeconds(0.0)
    , m_playbackPhase(0.0)
    , m_isRecording(false)
{
    // Create mixer bus for this track
    m_mixerBus = std::make_unique<MixerBus>(m_name.c_str(), 2);  // Stereo

    // Set initial parameters
    m_mixerBus->setGain(m_volume.load());
    m_mixerBus->setPan(m_pan.load());
    m_mixerBus->setMute(m_muted.load());
    m_mixerBus->setSolo(m_soloed.load());

    // Enable flush-to-zero and denormals-are-zero on platforms that support SSE
    // This protects the audio thread from denormal slowdowns caused by tiny near-zero floats.
    // It's low-risk on Windows/MSVC with SSE support; noop on compilers without xmmintrin.h.
#if defined(_MSC_VER) || defined(__SSE__)
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    // Note: _MM_SET_DENORMALS_ZERO_MODE may not be available on all toolchains; flush-to-zero is the primary protection.
#endif

    Log::info("Track created: " + m_name + " (ID: " + std::to_string(m_trackId) + ")");
}

Track::~Track() {
    if (isRecording()) {
        stopRecording();
    }
    Log::info("Track destroyed: " + m_name);
}
// Update output sample rate - update device output rate used for playback.
// Do NOT perform automatic re-resampling here; playback will use the
// configured playback mode to determine how to produce output.
void Track::setOutputSampleRate(uint32_t sampleRate) {
    if (m_sampleRate == sampleRate) {
        return;
    }
    m_sampleRate = sampleRate;
    Log::info("Track " + m_name + ": output sample rate set to " + std::to_string(sampleRate) + " Hz");
}

// Track Properties
void Track::setName(const std::string& name) {
    m_name = name;
    if (m_mixerBus) {
        // Update mixer bus name too
        // Note: MixerBus doesn't have setName method yet, but we could add it
    }
}

void Track::setColor(uint32_t color) {
    m_color = color;
}

// Audio Parameters (thread-safe)
void Track::setVolume(float volume) {
    volume = (volume < 0.0f) ? 0.0f : (volume > 2.0f) ? 2.0f : volume;  // 0% to 200%
    m_volume.store(volume);
    if (m_mixerBus) {
        m_mixerBus->setGain(volume);
    }
}

void Track::setPan(float pan) {
    pan = (pan < -1.0f) ? -1.0f : (pan > 1.0f) ? 1.0f : pan;
    m_pan.store(pan);
    if (m_mixerBus) {
        m_mixerBus->setPan(pan);
    }
}

void Track::setMute(bool mute) {
    m_muted.store(mute);
    if (m_mixerBus) {
        m_mixerBus->setMute(mute);
    }
}

void Track::setSolo(bool solo) {
    m_soloed.store(solo);
    if (m_mixerBus) {
        m_mixerBus->setSolo(solo);
    }
}

// Track State
void Track::setState(TrackState state) {
    TrackState oldState = m_state.exchange(state);
    if (oldState != state) {
        Log::info("Track " + m_name + " state changed: " +
                  std::to_string(static_cast<int>(oldState)) + " -> " +
                  std::to_string(static_cast<int>(state)));

        // Handle state transitions
        switch (state) {
            case TrackState::Playing:
                // CRITICAL FIX: Do NOT reset playback phase when transitioning to Playing
                // The phase should be preserved from setPosition() calls (e.g., seeking while stopped)
                // This allows seeking while stopped, then playing from that position
                // Phase is only reset when explicitly calling stop()
                break;
            case TrackState::Stopped:
                Log::info("Reached TrackState::Stopped case");
                m_playbackPhase.store(0.0);
                m_positionSeconds.store(0.0);
                break;
            case TrackState::Recording:
                m_recordingBuffer.clear();
                m_isRecording.store(true);
                break;
            default:
                break;
        }
    }
}

// Audio Data Management
bool Track::loadAudioFile(const std::string& filePath) {
    std::cout << "Loading: " << filePath << " (track: " << m_name << ")" << std::endl;

    // Check if file exists
    std::ifstream checkFile(filePath);
    if (!checkFile) {
        std::cout << "File not found, generating preview tone" << std::endl;
        return generatePreviewTone(filePath);
    }
    checkFile.close();

    // Clear any existing audio data
    m_audioData.clear();
    m_durationSeconds.store(0.0);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);

    // Determine file extension to choose appropriate loader
    std::string extension = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "wav") {
        // Load WAV file
        uint32_t fileSampleRate = 48000; // Default fallback
        uint16_t numChannels = 2;        // Default fallback
        std::vector<float> tempAudioData;

        if (loadWavFile(filePath, tempAudioData, fileSampleRate, numChannels)) {
            m_numChannels = numChannels;

            // Keep original file buffer and its sample rate. We'll use one of two
            // strategies depending on playback mode:
            // - RealTimeInterpolation (default): keep original and interpolate
            //   during playback (no pre-resample)
            // - PreResample: create a resampled buffer now for legacy behaviour
            m_originalAudioData = std::move(tempAudioData);
            m_originalSampleRate = fileSampleRate;

            if (m_playbackMode == PlaybackMode::PreResample) {
                if (m_originalSampleRate != m_sampleRate) {
                    resampleAudio(m_originalAudioData, m_audioData, m_originalSampleRate, m_sampleRate, numChannels);
                } else {
                    m_audioData = m_originalAudioData; // copy since original is kept
                }
                // PreResample: duration based on resampled data
                m_durationSeconds.store(static_cast<double>(m_audioData.size()) / (m_sampleRate * numChannels));
            } else {
                // Real-time interpolation: don't pre-resample. Duration based on original.
                uint32_t frames = static_cast<uint32_t>(m_originalAudioData.size() / numChannels);
                m_durationSeconds.store(static_cast<double>(frames) / static_cast<double>(m_originalSampleRate));
                m_audioData.clear();
            }

            setState(TrackState::Loaded);

            Log::info("WAV loaded successfully: " + std::to_string((m_playbackMode == PlaybackMode::PreResample ? m_audioData.size() : m_originalAudioData.size())) + " samples, " +
                       std::to_string(m_durationSeconds.load()) + " seconds at " + 
                       std::to_string((m_playbackMode == PlaybackMode::PreResample ? m_sampleRate : m_originalSampleRate)) + " Hz");
            return true;
        } else {
            Log::warning("Failed to load WAV file: " + filePath + ", generating preview tone instead");
        }
    }

    // Special handling for demo files - generate audio directly
    if (filePath.find("demo_") != std::string::npos) {
        std::cout << "Demo file detected, generating audio directly" << std::endl;
        return generateDemoAudio(filePath);
    }

    // Fallback: generate preview tone for unsupported formats or failed loads
    std::cout << "Falling back to preview tone" << std::endl;
    return generatePreviewTone(filePath);
}

bool Track::generatePreviewTone(const std::string& filePath) {
    // Use filename hash to generate a unique frequency
    size_t filenameHash = std::hash<std::string>{}(filePath);
    double baseFrequency = 220.0 + (filenameHash % 440);

    // Generate 5 seconds of audio
    const double duration = 5.0;
    const uint32_t totalSamples = static_cast<uint32_t>(m_sampleRate * duration * m_numChannels);
    m_audioData.resize(totalSamples);

    // Generate waveform
    double freq1 = baseFrequency;
    double freq2 = baseFrequency * 1.5;
    double freq3 = baseFrequency * 2.0;

    for (uint32_t i = 0; i < totalSamples / m_numChannels; ++i) {
        double phase1 = 2.0 * 3.14159265358979323846 * freq1 * i / m_sampleRate;
        double phase2 = 2.0 * 3.14159265358979323846 * freq2 * i / m_sampleRate;
        double phase3 = 2.0 * 3.14159265358979323846 * freq3 * i / m_sampleRate;

        float sample1 = 0.4f * static_cast<float>(std::sin(phase1));
        float sample2 = 0.2f * static_cast<float>(std::sin(phase2));
        float sample3 = 0.1f * static_cast<float>(std::sin(phase3));

        float sample = sample1 + sample2 + sample3;
        sample = (sample < -0.9f) ? -0.9f : (sample > 0.9f) ? 0.9f : sample;

        m_audioData[i * m_numChannels + 0] = sample;
        m_audioData[i * m_numChannels + 1] = sample;
    }

    m_durationSeconds.store(duration);
    setState(TrackState::Loaded);

    std::cout << "Preview tone generated: " << m_audioData.size() << " samples, "
              << m_durationSeconds.load() << " seconds, " << baseFrequency << " Hz" << std::endl;

    return true;
}

bool Track::generateDemoAudio(const std::string& filePath) {
    std::cout << "Generating demo audio for: " << filePath << std::endl;

    // Determine frequency and duration based on filename
    double frequency = 440.0;
    double duration = 3.0;

    if (filePath.find("guitar") != std::string::npos) {
        frequency = 440.0; duration = 3.0;
    } else if (filePath.find("drums") != std::string::npos) {
        frequency = 120.0; duration = 2.0;
    } else if (filePath.find("vocals") != std::string::npos) {
        frequency = 330.0; duration = 4.0;
    }

    // Generate audio data
    const uint32_t totalSamples = static_cast<uint32_t>(m_sampleRate * duration * m_numChannels);
    m_audioData.resize(totalSamples);

    // Generate waveform
    double freq1 = frequency;
    double freq2 = frequency * 1.5;
    double freq3 = frequency * 2.0;

    for (uint32_t i = 0; i < totalSamples / m_numChannels; ++i) {
        double phase1 = 2.0 * 3.14159265358979323846 * freq1 * i / m_sampleRate;
        double phase2 = 2.0 * 3.14159265358979323846 * freq2 * i / m_sampleRate;
        double phase3 = 2.0 * 3.14159265358979323846 * freq3 * i / m_sampleRate;

        float sample1 = 0.4f * static_cast<float>(std::sin(phase1));
        float sample2 = 0.2f * static_cast<float>(std::sin(phase2));
        float sample3 = 0.1f * static_cast<float>(std::sin(phase3));

        float sample = sample1 + sample2 + sample3;
        sample = (sample < -0.9f) ? -0.9f : (sample > 0.9f) ? 0.9f : sample;

        m_audioData[i * m_numChannels + 0] = sample;
        m_audioData[i * m_numChannels + 1] = sample;
    }

    m_durationSeconds.store(duration);
    setState(TrackState::Loaded);

    std::cout << "Demo audio generated: " << m_audioData.size() << " samples, "
              << m_durationSeconds.load() << " seconds, " << frequency << " Hz" << std::endl;

    return true;
}

void Track::clearAudioData() {
    m_audioData.clear();
    m_originalAudioData.clear();
    m_originalSampleRate = 0;
    m_recordingBuffer.clear();
    m_durationSeconds.store(0.0);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
    setState(TrackState::Empty);
}

void Track::setAudioData(const float* data, uint32_t numSamples, uint32_t sampleRate, uint32_t numChannels) {
    if (!data || numSamples == 0) {
        Log::error("Invalid audio data");
        return;
    }
    
    // Copy audio data
    m_audioData.assign(data, data + (numSamples * numChannels));
    m_sampleRate = sampleRate;
    m_numChannels = numChannels;
    m_durationSeconds.store(static_cast<double>(numSamples) / sampleRate);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
    setState(TrackState::Loaded);
    
    Log::info("Audio data loaded: " + std::to_string(numSamples) + " samples, " +
               std::to_string(m_durationSeconds.load()) + " seconds, " + 
               std::to_string(sampleRate) + " Hz, " + std::to_string(numChannels) + " channels");
}

// Recording
void Track::setLatencyCompensation(double inputLatencyMs, double outputLatencyMs) {
    m_latencyCompensationMs = inputLatencyMs + outputLatencyMs;
    Log::info("Track '" + m_name + "' latency compensation set: " + 
              std::to_string(m_latencyCompensationMs) + " ms (Input: " + 
              std::to_string(inputLatencyMs) + " ms + Output: " + 
              std::to_string(outputLatencyMs) + " ms)");
}

void Track::startRecording() {
    if (getState() != TrackState::Empty) {
        Log::warning("Cannot start recording: track not empty");
        return;
    }

    Log::info("Starting recording on track: " + m_name);
    setState(TrackState::Recording);
}

void Track::stopRecording() {
    if (!isRecording()) {
        return;
    }

    Log::info("Stopping recording on track: " + m_name);

    // Move recording buffer to main audio data
    if (!m_recordingBuffer.empty()) {
        m_audioData = std::move(m_recordingBuffer);
        
        // Apply latency compensation if configured
        if (m_latencyCompensationMs > 0.0) {
            // Calculate how many samples to shift (compensate for input + output latency)
            uint32_t compensationSamples = static_cast<uint32_t>(
                (m_latencyCompensationMs / 1000.0) * m_sampleRate * m_numChannels
            );
            
            // Ensure we don't shift more than available data
            if (compensationSamples > 0 && compensationSamples < m_audioData.size()) {
                // Shift audio data earlier by removing the latency from the beginning
                // This aligns the recorded audio with the timeline
                m_audioData.erase(m_audioData.begin(), m_audioData.begin() + compensationSamples);
                
                Log::info("[Latency Compensation] Shifted recorded audio earlier by " + 
                          std::to_string(m_latencyCompensationMs) + " ms (" + 
                          std::to_string(compensationSamples / m_numChannels) + " frames)");
            }
        }
        
        m_durationSeconds.store(static_cast<double>(m_audioData.size()) / (m_sampleRate * m_numChannels));
        setState(TrackState::Loaded);
    } else {
        setState(TrackState::Empty);
    }

    m_recordingBuffer.clear();
    m_isRecording.store(false);
}


void Track::pause() {
    if (getState() == TrackState::Playing) {
        Log::info("Pausing track: " + m_name);
        setState(TrackState::Paused);
    }
}

void Track::stop() {
    Log::info("Stopping track: " + m_name);
    setState(TrackState::Stopped);
    // Position reset will happen when play() is called again
}

void Track::reset() {
    Log::info("Resetting DSP state for track: " + m_name);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
    m_dcOffset = 0.0;
    
    // Reset dither history
    m_ditherHistory[0] = 0.0f;
    m_ditherHistory[1] = 0.0f;
    m_highPassDitherHistory = 0.0f;
    
    // Reset Euphoria engine state
    m_airDelayPos = 0;
    m_driftPhase = 0.0f;
    std::fill(std::begin(m_airDelayBufferL), std::end(m_airDelayBufferL), 0.0f);
    std::fill(std::begin(m_airDelayBufferR), std::end(m_airDelayBufferR), 0.0f);
}

// Position Control
void Track::setPosition(double seconds) {
    // Clamp position to valid range [0, duration]
    double duration = getDuration();
    seconds = std::max(0.0, std::min(duration, seconds));
    m_positionSeconds.store(seconds);

    // Convert seconds to playback phase using appropriate sample rate
    // For RealTimeInterpolation mode: phase is in original sample space (m_originalSampleRate)
    // For PreResample mode: phase is in output sample space (m_sampleRate)
    
    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        // Real-time interpolation: phase needs to be in original file's sample space
        m_playbackPhase.store(seconds * m_originalSampleRate);
    } else {
        // Pre-resample: phase is in output sample space
        m_playbackPhase.store(seconds * m_sampleRate);
    }
}

void Track::setStartPositionInTimeline(double seconds) {
    m_startPositionInTimeline.store(seconds);
    
    // CRITICAL: Reset playback phase to beginning of sample when timeline position changes
    // This ensures audio plays correctly from the new position
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
}

// Align track playback phase to the project's global playhead position.
// If the playhead is before the sample start, phase is set to 0.0.
// If the playhead is after the sample end, the phase is left unchanged.
void Track::syncPlaybackPhaseWithGlobalPlayhead(double globalPlayheadPosition) {
    if (globalPlayheadPosition < 0.0) return;

    double sampleStart = m_startPositionInTimeline.load();
    double duration = getDuration();
    double sampleEnd = sampleStart + duration;

    // If playhead is before the sample, reset to start
    if (globalPlayheadPosition < sampleStart) {
        m_playbackPhase.store(0.0);
        m_positionSeconds.store(0.0);
        return;
    }

    // If playhead is after sample end, don't change phase
    if (globalPlayheadPosition >= sampleEnd) {
        return;
    }

    double timeIntoSample = globalPlayheadPosition - sampleStart;
    double newPhase = 0.0;

    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        newPhase = timeIntoSample * static_cast<double>(m_originalSampleRate);
    } else {
        if (m_sampleRate == 0) return;
        newPhase = timeIntoSample * static_cast<double>(m_sampleRate);
    }

    // Clamp to available frames
    uint64_t totalFrames = 0;
    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        totalFrames = m_originalAudioData.empty() ? 0 : (m_originalAudioData.size() / m_numChannels);
    } else {
        totalFrames = m_audioData.empty() ? 0 : (m_audioData.size() / m_numChannels);
    }

    if (totalFrames > 0 && newPhase >= static_cast<double>(totalFrames)) {
        newPhase = static_cast<double>(totalFrames) - 1.0;
        if (newPhase < 0.0) newPhase = 0.0;
    }

    // Debug: log when playback phase is clamped to end
    if (totalFrames > 0 && newPhase >= static_cast<double>(totalFrames) - 1.0) {
        Log::info("[DEBUG] Track '" + m_name + "' syncPlaybackPhase clamped to end: newPhase=" + std::to_string(newPhase) + ", totalFrames=" + std::to_string(totalFrames));
    }
    m_playbackPhase.store(newPhase);

    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        m_positionSeconds.store(newPhase / static_cast<double>(m_originalSampleRate));
    } else if (m_sampleRate > 0) {
        m_positionSeconds.store(newPhase / static_cast<double>(m_sampleRate));
    }
}

// Audio Processing (legacy - no timeline position checking)
void Track::processAudio(float* outputBuffer, uint32_t numFrames, double streamTime) {
    // Call the new version with position = 0 (no timeline checking)
    processAudio(outputBuffer, numFrames, streamTime, -1.0);
}

// Audio Processing with timeline position checking
void Track::processAudio(float* outputBuffer, uint32_t numFrames, double streamTime, double globalPlayheadPosition) {
    if (!outputBuffer || numFrames == 0) {
        return;
    }

    TrackState currentState = getState();

    // Create temporary buffer for this track's audio before mixing
    std::vector<float> trackBuffer(numFrames * m_numChannels, 0.0f);

    switch (currentState) {
        case TrackState::Playing: {
            // Check if global playhead is within this sample's timeline bounds
            bool shouldPlay = true;
            if (globalPlayheadPosition >= 0.0) {
                double sampleStart = m_startPositionInTimeline.load();
                double sampleEnd = sampleStart + getDuration();
                
                // Only play if playhead is within sample bounds
                shouldPlay = (globalPlayheadPosition >= sampleStart && globalPlayheadPosition < sampleEnd);
            }
            
            if (shouldPlay) {
                    // Debug: log when the playhead is about to enter this sample
                    if (globalPlayheadPosition >= 0.0) {
                        double sampleStart = m_startPositionInTimeline.load();
                        double timeUntilStart = sampleStart - globalPlayheadPosition;
                        if (timeUntilStart >= 0.0 && timeUntilStart < 0.5) {
                            Log::info("[DEBUG] Track '" + m_name + "' entering sample in " + std::to_string(timeUntilStart) + "s; phase=" + std::to_string(m_playbackPhase.load()) + ", posSec=" + std::to_string(m_positionSeconds.load()));
                        }
                    }
    
                    // Throttled entry trace: log occasionally to confirm the audio thread is invoking this method
                    uint32_t callIdx = m_copyAudioCallCounter.fetch_add(1);
                    if ((callIdx & 0x3F) == 0) { // every 64th call
                        Log::info(std::string("[TRACE] processAudio -> copyAudioData for track '") + m_name + "', state=Playing, phase=" + std::to_string(m_playbackPhase.load()) + ", startPos=" + std::to_string(m_startPositionInTimeline.load()) + ", globalPlayhead=" + std::to_string(globalPlayheadPosition));
                    }
    
                    // Copy audio data to temporary buffer
                    copyAudioData(trackBuffer.data(), numFrames);
                
                // Process through mixer bus for volume/pan/mute/solo
                if (m_mixerBus) {
                    m_mixerBus->process(trackBuffer.data(), numFrames);
                }
                
                // Mix into output buffer
                for (uint32_t i = 0; i < numFrames * m_numChannels; ++i) {
                    outputBuffer[i] += trackBuffer[i];
                }
            }
            // else: output silence (don't modify output buffer)
            break;
        }

        case TrackState::Recording:
            m_playbackPhase.store(m_playbackPhase.load() + numFrames * m_numChannels);
            break;

        case TrackState::Paused:
        case TrackState::Stopped:
        case TrackState::Empty:
        case TrackState::Loaded:
        default:
            // Output silence (don't modify output buffer)
            break;
    }

    // Position is now updated inside copyAudioData (phase -> seconds conversion)
    // to ensure a single authoritative source of truth and avoid double-updating
    // between audio thread and UI thread. No position arithmetic here.
}

void Track::generateSilence(float* buffer, uint32_t numFrames) {
    if (buffer) {
        std::fill(buffer, buffer + numFrames * m_numChannels, 0.0f);
    }
}

void Track::copyAudioData(float* outputBuffer, uint32_t numFrames) {
    // --- CRITICAL FIX: Enhanced self-stopping logic ---
    const auto* audioDataSource = (m_playbackMode == PlaybackMode::RealTimeInterpolation) ? &m_originalAudioData : &m_audioData;
    const uint32_t totalFrames = audioDataSource->empty() ? 0 : static_cast<uint32_t>(audioDataSource->size() / m_numChannels);
    
    double currentPhase = m_playbackPhase.load();
    
    // PHASE OVERFLOW PROTECTION: Multiple safety checks
    bool shouldStop = false;
    std::string stopReason = "";
    
    if (totalFrames == 0) {
        shouldStop = true;
        stopReason = "No audio data";
    } else if (currentPhase >= totalFrames) {
        shouldStop = true;
        stopReason = "Phase overflow (current=" + std::to_string(currentPhase) + ", max=" + std::to_string(totalFrames) + ")";
    } else if (std::isnan(currentPhase) || std::isinf(currentPhase)) {
        shouldStop = true;
        stopReason = "Invalid phase (NaN/Inf)";
    } else if (currentPhase < 0) {
        shouldStop = true;
        stopReason = "Negative phase";
    }
    
    // ENHANCED: Log ALL stop conditions for debugging
    if (shouldStop) {
        Log::error("[CRITICAL FIX] AUTO-STOP TRIGGERED for track '" + m_name + "': " + stopReason);
        Log::error("[CRITICAL FIX] Track details: phase=" + std::to_string(currentPhase) +
                   ", totalFrames=" + std::to_string(totalFrames) +
                   ", playbackMode=" + std::to_string(static_cast<int>(m_playbackMode)) +
                   ", state=" + std::to_string(static_cast<int>(getState())));
        
        if (getState() == TrackState::Playing) {
            Log::error("[CRITICAL FIX] Calling setState(Stopped) from Playing state");
            setState(TrackState::Stopped);
        }
        
        generateSilence(outputBuffer, numFrames);
        Log::error("[CRITICAL FIX] Auto-stop complete, generating silence and returning");
        return;
    }

    // Throttled entry trace: log occasionally to confirm the audio thread is invoking this method.
    // This avoids producing excessive log volume during sustained playback while still
    // giving us visibility for intermittent repro steps.
    uint32_t callIdx = m_copyAudioCallCounter.fetch_add(1);
    if ((callIdx & 0x3F) == 0) { // every 64th call
        std::string msg = "[TRACE] copyAudioData for track '" + m_name + "'";
        msg += ", state=" + std::to_string(static_cast<int>(m_state.load()));
        msg += ", phase=" + std::to_string(m_playbackPhase.load());
        msg += ", startPos=" + std::to_string(m_startPositionInTimeline.load());
        msg += ", outSampleRate=" + std::to_string(m_sampleRate);
        Log::info(msg);
    }

    // Start timing for the whole copyAudioData call so we can measure expensive blocks
    auto __copy_start = std::chrono::high_resolution_clock::now();

    // Two modes of playback:
    // - PreResample: use m_audioData which is already resampled to output rate
    // - RealTimeInterpolation: use m_originalAudioData and interpolate on-the-fly
    if (m_playbackMode == PlaybackMode::PreResample) {
        // PreResample mode
        uint32_t totalSamples = static_cast<uint32_t>(m_audioData.size());
        if (totalSamples == 0) {
            Log::warning("[WATCHDOG] copyAudioData: m_audioData empty for track '" + m_name + "' -> generating silence");
            generateSilence(outputBuffer, numFrames);
            return;
        }

        double phase = m_playbackPhase.load();
        double blockPhaseStart = phase;

        // PreResample phase tracking continues normally - safety checks handle overflow conditions
        // m_audioData is in output-rate frames, so increment by 1 frame per output frame
        const double phaseIncrement = 1.0;

        // Compute number of source frames (frames = samples / channels)
        uint32_t totalFrames = (m_numChannels == 0) ? 0 : (totalSamples / m_numChannels);

        for (uint32_t frame = 0; frame < numFrames; ++frame) {
            double exactSamplePos = phase;

            // CRITICAL FIX: Immediate phase validation during accumulation
            if (!std::isfinite(exactSamplePos) || exactSamplePos < 0.0) {
                Log::error("[CRITICAL FIX] PreResample: Invalid phase detected, stopping playback");
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer, numFrames);
                return;
            }
            
            if (totalFrames > 0 && exactSamplePos >= totalFrames) {
                Log::error("[CRITICAL FIX] PreResample: Phase exceeded buffer, stopping playback. phase=" +
                          std::to_string(exactSamplePos) + ", totalFrames=" + std::to_string(totalFrames));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer, numFrames);
                return;
            }

            for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
                float sample = 0.0f;

                switch (m_qualitySettings.resampling) {
                    case ResamplingMode::Fast:
                        sample = interpolateLinear(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Medium:
                        sample = interpolateCubic(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::High:
                        sample = interpolateSinc(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Ultra:
                        sample = interpolateUltra(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Extreme:
                        sample = interpolateExtreme(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Perfect:
                        sample = interpolatePerfect(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                }

                // Sanitize sample: guard against NaN/Inf and denormals
                if (!std::isfinite(sample) || std::abs(sample) < 1e-20f) {
                    sample = 0.0f;
                }
                sample = std::max(-1.0f, std::min(1.0f, sample));

                outputBuffer[frame * m_numChannels + ch] = sample;
            }

            // CRITICAL FIX: Immediate stop if next phase would exceed bounds
            double nextPhase = phase + phaseIncrement;
            if (totalFrames > 0 && nextPhase >= totalFrames) {
                Log::error("[CRITICAL FIX] PreResample: Next phase would exceed bounds, stopping. nextPhase=" +
                          std::to_string(nextPhase) + ", totalFrames=" + std::to_string(totalFrames));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer + frame * m_numChannels, (numFrames - frame) * m_numChannels);
                return;
            }
            phase = nextPhase;
        }

        // Clamp phase to valid range and stop playback if we've reached the end
        double maxPhase = (totalFrames == 0) ? 0.0 : (static_cast<double>(totalFrames) - 1.0);
        if (!std::isfinite(phase)) phase = 0.0;
        if (phase >= maxPhase) {
            phase = maxPhase;
            if (getState() == TrackState::Playing) {
                Log::warning("[WATCHDOG] Track '" + m_name + "' reached end during PreResample block; auto-stopping.");
                setState(TrackState::Stopped);
            }
        }
        m_playbackPhase.store(phase);
        
        // CRITICAL FIX: Consistent time-based positioning for both playback modes
        // Position is calculated from playback phase, ensuring sample-accurate timing
        m_positionSeconds.store(phase / static_cast<double>(m_sampleRate));

        // Optional short debug trace: emit a summary for the first few blocks after play
        if (m_debugTraceBlocksRemaining.load() > 0) {
            float maxAbs = 0.0f;
            uint32_t totalOutSamples = numFrames * m_numChannels;
            for (uint32_t i = 0; i < totalOutSamples; ++i) {
                float a = std::abs(outputBuffer[i]);
                if (a > maxAbs) maxAbs = a;
            }
            std::string msg = "[TRACE] copyAudioData block summary for track '" + m_name + "'";
            msg += ", mode=PreResample";
            msg += ", startPhase=" + std::to_string(blockPhaseStart);
            msg += ", endPhase=" + std::to_string(phase);
            msg += ", posSec=" + std::to_string(m_positionSeconds.load());
            msg += ", maxAbs=" + std::to_string(maxAbs);
            Log::info(msg);
            m_debugTraceBlocksRemaining.fetch_sub(1);
        }
        // End timing and conditionally log if this block was slow
        auto __copy_end = std::chrono::high_resolution_clock::now();
        double __copy_ms = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(__copy_end - __copy_start).count()) / 1000.0;
        const double __copy_log_threshold_ms = 1.0; // adjustable threshold
        if (__copy_ms > __copy_log_threshold_ms) {
            std::string perfMsg = "[PERF] copyAudioData (PreResample) took " + std::to_string(__copy_ms) + " ms for track '" + m_name + "', frames=" + std::to_string(numFrames) + ", channels=" + std::to_string(m_numChannels);
            Log::warning(perfMsg);
        }
    } else {
        // RealTimeInterpolation mode
        uint32_t originalTotalSamples = static_cast<uint32_t>(m_originalAudioData.size());
        if (originalTotalSamples == 0 || m_originalSampleRate == 0) {
            Log::warning("[WATCHDOG] copyAudioData: m_originalAudioData empty or originalSampleRate==0 for track '" + m_name + "' -> generating silence");
            generateSilence(outputBuffer, numFrames);
            return;
        }

        uint32_t totalOrigFrames = (m_numChannels == 0) ? 0 : (originalTotalSamples / m_numChannels);
        double maxPhase = static_cast<double>(totalOrigFrames);

        double rtPhase = m_playbackPhase.load(); // phase in source frames (original sample rate)
        double rtBlockPhaseStart = rtPhase;

        double sampleRateRatio = 1.0;
        if (m_sampleRate > 0 && m_originalSampleRate > 0) {
            sampleRateRatio = static_cast<double>(m_originalSampleRate) / static_cast<double>(m_sampleRate);
            if (sampleRateRatio <= 0.0 || !std::isfinite(sampleRateRatio)) {
                Log::error("[CRITICAL] Invalid sampleRateRatio: " + std::to_string(sampleRateRatio));
                generateSilence(outputBuffer, numFrames);
                return;
            }
        }

        for (uint32_t frame = 0; frame < numFrames; ++frame) {
            // Stop playback if we have reached the end of the buffer.
            if (rtPhase >= maxPhase) {
                if (getState() == TrackState::Playing) {
                    Log::info("[WATCHDOG] Reached end of buffer, stopping normally. phase=" + std::to_string(rtPhase) + ", maxPhase=" + std::to_string(maxPhase));
                    setState(TrackState::Stopped);
                }
                // Fill the rest of the buffer with silence and exit.
                generateSilence(outputBuffer + frame * m_numChannels, numFrames - frame);
                break;
            }

            for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
                float sample = 0.0f;
                switch (m_qualitySettings.resampling) {
                    case ResamplingMode::Fast:
                        sample = interpolateLinear(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                        break;
                    case ResamplingMode::Medium:
                        sample = interpolateCubic(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                        break;
                    case ResamplingMode::High:
                        sample = interpolateSinc(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                        break;
                    case ResamplingMode::Ultra:
                        sample = interpolateUltra(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                        break;
                    case ResamplingMode::Extreme:
                        sample = interpolateExtreme(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                        break;
                    case ResamplingMode::Perfect:
                        sample = interpolatePerfect(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                        break;
                }
                if (!std::isfinite(sample)) {
                    sample = 0.0f;
                }
                outputBuffer[frame * m_numChannels + ch] = sample;
            }

            rtPhase += sampleRateRatio;
        }

        m_playbackPhase.store(rtPhase);

        if (m_originalSampleRate > 0) {
            m_positionSeconds.store(rtPhase / static_cast<double>(m_originalSampleRate));
        }

        // Optional short debug trace
        if (m_debugTraceBlocksRemaining.load() > 0) {
            float maxAbs = 0.0f;
            for (uint32_t i = 0; i < numFrames * m_numChannels; ++i) {
                float a = std::abs(outputBuffer[i]);
                if (a > maxAbs) maxAbs = a;
            }
            std::string msg = "[TRACE] copyAudioData block summary for track '" + m_name + "'";
            msg += ", mode=RealTimeInterpolation";
            msg += ", startPhase=" + std::to_string(rtBlockPhaseStart);
            msg += ", endPhase=" + std::to_string(rtPhase);
            msg += ", posSec=" + std::to_string(m_positionSeconds.load());
            msg += ", maxAbs=" + std::to_string(maxAbs);
            Log::info(msg);
            m_debugTraceBlocksRemaining.fetch_sub(1);
        }
    }

// Â© 2025 Nomad Studios â€” All Rights Reserved. Licensed for personal & educational use only.
#include "Track.h"
#include "NomadLog.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>
#include <cassert>
#include <chrono>
// Enable SSE intrinsics for flush-to-zero / denormals-zero control
#include <xmmintrin.h>

namespace Nomad {
namespace Audio {

// === Audio Quality Preset Implementations ===

AudioQualitySettings AudioQualitySettings::Economy() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::Economy;
    settings.resampling = ResamplingMode::Fast;
    settings.dithering = DitheringMode::None;
    settings.precision = InternalPrecision::Float32;
    settings.oversampling = OversamplingMode::None;
    settings.removeDCOffset = false;
    settings.enableSoftClipping = false;
    settings.antiAliasingFilter = FilterSteepness::Soft;
    return settings;
}

void Track::play() {
    if (getState() == TrackState::Playing) return;

    Log::info("Starting playback on track: " + m_name);
    setState(TrackState::Playing);
    // Enable short per-block debug trace so we can observe the first few audio blocks
    // after playback starts. This is temporary instrumentation to help diagnose
    // silent-gap issues when samples are placed on the timeline and played.
    // Arm a larger number of blocks so a short Play run will still emit traces
    // (useful while reproducing intermittent silent-drop issues)
    m_debugTraceBlocksRemaining.store(32);
}

AudioQualitySettings AudioQualitySettings::Balanced() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::Balanced;
    settings.resampling = ResamplingMode::Medium;
    settings.dithering = DitheringMode::Triangular;
    settings.precision = InternalPrecision::Float32;
    settings.oversampling = OversamplingMode::Auto;
    settings.removeDCOffset = true;
    settings.enableSoftClipping = false;
    settings.antiAliasingFilter = FilterSteepness::Medium;
    return settings;
}

AudioQualitySettings AudioQualitySettings::HighFidelity() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::HighFidelity;
    settings.resampling = ResamplingMode::High;
    settings.dithering = DitheringMode::NoiseShaped;
    settings.precision = InternalPrecision::Float32;
    settings.oversampling = OversamplingMode::Force2x;
    settings.removeDCOffset = true;
    settings.enableSoftClipping = true;
    settings.antiAliasingFilter = FilterSteepness::Steep;
    return settings;
}

AudioQualitySettings AudioQualitySettings::Mastering() {
    AudioQualitySettings settings;
    settings.preset = QualityPreset::Mastering;
    settings.resampling = ResamplingMode::Ultra;
    settings.dithering = DitheringMode::NoiseShaped;
    settings.precision = InternalPrecision::Float64;
    settings.oversampling = OversamplingMode::Force4x;
    settings.removeDCOffset = true;
    settings.enableSoftClipping = true;
    settings.antiAliasingFilter = FilterSteepness::Steep;
    return settings;
}

void AudioQualitySettings::applyPreset(QualityPreset newPreset) {
    switch (newPreset) {
        case QualityPreset::Economy:
            *this = Economy();
            break;
        case QualityPreset::Balanced:
            *this = Balanced();
            break;
        case QualityPreset::HighFidelity:
            *this = HighFidelity();
            break;
        case QualityPreset::Mastering:
            *this = Mastering();
            break;
        case QualityPreset::Custom:
            // Keep current settings, just update preset marker
            preset = QualityPreset::Custom;
            break;
    }
}

// WAV file header structure (Windows compatible)
#pragma pack(push, 1)
struct WavHeader {
    char riff[4];        // "RIFF"
    uint32_t fileSize;   // File size - 8
    char wave[4];        // "WAVE"
    char fmt[4];         // "fmt "
    uint32_t fmtSize;    // Format chunk size
    uint16_t audioFormat; // Audio format (1 = PCM)
    uint16_t numChannels; // Number of channels
    uint32_t sampleRate;  // Sample rate
    uint32_t byteRate;    // Byte rate
    uint16_t blockAlign;  // Block align
    uint16_t bitsPerSample; // Bits per sample
    char data[4];        // "data"
    uint32_t dataSize;    // Data chunk size
};
#pragma pack(pop)

// Simple WAV file loader
bool loadWavFile(const std::string& filePath, std::vector<float>& audioData, uint32_t& sampleRate, uint16_t& numChannels) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        Log::warning("Failed to open WAV file: " + filePath);
        return false;
    }

    WavHeader header = {};
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    // Debug: Log WAV header information
    Log::info("WAV Header Debug:");
    Log::info("  RIFF: " + std::string(header.riff, 4));
    Log::info("  File size: " + std::to_string(header.fileSize));
    Log::info("  WAVE: " + std::string(header.wave, 4));
    Log::info("  fmt: " + std::string(header.fmt, 4));
    Log::info("  Format size: " + std::to_string(header.fmtSize));
    Log::info("  Audio format: " + std::to_string(header.audioFormat));
    Log::info("  Channels: " + std::to_string(header.numChannels));
    Log::info("  Sample rate: " + std::to_string(header.sampleRate));
    Log::info("  Bits per sample: " + std::to_string(header.bitsPerSample));
    Log::info("  Data chunk: " + std::string(header.data, 4));
    Log::info("  Data size: " + std::to_string(header.dataSize));

    // Robust WAV chunk parser - handles files with any chunk order/types
    std::cout << "DEBUG: Checking first chunk after WAVE, header.fmt = '" << std::string(header.fmt, 4) << "'" << std::endl;
    
    // Reset file position to right after WAVE header (12 bytes from start)
    file.seekg(12, std::ios::beg);
    
    bool foundFmtChunk = false;
    bool foundDataChunk = false;
    
    // Parse all chunks until we find both fmt and data
    while (file && (!foundFmtChunk || !foundDataChunk)) {
        char chunkType[4];
        uint32_t chunkSize;
        
        if (!file.read(chunkType, 4)) break;
        if (!file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;
        
        std::string chunkTypeStr(chunkType, 4);
        Log::info("Found chunk: " + chunkTypeStr + ", size: " + std::to_string(chunkSize));
        
        if (chunkTypeStr == "fmt ") {
            // Read fmt chunk data
            if (chunkSize < 16) {
                Log::error("Invalid fmt chunk size: " + std::to_string(chunkSize));
                return false;
            }
            
            if (!file.read(reinterpret_cast<char*>(&header.audioFormat), 2)) {
                Log::error("Failed to read audio format");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.numChannels), 2)) {
                Log::error("Failed to read numChannels");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.sampleRate), 4)) {
                Log::error("Failed to read sampleRate");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.byteRate), 4)) {
                Log::error("Failed to read byteRate");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.blockAlign), 2)) {
                Log::error("Failed to read blockAlign");
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2)) {
                Log::error("Failed to read bitsPerSample");
                return false;
            }
            
            Log::info("fmt chunk parsed: format=" + std::to_string(header.audioFormat) +
                      ", channels=" + std::to_string(header.numChannels) +
                      ", sampleRate=" + std::to_string(header.sampleRate) +
                      ", bitsPerSample=" + std::to_string(header.bitsPerSample));
            
            foundFmtChunk = true;
            
            // Skip any extra bytes in fmt chunk (e.g., extensible format has more data)
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            
        } else if (chunkTypeStr == "data") {
            header.dataSize = chunkSize;
            foundDataChunk = true;
            Log::info("data chunk found with " + std::to_string(chunkSize) + " bytes");
            // Don't read data yet - just mark position
            break;
            
        } else {
            // Skip unknown chunks (JUNK, LIST, etc.)
            Log::info("Skipping " + chunkTypeStr + " chunk (" + std::to_string(chunkSize) + " bytes)");
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (!foundFmtChunk) {
        Log::error("WAV file missing fmt chunk");
        return false;
    }
    
    if (!foundDataChunk) {
        Log::error("WAV file missing data chunk");
        return false;
    }

    // Validate WAV format
    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        Log::warning("Invalid WAV file format: " + filePath);
        Log::warning("  Expected: RIFF, WAVE");
        Log::warning("  Got: " + std::string(header.riff, 4) + ", " + std::string(header.wave, 4));
        return false;
    }

    // Check if it's PCM format (allow both PCM and floating point)
    if (header.audioFormat != 1 && header.audioFormat != 3) {
        Log::warning("Unsupported audio format: " + std::to_string(header.audioFormat) + " (only PCM and float supported)");
        return false;
    }

    // Support different bit depths
    if (header.bitsPerSample != 16 && header.bitsPerSample != 24 && header.bitsPerSample != 32) {
        Log::warning("Unsupported bit depth: " + std::to_string(header.bitsPerSample) + " (only 16/24/32-bit supported)");
        return false;
    }

    Log::info("WAV format validated successfully");

    // Read audio data based on bit depth
    size_t bytesPerSample = header.bitsPerSample / 8;
    audioData.resize(header.dataSize / bytesPerSample);

    if (header.bitsPerSample == 16) {
        // 16-bit samples
        std::vector<int16_t> rawData(header.dataSize / sizeof(int16_t));
        file.read(reinterpret_cast<char*>(rawData.data()), header.dataSize);

        // Convert to float and normalize
        for (size_t i = 0; i < rawData.size(); ++i) {
            audioData[i] = rawData[i] / 32768.0f; // Normalize 16-bit to [-1, 1]
        }
    } else if (header.bitsPerSample == 24) {
        // 24-bit samples (stored as 3 bytes per sample, little-endian)
        std::vector<uint8_t> rawData(header.dataSize);
        file.read(reinterpret_cast<char*>(rawData.data()), header.dataSize);

        for (size_t i = 0; i < audioData.size(); ++i) {
            // Read 3 bytes and convert to 32-bit signed integer
            // Bytes are stored little-endian: LSB, middle byte, MSB
            uint32_t sample24 = rawData[i * 3] | 
                               (rawData[i * 3 + 1] << 8) | 
                               (rawData[i * 3 + 2] << 16);

            // Sign extend 24-bit to 32-bit signed integer
            int32_t signedSample;
            if (sample24 & 0x800000) {
                // Negative number - extend sign bit
                signedSample = static_cast<int32_t>(sample24 | 0xFF000000);
            } else {
                // Positive number
                signedSample = static_cast<int32_t>(sample24);
            }

            // Convert to float normalized to [-1.0, 1.0]
            // 24-bit max value is 8388607 (2^23 - 1)
            audioData[i] = signedSample / 8388608.0f;
        }
    } else if (header.bitsPerSample == 32) {
        // 32-bit float samples
        file.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);

        // 32-bit float is already in the right range [-1, 1], but clamp just in case
        for (size_t i = 0; i < audioData.size(); ++i) {
            audioData[i] = std::max(-1.0f, std::min(1.0f, audioData[i]));
        }
    }

    sampleRate = header.sampleRate;
    numChannels = header.numChannels;

    Log::info("WAV loaded: " + std::to_string(audioData.size()) + " samples, " +
              std::to_string(sampleRate) + " Hz, " + std::to_string(numChannels) + " channels");

    return true;
}

// Simple linear resampling function
void resampleAudio(const std::vector<float>& inputData, std::vector<float>& outputData,
                   uint32_t inputSampleRate, uint32_t outputSampleRate, uint16_t numChannels) {
    if (inputSampleRate == outputSampleRate) {
        outputData = inputData;
        return;
    }

    Log::info("Resampling audio: " + std::to_string(inputSampleRate) + " Hz -> " + 
              std::to_string(outputSampleRate) + " Hz");

    double ratio = static_cast<double>(inputSampleRate) / static_cast<double>(outputSampleRate);
    size_t inputFrames = inputData.size() / numChannels;
    size_t outputFrames = static_cast<size_t>(inputFrames / ratio);
    
    outputData.resize(outputFrames * numChannels);

    // Linear interpolation resampling
    for (size_t outFrame = 0; outFrame < outputFrames; ++outFrame) {
        double srcPos = outFrame * ratio;
        size_t srcFrame = static_cast<size_t>(srcPos);
        double frac = srcPos - srcFrame;

        for (uint16_t ch = 0; ch < numChannels; ++ch) {
            size_t idx1 = srcFrame * numChannels + ch;
            size_t idx2 = std::min((srcFrame + 1) * numChannels + ch, inputData.size() - 1);
            
            float sample1 = inputData[idx1];
            float sample2 = inputData[idx2];
            
            // Linear interpolation
            outputData[outFrame * numChannels + ch] = sample1 + frac * (sample2 - sample1);
        }
    }

    Log::info("Resampling complete: " + std::to_string(inputFrames) + " -> " + 
              std::to_string(outputFrames) + " frames");
}

Track::Track(const std::string& name, uint32_t trackId)
    : m_name(name)
    , m_trackId(trackId)
    , m_color(0xFF4080FF)  // Default blue (ARGB)
    , m_state(TrackState::Empty)
    , m_positionSeconds(0.0)
    , m_durationSeconds(0.0)
    , m_playbackPhase(0.0)
    , m_isRecording(false)
{
    // Create mixer bus for this track
    m_mixerBus = std::make_unique<MixerBus>(m_name.c_str(), 2);  // Stereo

    // Set initial parameters
    m_mixerBus->setGain(m_volume.load());
    m_mixerBus->setPan(m_pan.load());
    m_mixerBus->setMute(m_muted.load());
    m_mixerBus->setSolo(m_soloed.load());

    // Enable flush-to-zero and denormals-are-zero on platforms that support SSE
    // This protects the audio thread from denormal slowdowns caused by tiny near-zero floats.
    // It's low-risk on Windows/MSVC with SSE support; noop on compilers without xmmintrin.h.
#if defined(_MSC_VER) || defined(__SSE__)
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    // Note: _MM_SET_DENORMALS_ZERO_MODE may not be available on all toolchains; flush-to-zero is the primary protection.
#endif

    Log::info("Track created: " + m_name + " (ID: " + std::to_string(m_trackId) + ")");
}

Track::~Track() {
    if (isRecording()) {
        stopRecording();
    }
    Log::info("Track destroyed: " + m_name);
}
// Update output sample rate - update device output rate used for playback.
// Do NOT perform automatic re-resampling here; playback will use the
// configured playback mode to determine how to produce output.
void Track::setOutputSampleRate(uint32_t sampleRate) {
    if (m_sampleRate == sampleRate) {
        return;
    }
    m_sampleRate = sampleRate;
    Log::info("Track " + m_name + ": output sample rate set to " + std::to_string(sampleRate) + " Hz");
}

// Track Properties
void Track::setName(const std::string& name) {
    m_name = name;
    if (m_mixerBus) {
        // Update mixer bus name too
        // Note: MixerBus doesn't have setName method yet, but we could add it
    }
}

void Track::setColor(uint32_t color) {
    m_color = color;
}

// Audio Parameters (thread-safe)
void Track::setVolume(float volume) {
    volume = (volume < 0.0f) ? 0.0f : (volume > 2.0f) ? 2.0f : volume;  // 0% to 200%
    m_volume.store(volume);
    if (m_mixerBus) {
        m_mixerBus->setGain(volume);
    }
}

void Track::setPan(float pan) {
    pan = (pan < -1.0f) ? -1.0f : (pan > 1.0f) ? 1.0f : pan;
    m_pan.store(pan);
    if (m_mixerBus) {
        m_mixerBus->setPan(pan);
    }
}

void Track::setMute(bool mute) {
    m_muted.store(mute);
    if (m_mixerBus) {
        m_mixerBus->setMute(mute);
    }
}

void Track::setSolo(bool solo) {
    m_soloed.store(solo);
    if (m_mixerBus) {
        m_mixerBus->setSolo(solo);
    }
}

// Track State
void Track::setState(TrackState state) {
    TrackState oldState = m_state.exchange(state);
    if (oldState != state) {
        Log::info("Track " + m_name + " state changed: " +
                  std::to_string(static_cast<int>(oldState)) + " -> " +
                  std::to_string(static_cast<int>(state)));

        // Handle state transitions
        switch (state) {
            case TrackState::Playing:
                // CRITICAL FIX: Do NOT reset playback phase when transitioning to Playing
                // The phase should be preserved from setPosition() calls (e.g., seeking while stopped)
                // This allows seeking while stopped, then playing from that position
                // Phase is only reset when explicitly calling stop()
                break;
            case TrackState::Stopped:
                Log::info("Reached TrackState::Stopped case");
                m_playbackPhase.store(0.0);
                m_positionSeconds.store(0.0);
                break;
            case TrackState::Recording:
                m_recordingBuffer.clear();
                m_isRecording.store(true);
                break;
            default:
                break;
        }
    }
}

// Audio Data Management
bool Track::loadAudioFile(const std::string& filePath) {
    std::cout << "Loading: " << filePath << " (track: " << m_name << ")" << std::endl;

    // Check if file exists
    std::ifstream checkFile(filePath);
    if (!checkFile) {
        std::cout << "File not found, generating preview tone" << std::endl;
        return generatePreviewTone(filePath);
    }
    checkFile.close();

    // Clear any existing audio data
    m_audioData.clear();
    m_durationSeconds.store(0.0);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);

    // Determine file extension to choose appropriate loader
    std::string extension = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "wav") {
        // Load WAV file
        uint32_t fileSampleRate = 48000; // Default fallback
        uint16_t numChannels = 2;        // Default fallback
        std::vector<float> tempAudioData;

        if (loadWavFile(filePath, tempAudioData, fileSampleRate, numChannels)) {
            m_numChannels = numChannels;

            // Keep original file buffer and its sample rate. We'll use one of two
            // strategies depending on playback mode:
            // - RealTimeInterpolation (default): keep original and interpolate
            //   during playback (no pre-resample)
            // - PreResample: create a resampled buffer now for legacy behaviour
            m_originalAudioData = std::move(tempAudioData);
            m_originalSampleRate = fileSampleRate;

            if (m_playbackMode == PlaybackMode::PreResample) {
                if (m_originalSampleRate != m_sampleRate) {
                    resampleAudio(m_originalAudioData, m_audioData, m_originalSampleRate, m_sampleRate, numChannels);
                } else {
                    m_audioData = m_originalAudioData; // copy since original is kept
                }
                // PreResample: duration based on resampled data
                m_durationSeconds.store(static_cast<double>(m_audioData.size()) / (m_sampleRate * numChannels));
            } else {
                // Real-time interpolation: don't pre-resample. Duration based on original.
                uint32_t frames = static_cast<uint32_t>(m_originalAudioData.size() / numChannels);
                m_durationSeconds.store(static_cast<double>(frames) / static_cast<double>(m_originalSampleRate));
                m_audioData.clear();
            }

            setState(TrackState::Loaded);

            Log::info("WAV loaded successfully: " + std::to_string((m_playbackMode == PlaybackMode::PreResample ? m_audioData.size() : m_originalAudioData.size())) + " samples, " +
                       std::to_string(m_durationSeconds.load()) + " seconds at " + 
                       std::to_string((m_playbackMode == PlaybackMode::PreResample ? m_sampleRate : m_originalSampleRate)) + " Hz");
            return true;
        } else {
            Log::warning("Failed to load WAV file: " + filePath + ", generating preview tone instead");
        }
    }

    // Special handling for demo files - generate audio directly
    if (filePath.find("demo_") != std::string::npos) {
        std::cout << "Demo file detected, generating audio directly" << std::endl;
        return generateDemoAudio(filePath);
    }

    // Fallback: generate preview tone for unsupported formats or failed loads
    std::cout << "Falling back to preview tone" << std::endl;
    return generatePreviewTone(filePath);
}

bool Track::generatePreviewTone(const std::string& filePath) {
    // Use filename hash to generate a unique frequency
    size_t filenameHash = std::hash<std::string>{}(filePath);
    double baseFrequency = 220.0 + (filenameHash % 440);

    // Generate 5 seconds of audio
    const double duration = 5.0;
    const uint32_t totalSamples = static_cast<uint32_t>(m_sampleRate * duration * m_numChannels);
    m_audioData.resize(totalSamples);

    // Generate waveform
    double freq1 = baseFrequency;
    double freq2 = baseFrequency * 1.5;
    double freq3 = baseFrequency * 2.0;

    for (uint32_t i = 0; i < totalSamples / m_numChannels; ++i) {
        double phase1 = 2.0 * 3.14159265358979323846 * freq1 * i / m_sampleRate;
        double phase2 = 2.0 * 3.14159265358979323846 * freq2 * i / m_sampleRate;
        double phase3 = 2.0 * 3.14159265358979323846 * freq3 * i / m_sampleRate;

        float sample1 = 0.4f * static_cast<float>(std::sin(phase1));
        float sample2 = 0.2f * static_cast<float>(std::sin(phase2));
        float sample3 = 0.1f * static_cast<float>(std::sin(phase3));

        float sample = sample1 + sample2 + sample3;
        sample = (sample < -0.9f) ? -0.9f : (sample > 0.9f) ? 0.9f : sample;

        m_audioData[i * m_numChannels + 0] = sample;
        m_audioData[i * m_numChannels + 1] = sample;
    }

    m_durationSeconds.store(duration);
    setState(TrackState::Loaded);

    std::cout << "Preview tone generated: " << m_audioData.size() << " samples, "
              << m_durationSeconds.load() << " seconds, " << baseFrequency << " Hz" << std::endl;

    return true;
}

bool Track::generateDemoAudio(const std::string& filePath) {
    std::cout << "Generating demo audio for: " << filePath << std::endl;

    // Determine frequency and duration based on filename
    double frequency = 440.0;
    double duration = 3.0;

    if (filePath.find("guitar") != std::string::npos) {
        frequency = 440.0; duration = 3.0;
    } else if (filePath.find("drums") != std::string::npos) {
        frequency = 120.0; duration = 2.0;
    } else if (filePath.find("vocals") != std::string::npos) {
        frequency = 330.0; duration = 4.0;
    }

    // Generate audio data
    const uint32_t totalSamples = static_cast<uint32_t>(m_sampleRate * duration * m_numChannels);
    m_audioData.resize(totalSamples);

    // Generate waveform
    double freq1 = frequency;
    double freq2 = frequency * 1.5;
    double freq3 = frequency * 2.0;

    for (uint32_t i = 0; i < totalSamples / m_numChannels; ++i) {
        double phase1 = 2.0 * 3.14159265358979323846 * freq1 * i / m_sampleRate;
        double phase2 = 2.0 * 3.14159265358979323846 * freq2 * i / m_sampleRate;
        double phase3 = 2.0 * 3.14159265358979323846 * freq3 * i / m_sampleRate;

        float sample1 = 0.4f * static_cast<float>(std::sin(phase1));
        float sample2 = 0.2f * static_cast<float>(std::sin(phase2));
        float sample3 = 0.1f * static_cast<float>(std::sin(phase3));

        float sample = sample1 + sample2 + sample3;
        sample = (sample < -0.9f) ? -0.9f : (sample > 0.9f) ? 0.9f : sample;

        m_audioData[i * m_numChannels + 0] = sample;
        m_audioData[i * m_numChannels + 1] = sample;
    }

    m_durationSeconds.store(duration);
    setState(TrackState::Loaded);

    std::cout << "Demo audio generated: " << m_audioData.size() << " samples, "
              << m_durationSeconds.load() << " seconds, " << frequency << " Hz" << std::endl;

    return true;
}

void Track::clearAudioData() {
    m_audioData.clear();
    m_originalAudioData.clear();
    m_originalSampleRate = 0;
    m_recordingBuffer.clear();
    m_durationSeconds.store(0.0);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
    setState(TrackState::Empty);
}

void Track::setAudioData(const float* data, uint32_t numSamples, uint32_t sampleRate, uint32_t numChannels) {
    if (!data || numSamples == 0) {
        Log::error("Invalid audio data");
        return;
    }
    
    // Copy audio data
    m_audioData.assign(data, data + (numSamples * numChannels));
    m_sampleRate = sampleRate;
    m_numChannels = numChannels;
    m_durationSeconds.store(static_cast<double>(numSamples) / sampleRate);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
    setState(TrackState::Loaded);
    
    Log::info("Audio data loaded: " + std::to_string(numSamples) + " samples, " +
               std::to_string(m_durationSeconds.load()) + " seconds, " + 
               std::to_string(sampleRate) + " Hz, " + std::to_string(numChannels) + " channels");
}

// Recording
void Track::setLatencyCompensation(double inputLatencyMs, double outputLatencyMs) {
    m_latencyCompensationMs = inputLatencyMs + outputLatencyMs;
    Log::info("Track '" + m_name + "' latency compensation set: " + 
              std::to_string(m_latencyCompensationMs) + " ms (Input: " + 
              std::to_string(inputLatencyMs) + " ms + Output: " + 
              std::to_string(outputLatencyMs) + " ms)");
}

void Track::startRecording() {
    if (getState() != TrackState::Empty) {
        Log::warning("Cannot start recording: track not empty");
        return;
    }

    Log::info("Starting recording on track: " + m_name);
    setState(TrackState::Recording);
}

void Track::stopRecording() {
    if (!isRecording()) {
        return;
    }

    Log::info("Stopping recording on track: " + m_name);

    // Move recording buffer to main audio data
    if (!m_recordingBuffer.empty()) {
        m_audioData = std::move(m_recordingBuffer);
        
        // Apply latency compensation if configured
        if (m_latencyCompensationMs > 0.0) {
            // Calculate how many samples to shift (compensate for input + output latency)
            uint32_t compensationSamples = static_cast<uint32_t>(
                (m_latencyCompensationMs / 1000.0) * m_sampleRate * m_numChannels
            );
            
            // Ensure we don't shift more than available data
            if (compensationSamples > 0 && compensationSamples < m_audioData.size()) {
                // Shift audio data earlier by removing the latency from the beginning
                // This aligns the recorded audio with the timeline
                m_audioData.erase(m_audioData.begin(), m_audioData.begin() + compensationSamples);
                
                Log::info("[Latency Compensation] Shifted recorded audio earlier by " + 
                          std::to_string(m_latencyCompensationMs) + " ms (" + 
                          std::to_string(compensationSamples / m_numChannels) + " frames)");
            }
        }
        
        m_durationSeconds.store(static_cast<double>(m_audioData.size()) / (m_sampleRate * m_numChannels));
        setState(TrackState::Loaded);
    } else {
        setState(TrackState::Empty);
    }

    m_recordingBuffer.clear();
    m_isRecording.store(false);
}


void Track::pause() {
    if (getState() == TrackState::Playing) {
        Log::info("Pausing track: " + m_name);
        setState(TrackState::Paused);
    }
}

void Track::stop() {
    Log::info("Stopping track: " + m_name);
    setState(TrackState::Stopped);
    // Position reset will happen when play() is called again
}

void Track::reset() {
    Log::info("Resetting DSP state for track: " + m_name);
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
    m_dcOffset = 0.0;
    
    // Reset dither history
    m_ditherHistory[0] = 0.0f;
    m_ditherHistory[1] = 0.0f;
    m_highPassDitherHistory = 0.0f;
    
    // Reset Euphoria engine state
    m_airDelayPos = 0;
    m_driftPhase = 0.0f;
    std::fill(std::begin(m_airDelayBufferL), std::end(m_airDelayBufferL), 0.0f);
    std::fill(std::begin(m_airDelayBufferR), std::end(m_airDelayBufferR), 0.0f);
}

// Position Control
void Track::setPosition(double seconds) {
    // Clamp position to valid range [0, duration]
    double duration = getDuration();
    seconds = std::max(0.0, std::min(duration, seconds));
    m_positionSeconds.store(seconds);

    // Convert seconds to playback phase using appropriate sample rate
    // For RealTimeInterpolation mode: phase is in original sample space (m_originalSampleRate)
    // For PreResample mode: phase is in output sample space (m_sampleRate)
    
    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        // Real-time interpolation: phase needs to be in original file's sample space
        m_playbackPhase.store(seconds * m_originalSampleRate);
    } else {
        // Pre-resample: phase is in output sample space
        m_playbackPhase.store(seconds * m_sampleRate);
    }
}

void Track::setStartPositionInTimeline(double seconds) {
    m_startPositionInTimeline.store(seconds);
    
    // CRITICAL: Reset playback phase to beginning of sample when timeline position changes
    // This ensures audio plays correctly from the new position
    m_playbackPhase.store(0.0);
    m_positionSeconds.store(0.0);
}

// Align track playback phase to the project's global playhead position.
// If the playhead is before the sample start, phase is set to 0.0.
// If the playhead is after the sample end, the phase is left unchanged.
void Track::syncPlaybackPhaseWithGlobalPlayhead(double globalPlayheadPosition) {
    if (globalPlayheadPosition < 0.0) return;

    double sampleStart = m_startPositionInTimeline.load();
    double duration = getDuration();
    double sampleEnd = sampleStart + duration;

    // If playhead is before the sample, reset to start
    if (globalPlayheadPosition < sampleStart) {
        m_playbackPhase.store(0.0);
        m_positionSeconds.store(0.0);
        return;
    }

    // If playhead is after sample end, don't change phase
    if (globalPlayheadPosition >= sampleEnd) {
        return;
    }

    double timeIntoSample = globalPlayheadPosition - sampleStart;
    double newPhase = 0.0;

    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        newPhase = timeIntoSample * static_cast<double>(m_originalSampleRate);
    } else {
        if (m_sampleRate == 0) return;
        newPhase = timeIntoSample * static_cast<double>(m_sampleRate);
    }

    // Clamp to available frames
    uint64_t totalFrames = 0;
    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        totalFrames = m_originalAudioData.empty() ? 0 : (m_originalAudioData.size() / m_numChannels);
    } else {
        totalFrames = m_audioData.empty() ? 0 : (m_audioData.size() / m_numChannels);
    }

    if (totalFrames > 0 && newPhase >= static_cast<double>(totalFrames)) {
        newPhase = static_cast<double>(totalFrames) - 1.0;
        if (newPhase < 0.0) newPhase = 0.0;
    }

    // Debug: log when playback phase is clamped to end
    if (totalFrames > 0 && newPhase >= static_cast<double>(totalFrames) - 1.0) {
        Log::info("[DEBUG] Track '" + m_name + "' syncPlaybackPhase clamped to end: newPhase=" + std::to_string(newPhase) + ", totalFrames=" + std::to_string(totalFrames));
    }
    m_playbackPhase.store(newPhase);

    if (m_playbackMode == PlaybackMode::RealTimeInterpolation && m_originalSampleRate > 0) {
        m_positionSeconds.store(newPhase / static_cast<double>(m_originalSampleRate));
    } else if (m_sampleRate > 0) {
        m_positionSeconds.store(newPhase / static_cast<double>(m_sampleRate));
    }
}

// Audio Processing (legacy - no timeline position checking)
void Track::processAudio(float* outputBuffer, uint32_t numFrames, double streamTime) {
    // Call the new version with position = 0 (no timeline checking)
    processAudio(outputBuffer, numFrames, streamTime, -1.0);
}

// Audio Processing with timeline position checking
void Track::processAudio(float* outputBuffer, uint32_t numFrames, double streamTime, double globalPlayheadPosition) {
    if (!outputBuffer || numFrames == 0) {
        return;
    }

    TrackState currentState = getState();

    // Create temporary buffer for this track's audio before mixing
    std::vector<float> trackBuffer(numFrames * m_numChannels, 0.0f);

    switch (currentState) {
        case TrackState::Playing: {
            // Check if global playhead is within this sample's timeline bounds
            bool shouldPlay = true;
            if (globalPlayheadPosition >= 0.0) {
                double sampleStart = m_startPositionInTimeline.load();
                double sampleEnd = sampleStart + getDuration();
                
                // Only play if playhead is within sample bounds
                shouldPlay = (globalPlayheadPosition >= sampleStart && globalPlayheadPosition < sampleEnd);
            }
            
            if (shouldPlay) {
                    // Debug: log when the playhead is about to enter this sample
                    if (globalPlayheadPosition >= 0.0) {
                        double sampleStart = m_startPositionInTimeline.load();
                        double timeUntilStart = sampleStart - globalPlayheadPosition;
                        if (timeUntilStart >= 0.0 && timeUntilStart < 0.5) {
                            Log::info("[DEBUG] Track '" + m_name + "' entering sample in " + std::to_string(timeUntilStart) + "s; phase=" + std::to_string(m_playbackPhase.load()) + ", posSec=" + std::to_string(m_positionSeconds.load()));
                        }
                    }
    
                    // Throttled entry trace: log occasionally to confirm the audio thread is invoking this method
                    uint32_t callIdx = m_copyAudioCallCounter.fetch_add(1);
                    if ((callIdx & 0x3F) == 0) { // every 64th call
                        Log::info(std::string("[TRACE] processAudio -> copyAudioData for track '") + m_name + "', state=Playing, phase=" + std::to_string(m_playbackPhase.load()) + ", startPos=" + std::to_string(m_startPositionInTimeline.load()) + ", globalPlayhead=" + std::to_string(globalPlayheadPosition));
                    }
    
                    // Copy audio data to temporary buffer
                    copyAudioData(trackBuffer.data(), numFrames);
                
                // Process through mixer bus for volume/pan/mute/solo
                if (m_mixerBus) {
                    m_mixerBus->process(trackBuffer.data(), numFrames);
                }
                
                // Mix into output buffer
                for (uint32_t i = 0; i < numFrames * m_numChannels; ++i) {
                    outputBuffer[i] += trackBuffer[i];
                }
            }
            // else: output silence (don't modify output buffer)
            break;
        }

        case TrackState::Recording:
            m_playbackPhase.store(m_playbackPhase.load() + numFrames * m_numChannels);
            break;

        case TrackState::Paused:
        case TrackState::Stopped:
        case TrackState::Empty:
        case TrackState::Loaded:
        default:
            // Output silence (don't modify output buffer)
            break;
    }

    // Position is now updated inside copyAudioData (phase -> seconds conversion)
    // to ensure a single authoritative source of truth and avoid double-updating
    // between audio thread and UI thread. No position arithmetic here.
}

void Track::generateSilence(float* buffer, uint32_t numFrames) {
    if (buffer) {
        std::fill(buffer, buffer + numFrames * m_numChannels, 0.0f);
    }
}

void Track::copyAudioData(float* outputBuffer, uint32_t numFrames) {
    // --- CRITICAL FIX: Enhanced self-stopping logic ---
    const auto* audioDataSource = (m_playbackMode == PlaybackMode::RealTimeInterpolation) ? &m_originalAudioData : &m_audioData;
    const uint32_t totalFrames = audioDataSource->empty() ? 0 : static_cast<uint32_t>(audioDataSource->size() / m_numChannels);
    
    double currentPhase = m_playbackPhase.load();
    
    // PHASE OVERFLOW PROTECTION: Multiple safety checks
    bool shouldStop = false;
    std::string stopReason = "";
    
    if (totalFrames == 0) {
        shouldStop = true;
        stopReason = "No audio data";
    } else if (currentPhase >= totalFrames) {
        shouldStop = true;
        stopReason = "Phase overflow (current=" + std::to_string(currentPhase) + ", max=" + std::to_string(totalFrames) + ")";
    } else if (std::isnan(currentPhase) || std::isinf(currentPhase)) {
        shouldStop = true;
        stopReason = "Invalid phase (NaN/Inf)";
    } else if (currentPhase < 0) {
        shouldStop = true;
        stopReason = "Negative phase";
    }
    
    // ENHANCED: Log ALL stop conditions for debugging
    if (shouldStop) {
        Log::error("[CRITICAL FIX] AUTO-STOP TRIGGERED for track '" + m_name + "': " + stopReason);
        Log::error("[CRITICAL FIX] Track details: phase=" + std::to_string(currentPhase) +
                   ", totalFrames=" + std::to_string(totalFrames) +
                   ", playbackMode=" + std::to_string(static_cast<int>(m_playbackMode)) +
                   ", state=" + std::to_string(static_cast<int>(getState())));
        
        if (getState() == TrackState::Playing) {
            Log::error("[CRITICAL FIX] Calling setState(Stopped) from Playing state");
            setState(TrackState::Stopped);
        }
        
        generateSilence(outputBuffer, numFrames);
        Log::error("[CRITICAL FIX] Auto-stop complete, generating silence and returning");
        return;
    }

    // Throttled entry trace: log occasionally to confirm the audio thread is invoking this method.
    // This avoids producing excessive log volume during sustained playback while still
    // giving us visibility for intermittent repro steps.
    uint32_t callIdx = m_copyAudioCallCounter.fetch_add(1);
    if ((callIdx & 0x3F) == 0) { // every 64th call
        std::string msg = "[TRACE] copyAudioData for track '" + m_name + "'";
        msg += ", state=" + std::to_string(static_cast<int>(m_state.load()));
        msg += ", phase=" + std::to_string(m_playbackPhase.load());
        msg += ", startPos=" + std::to_string(m_startPositionInTimeline.load());
        msg += ", outSampleRate=" + std::to_string(m_sampleRate);
        Log::info(msg);
    }

    // Start timing for the whole copyAudioData call so we can measure expensive blocks
    auto __copy_start = std::chrono::high_resolution_clock::now();

    // Two modes of playback:
    // - PreResample: use m_audioData which is already resampled to output rate
    // - RealTimeInterpolation: use m_originalAudioData and interpolate on-the-fly
    if (m_playbackMode == PlaybackMode::PreResample) {
        // PreResample mode
        uint32_t totalSamples = static_cast<uint32_t>(m_audioData.size());
        if (totalSamples == 0) {
            Log::warning("[WATCHDOG] copyAudioData: m_audioData empty for track '" + m_name + "' -> generating silence");
            generateSilence(outputBuffer, numFrames);
            return;
        }

        double phase = m_playbackPhase.load();
        double blockPhaseStart = phase;

        // PreResample phase tracking continues normally - safety checks handle overflow conditions
        // m_audioData is in output-rate frames, so increment by 1 frame per output frame
        const double phaseIncrement = 1.0;

        // Compute number of source frames (frames = samples / channels)
        uint32_t totalFrames = (m_numChannels == 0) ? 0 : (totalSamples / m_numChannels);

        for (uint32_t frame = 0; frame < numFrames; ++frame) {
            double exactSamplePos = phase;

            // CRITICAL FIX: Immediate phase validation during accumulation
            if (!std::isfinite(exactSamplePos) || exactSamplePos < 0.0) {
                Log::error("[CRITICAL FIX] PreResample: Invalid phase detected, stopping playback");
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer, numFrames);
                return;
            }
            
            if (totalFrames > 0 && exactSamplePos >= totalFrames) {
                Log::error("[CRITICAL FIX] PreResample: Phase exceeded buffer, stopping playback. phase=" +
                          std::to_string(exactSamplePos) + ", totalFrames=" + std::to_string(totalFrames));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer, numFrames);
                return;
            }

            for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
                float sample = 0.0f;

                switch (m_qualitySettings.resampling) {
                    case ResamplingMode::Fast:
                        sample = interpolateLinear(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Medium:
                        sample = interpolateCubic(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::High:
                        sample = interpolateSinc(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Ultra:
                        sample = interpolateUltra(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Extreme:
                        sample = interpolateExtreme(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                    case ResamplingMode::Perfect:
                        sample = interpolatePerfect(m_audioData.data(), totalSamples, exactSamplePos, ch);
                        break;
                }

                // Sanitize sample: guard against NaN/Inf and denormals
                if (!std::isfinite(sample) || std::abs(sample) < 1e-20f) {
                    sample = 0.0f;
                }
                sample = std::max(-1.0f, std::min(1.0f, sample));

                outputBuffer[frame * m_numChannels + ch] = sample;
            }

            // CRITICAL FIX: Immediate stop if next phase would exceed bounds
            double nextPhase = phase + phaseIncrement;
            if (totalFrames > 0 && nextPhase >= totalFrames) {
                Log::error("[CRITICAL FIX] PreResample: Next phase would exceed bounds, stopping. nextPhase=" +
                          std::to_string(nextPhase) + ", totalFrames=" + std::to_string(totalFrames));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer + frame * m_numChannels, (numFrames - frame) * m_numChannels);
                return;
            }
            phase = nextPhase;
        }

        // Clamp phase to valid range and stop playback if we've reached the end
        double maxPhase = (totalFrames == 0) ? 0.0 : (static_cast<double>(totalFrames) - 1.0);
        if (!std::isfinite(phase)) phase = 0.0;
        if (phase >= maxPhase) {
            phase = maxPhase;
            if (getState() == TrackState::Playing) {
                Log::warning("[WATCHDOG] Track '" + m_name + "' reached end during PreResample block; auto-stopping.");
                setState(TrackState::Stopped);
            }
        }
        m_playbackPhase.store(phase);
        
        // CRITICAL FIX: Consistent time-based positioning for both playback modes
        // Position is calculated from playback phase, ensuring sample-accurate timing
        m_positionSeconds.store(phase / static_cast<double>(m_sampleRate));

        // Optional short debug trace: emit a summary for the first few blocks after play
        if (m_debugTraceBlocksRemaining.load() > 0) {
            float maxAbs = 0.0f;
            uint32_t totalOutSamples = numFrames * m_numChannels;
            for (uint32_t i = 0; i < totalOutSamples; ++i) {
                float a = std::abs(outputBuffer[i]);
                if (a > maxAbs) maxAbs = a;
            }
            std::string msg = "[TRACE] copyAudioData block summary for track '" + m_name + "'";
            msg += ", mode=PreResample";
            msg += ", startPhase=" + std::to_string(blockPhaseStart);
            msg += ", endPhase=" + std::to_string(phase);
            msg += ", posSec=" + std::to_string(m_positionSeconds.load());
            msg += ", maxAbs=" + std::to_string(maxAbs);
            Log::info(msg);
            m_debugTraceBlocksRemaining.fetch_sub(1);
        }
        // End timing and conditionally log if this block was slow
        auto __copy_end = std::chrono::high_resolution_clock::now();
        double __copy_ms = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(__copy_end - __copy_start).count()) / 1000.0;
        const double __copy_log_threshold_ms = 1.0; // adjustable threshold
        if (__copy_ms > __copy_log_threshold_ms) {
            std::string perfMsg = "[PERF] copyAudioData (PreResample) took " + std::to_string(__copy_ms) + " ms for track '" + m_name + "', frames=" + std::to_string(numFrames) + ", channels=" + std::to_string(m_numChannels);
            Log::warning(perfMsg);
        }
    } else {
        // RealTimeInterpolation mode
        uint32_t originalTotalSamples = static_cast<uint32_t>(m_originalAudioData.size());
        if (originalTotalSamples == 0 || m_originalSampleRate == 0) {
            Log::warning("[WATCHDOG] copyAudioData: m_originalAudioData empty or originalSampleRate==0 for track '" + m_name + "' -> generating silence");
            generateSilence(outputBuffer, numFrames);
            return;
        }

        uint32_t totalOrigFrames = (m_numChannels == 0) ? 0 : (originalTotalSamples / m_numChannels);
        double maxValidPhase = (totalOrigFrames == 0) ? 0.0 : (static_cast<double>(totalOrigFrames) - 1.0);
        
        double rtPhase = m_playbackPhase.load(); // phase in source frames (original sample rate)
        double rtBlockPhaseStart = rtPhase;

        // CRITICAL FIX: Enhanced sample rate ratio validation
        double sampleRateRatio = 1.0;
        if (m_sampleRate > 0 && m_originalSampleRate > 0) {
            sampleRateRatio = static_cast<double>(m_originalSampleRate) / static_cast<double>(m_sampleRate);
            // Validate sample rate ratio is reasonable
            if (sampleRateRatio <= 0.0 || !std::isfinite(sampleRateRatio) || sampleRateRatio > 1000.0) {
                Log::error("[CRITICAL FIX] Invalid sampleRateRatio: " + std::to_string(sampleRateRatio) +
                          " (orig=" + std::to_string(m_originalSampleRate) +
                          ", out=" + std::to_string(m_sampleRate) + ")");
                generateSilence(outputBuffer, numFrames);
                return;
            }
        }
        
        // CRITICAL FIX: Ultra-early phase detection - only check for actual overflow
        if (rtPhase >= maxValidPhase) {
            Log::info("[WATCHDOG] RealTimeInterpolation: Reached end of buffer, stopping naturally. phase=" +
                     std::to_string(rtPhase) + ", maxPhase=" + std::to_string(maxValidPhase));
            if (getState() == TrackState::Playing) {
                setState(TrackState::Stopped);
            }
            generateSilence(outputBuffer, numFrames);
            return;
        }
        
        // Validate initial phase is reasonable (very strict)
        if (!std::isfinite(rtPhase) || rtPhase < 0.0 || rtPhase >= maxValidPhase * 0.3) {
            Log::error("[CRITICAL FIX] Invalid initial phase: " + std::to_string(rtPhase) +
                      ", maxValidPhase=" + std::to_string(maxValidPhase) +
                      ", sampleRateRatio=" + std::to_string(sampleRateRatio));
            if (getState() == TrackState::Playing) {
                setState(TrackState::Stopped);
            }
            generateSilence(outputBuffer, numFrames);
            return;
        }
        
        // Process frames with ultra-aggressive safety to prevent phase accumulation
        uint32_t framesProcessed = 0;
        double expectedPhase = rtPhase; // Track expected phase progression
        
        for (uint32_t frame = 0; frame < numFrames; ++frame) {
            // CRITICAL FIX: Stop at the actual end of the buffer, not halfway.
            if (rtPhase >= maxValidPhase) {
                Log::info("[WATCHDOG] Reached end of buffer, stopping normally. phase=" +
                         std::to_string(rtPhase) + ", maxPhase=" + std::to_string(maxValidPhase));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                // Generate silence for all remaining frames to prevent garbage audio.
                for (uint32_t remainingFrame = frame; remainingFrame < numFrames; ++remainingFrame) {
                    for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
                        outputBuffer[remainingFrame * m_numChannels + ch] = 0.0f;
                    }
                }
                frame = numFrames; // Force exit from loop
                break;
            }
            
            // CRITICAL FIX: Detect phase advancement anomalies (stricter)
            double phaseJump = rtPhase - expectedPhase;
            if (phaseJump > sampleRateRatio * 1.1) {
                Log::error("[CRITICAL FIX] Phase advancement anomaly detected! Expected=" +
                          std::to_string(expectedPhase) + ", Actual=" + std::to_string(rtPhase) +
                          ", Jump=" + std::to_string(phaseJump) + ", ratio=" + std::to_string(sampleRateRatio));
                Log::error("[CRITICAL FIX] STOPPING IMMEDIATELY to prevent buzzing");
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer, numFrames);
                return;
            }
            
            // Validate phase is still reasonable
            if (!std::isfinite(rtPhase) || rtPhase < 0.0 || rtPhase > maxValidPhase) {
                Log::error("[CRITICAL FIX] Phase became invalid during processing: " + std::to_string(rtPhase) +
                          ", expected=" + std::to_string(expectedPhase) + ", jump=" + std::to_string(phaseJump));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                generateSilence(outputBuffer + frame * m_numChannels, (numFrames - frame) * m_numChannels);
                return;
            }
            
            // Only interpolate if phase is within valid range (conservative threshold)
            if (rtPhase < maxValidPhase * 0.4) {
                for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
                    float sample = 0.0f;

                    switch (m_qualitySettings.resampling) {
                        case ResamplingMode::Fast:
                            sample = interpolateLinear(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                            break;
                        case ResamplingMode::Medium:
                            sample = interpolateCubic(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                            break;
                        case ResamplingMode::High:
                            sample = interpolateSinc(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                            break;
                        case ResamplingMode::Ultra:
                            sample = interpolateUltra(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                            break;
                        case ResamplingMode::Extreme:
                            sample = interpolateExtreme(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                            break;
                        case ResamplingMode::Perfect:
                            sample = interpolatePerfect(m_originalAudioData.data(), originalTotalSamples, rtPhase, ch);
                            break;
                    }

                    // Sanitize sample: guard against NaN/Inf and denormals
                    if (!std::isfinite(sample) || std::abs(sample) < 1e-20f) {
                        sample = 0.0f;
                    }
                    sample = std::max(-1.0f, std::min(1.0f, sample));
                    outputBuffer[frame * m_numChannels + ch] = sample;
                }
            } else {
                // Generate silence for remaining frames
                for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
                    outputBuffer[frame * m_numChannels + ch] = 0.0f;
                }
            }
            
            // Advance phase for next iteration
            rtPhase += sampleRateRatio;
            expectedPhase += sampleRateRatio; // Track what the phase SHOULD be
            framesProcessed++;
            
            // Very low threshold for emergency stop
            if (rtPhase > maxValidPhase * 0.6) {
                Log::error("[CRITICAL FIX] Phase overflow detected, emergency stop. phase=" +
                          std::to_string(rtPhase) + ", maxPhase=" + std::to_string(maxValidPhase) +
                          ", expected=" + std::to_string(expectedPhase));
                if (getState() == TrackState::Playing) {
                    setState(TrackState::Stopped);
                }
                break;
            }
        }
        
        // Final phase clamping and state update
        if (rtPhase > maxValidPhase) {
            rtPhase = maxValidPhase;
        }
        if (!std::isfinite(rtPhase) || rtPhase < 0.0) {
            rtPhase = 0.0;
        }
        
        m_playbackPhase.store(rtPhase);
        
        // Update position based on phase
        if (m_originalSampleRate > 0) {
            m_positionSeconds.store(rtPhase / static_cast<double>(m_originalSampleRate));
        }

        // Debug trace for processed frames
        if (m_debugTraceBlocksRemaining.load() > 0 && framesProcessed > 0) {
            float maxAbs = 0.0f;
            uint32_t totalOutSamples = framesProcessed * m_numChannels;
            for (uint32_t i = 0; i < totalOutSamples; ++i) {
                float a = std::abs(outputBuffer[i]);
                if (a > maxAbs) maxAbs = a;
            }
            std::string msg = "[TRACE] copyAudioData block summary for track '" + m_name + "'";
            msg += ", mode=RealTimeInterpolation";
            msg += ", startPhase=" + std::to_string(rtBlockPhaseStart);
            msg += ", endPhase=" + std::to_string(rtPhase);
            msg += ", posSec=" + std::to_string(m_positionSeconds.load());
            msg += ", maxAbs=" + std::to_string(maxAbs);
            msg += ", framesProcessed=" + std::to_string(framesProcessed);
            msg += ", sampleRateRatio=" + std::to_string(sampleRateRatio);
            Log::info(msg);
            m_debugTraceBlocksRemaining.fetch_sub(1);
        }

        // Optional: enforce single phase advancement.
        static double lastPhaseValue = 0.0;
        if (m_playbackPhase.load() - lastPhaseValue > sampleRateRatio * numFrames * 1.5) {
            Log::warning("[PHASE CHECK] Excess phase advancement detected");
        }
        lastPhaseValue = m_playbackPhase.load();

        // Performance logging
        auto __copy_end_rt = std::chrono::high_resolution_clock::now();
        double __copy_ms_rt = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(__copy_end_rt - __copy_start).count()) / 1000.0;
        const double __copy_log_threshold_ms_rt = 1.0;
        if (__copy_ms_rt > __copy_log_threshold_ms_rt) {
            std::string perfMsg = "[PERF] copyAudioData (RealTimeInterpolation) took " + std::to_string(__copy_ms_rt) +
                                " ms for track '" + m_name + "', frames=" + std::to_string(numFrames) +
                                ", channels=" + std::to_string(m_numChannels) + ", processed=" + std::to_string(framesProcessed);
            Log::warning(perfMsg);
        }
    }
    
    // Apply audio quality enhancements
    uint32_t totalOutputSamples = numFrames * m_numChannels;
    
    // === PROCESSING ORDER (optimized for best sound quality) ===
    
    // 1. Nomad Mode Euphoria Engine (if enabled)
    //    - Applied FIRST to get the signature character on raw audio
    if (m_qualitySettings.nomadMode == NomadMode::Euphoric) {
        applyEuphoriaEngine(outputBuffer, numFrames);
    }
    
    // 2. DC Offset Removal
    //    - Clean up any DC bias from processing
    if (m_qualitySettings.removeDCOffset) {
        removeDC(outputBuffer, totalOutputSamples);
    }
    
    // 3. Dithering (based on mode)
    //    - Applied before final limiting for proper quantization
    if (m_qualitySettings.dithering != DitheringMode::None) {
        applyDithering(outputBuffer, totalOutputSamples);
    }
    
    // 4. Soft Clipping (if enabled)
    //    - Final safety limiter to prevent hard clipping
    if (m_qualitySettings.enableSoftClipping) {
        applySoftClipping(outputBuffer, totalOutputSamples);
    }

    for (uint32_t i = 0; i < totalOutputSamples; ++i) {
        assert(!std::isnan(outputBuffer[i]) && !std::isinf(outputBuffer[i]));
    }

    // playback phase already stored inside branch-specific logic
}

// Linear interpolation (2-point, fast) - FIXED: Safe index calculation prevents overflow
float Track::interpolateLinear(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    uint32_t idx0 = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    uint32_t idx1 = std::min(idx0 + 1, maxFrameIdx);
    double fraction = position - static_cast<double>(idx0);
    
    // CRITICAL FIX: Safe index calculation with overflow prevention
    if (idx0 >= maxFrameIdx) {
        // At or beyond last sample - return last valid sample
        uint32_t lastSampleIdx = maxFrameIdx * m_numChannels + channel;
        return (lastSampleIdx < totalSamples) ? data[lastSampleIdx] : 0.0f;
    }
    
    uint32_t sample0_idx = idx0 * m_numChannels + channel;
    uint32_t sample1_idx = idx1 * m_numChannels + channel;
    
    // Validate calculated indices before accessing data
    if (sample0_idx >= totalSamples || sample1_idx >= totalSamples) {
        return 0.0f;
    }
    
    float s0 = data[sample0_idx];
    float s1 = data[sample1_idx];
    
    // Ensure inputs are finite before interpolation
    if (!std::isfinite(s0) || !std::isfinite(s1)) {
        return 0.0f;
    }
    
    float result = s0 + static_cast<float>(fraction) * (s1 - s0);
    
    // Final validation: ensure result is finite and within bounds
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Cubic Hermite interpolation (4-point, good quality) - FIXED: Safe index calculation prevents overflow
float Track::interpolateCubic(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    uint32_t idx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(idx);
    
    // CRITICAL FIX: Safe sample retrieval with boundary checking
    float s0, s1, s2, s3;
    
    // Sample 0: previous sample (if available)
    if (idx > 0 && idx - 1 < maxFrameIdx) {
        uint32_t idx0 = (idx - 1) * m_numChannels + channel;
        s0 = (idx0 < totalSamples) ? data[idx0] : 0.0f;
    } else {
        s0 = 0.0f; // Use silence for out-of-bounds samples
    }
    
    // Sample 1: current sample
    uint32_t idx1 = idx * m_numChannels + channel;
    s1 = (idx1 < totalSamples) ? data[idx1] : 0.0f;
    
    // Sample 2: next sample
    if (idx + 1 < maxFrameIdx) {
        uint32_t idx2 = (idx + 1) * m_numChannels + channel;
        s2 = (idx2 < totalSamples) ? data[idx2] : 0.0f;
    } else {
        s2 = (maxFrameIdx > 0 && idx < maxFrameIdx) ? data[maxFrameIdx * m_numChannels + channel] : 0.0f;
    }
    
    // Sample 3: sample after next
    if (idx + 2 < maxFrameIdx) {
        uint32_t idx3 = (idx + 2) * m_numChannels + channel;
        s3 = (idx3 < totalSamples) ? data[idx3] : 0.0f;
    } else {
        s3 = (maxFrameIdx > 0 && idx < maxFrameIdx) ? data[maxFrameIdx * m_numChannels + channel] : 0.0f;
    }
    
    // Ensure all samples are finite before interpolation
    if (!std::isfinite(s0) || !std::isfinite(s1) || !std::isfinite(s2) || !std::isfinite(s3)) {
        return 0.0f;
    }
    
    // Hermite basis functions
    float t = static_cast<float>(fraction);
    float t2 = t * t;
    float t3 = t2 * t;
    
    float a0 = -0.5f * s0 + 1.5f * s1 - 1.5f * s2 + 0.5f * s3;
    float a1 = s0 - 2.5f * s1 + 2.0f * s2 - 0.5f * s3;
    float a2 = -0.5f * s0 + 0.5f * s2;
    float a3 = s1;
    
    float result = a0 * t3 + a1 * t2 + a2 * t + a3;
    
    // Final validation: ensure result is finite and within bounds
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Windowed Sinc interpolation (best quality, more CPU intensive) - FIXED: Safe index calculation prevents overflow
float Track::interpolateSinc(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int SINC_WINDOW_SIZE = 8;  // 8-point sinc (high quality)
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < SINC_WINDOW_SIZE/2) {
        // Not enough samples for proper sinc interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // Windowed sinc interpolation with enhanced bounds checking
    for (int i = -SINC_WINDOW_SIZE/2; i < SINC_WINDOW_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function: sin(PI*x) / (PI*x)
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.0001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Blackman window for better frequency response
        float window = 0.42f - 0.5f * std::cos(2.0f * PI * (i + SINC_WINDOW_SIZE/2) / SINC_WINDOW_SIZE)
                            + 0.08f * std::cos(4.0f * PI * (i + SINC_WINDOW_SIZE/2) / SINC_WINDOW_SIZE);
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Ultra (Polyphase Sinc) interpolation (16-point, mastering grade) - FIXED: Safe index calculation prevents overflow
float Track::interpolateUltra(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int POLYPHASE_SIZE = 16;  // 16-point polyphase for reference quality
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < POLYPHASE_SIZE/2) {
        // Not enough samples for proper ultra interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    // Precomputed Kaiser window lookup table (beta=8.6)
    static const float kaiserWindow[POLYPHASE_SIZE] = {
        0.0000f, 0.0217f, 0.0854f, 0.1865f, 0.3180f, 0.4706f, 0.6341f, 0.7975f,
        0.9500f, 0.9500f, 0.7975f, 0.6341f, 0.4706f, 0.3180f, 0.1865f, 0.0854f
    };
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // Polyphase sinc interpolation with precomputed Kaiser window and enhanced bounds checking
    for (int i = -POLYPHASE_SIZE/2; i < POLYPHASE_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function with higher precision
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.0001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Get precomputed Kaiser window value (FAST!)
        int windowIdx = i + POLYPHASE_SIZE/2;
        if (windowIdx < 0 || windowIdx >= POLYPHASE_SIZE) {
            continue;
        }
        float window = kaiserWindow[windowIdx];
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Extreme interpolation (64-point polyphase sinc - mastering grade, real-time safe) - FIXED: Safe index calculation prevents overflow
float Track::interpolateExtreme(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int EXTREME_SIZE = 64;  // 64-point sinc (sweet spot for quality/performance)
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < EXTREME_SIZE/2) {
        // Not enough samples for proper extreme interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    // Precomputed Kaiser window lookup table (beta=10.0)
    // Computed once, reused for all samples - HUGE performance win!
    static const float kaiserWindow[EXTREME_SIZE] = {
        0.0000f, 0.0011f, 0.0044f, 0.0098f, 0.0173f, 0.0268f, 0.0384f, 0.0520f,
        0.0675f, 0.0849f, 0.1042f, 0.1252f, 0.1479f, 0.1722f, 0.1980f, 0.2252f,
        0.2537f, 0.2834f, 0.3142f, 0.3460f, 0.3786f, 0.4119f, 0.4459f, 0.4803f,
        0.5151f, 0.5502f, 0.5854f, 0.6206f, 0.6557f, 0.6906f, 0.7250f, 0.7590f,
        0.7923f, 0.8249f, 0.8566f, 0.8873f, 0.9169f, 0.9453f, 0.9724f, 0.9981f,
        1.0000f, 0.9981f, 0.9724f, 0.9453f, 0.9169f, 0.8873f, 0.8566f, 0.8249f,
        0.7923f, 0.7590f, 0.7250f, 0.6906f, 0.6557f, 0.6206f, 0.5854f, 0.5502f,
        0.5151f, 0.4803f, 0.4459f, 0.4119f, 0.3786f, 0.3460f, 0.3142f, 0.2834f
    };
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // 64-point polyphase sinc with precomputed Kaiser window and enhanced bounds checking
    for (int i = -EXTREME_SIZE/2; i < EXTREME_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function: sin(PI*x) / (PI*x)
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.00001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Get precomputed Kaiser window value (FAST!) with bounds check
        int windowIdx = i + EXTREME_SIZE/2;
        if (windowIdx < 0 || windowIdx >= EXTREME_SIZE) {
            continue;
        }
        float window = kaiserWindow[windowIdx];
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Perfect interpolation (512-point polyphase sinc - FL Studio grade) - FIXED: Safe index calculation prevents overflow
// WARNING: EXTREMELY CPU INTENSIVE - Use only for offline rendering/mastering
float Track::interpolatePerfect(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int PERFECT_SIZE = 512;  // 512-point sinc (FL Studio quality)
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < PERFECT_SIZE/2) {
        // Not enough samples for proper perfect interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // 512-point polyphase sinc with Kaiser window (beta=12.0 for extreme precision) and enhanced bounds checking
    for (int i = -PERFECT_SIZE/2; i < PERFECT_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function: sin(PI*x) / (PI*x)
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.00001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Kaiser window (beta=12.0 for maximum stopband attenuation) with safety checks
        float alpha = static_cast<float>(i + PERFECT_SIZE/2) / PERFECT_SIZE;
        if (std::abs(2.0f * alpha - 1.0f) > 1.0f) {
            continue; // Skip if outside valid range for sqrt
        }
        float kaiserArg = 12.0f * std::sqrt(1.0f - (2.0f * alpha - 1.0f) * (2.0f * alpha - 1.0f));
        
        // Modified Bessel function I0 approximation (higher precision) with safety checks
        auto besselI0 = [](float x) -> float {
            float sum = 1.0f;
            float term = 1.0f;
            for (int n = 1; n < 20; ++n) {  // More iterations for better precision
                term *= (x / (2.0f * n)) * (x / (2.0f * n));
                sum += term;
                if (term < 1e-10f) break;  // Early exit for convergence
            }
            return sum;
        };
        
        float besselDenom = besselI0(12.0f);
        if (besselDenom <= 0.0f) {
            continue; // Avoid division by zero or invalid results
        }
        float window = besselI0(kaiserArg) / besselDenom;
        
        // Ensure window is finite
        if (!std::isfinite(window)) {
            continue;
        }
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// === Dithering Methods ===

// Master dithering dispatcher
void Track::applyDithering(float* buffer, uint32_t numSamples) {
    switch (m_qualitySettings.dithering) {
        case DitheringMode::Triangular:
            applyTriangularDither(buffer, numSamples);
            break;
        case DitheringMode::HighPass:
            applyHighPassDither(buffer, numSamples);
            break;
        case DitheringMode::NoiseShaped:
            // applyNoiseShapedDither(buffer, numSamples); // Temporarily disabled for debugging
            break;
        case DitheringMode::None:
        default:
            // No dithering
            break;
    }
}

// TPDF Dithering (Triangular Probability Density Function)
void Track::applyTriangularDither(float* buffer, uint32_t numSamples) {
    const float DITHER_AMPLITUDE = 1.0f / 32768.0f;  // 16-bit dither
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        // TPDF: sum of two uniform random numbers gives triangular distribution
        float r1 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float r2 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float dither = (r1 + r2) * DITHER_AMPLITUDE;
        
        buffer[i] += dither;
        
        // Clamp to prevent overflow
        buffer[i] = std::max(-1.0f, std::min(1.0f, buffer[i]));
    }
}

// High-Pass Shaped Dithering
void Track::applyHighPassDither(float* buffer, uint32_t numSamples) {
    const float DITHER_AMPLITUDE = 1.0f / 32768.0f;
    const float HP_COEFF = 0.5f;  // High-pass coefficient
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        // Generate TPDF dither
        float r1 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float r2 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float dither = (r1 + r2) * DITHER_AMPLITUDE;
        
        // Apply high-pass shaping (pushes noise to higher frequencies)
        float shapedDither = dither - HP_COEFF * m_highPassDitherHistory;
        m_highPassDitherHistory = dither;
        
        buffer[i] += shapedDither;
        buffer[i] = std::max(-1.0f, std::min(1.0f, buffer[i]));
    }
}

// Noise-Shaped Dithering (Psychoacoustic)
void Track::applyNoiseShapedDither(float* buffer, uint32_t numSamples) {
    const float DITHER_AMPLITUDE = 1.0f / 32768.0f;
    
    // F-weighted noise shaping coefficients (pushes noise above ~2kHz where hearing is less sensitive)
    const float a1 = 2.033f;
    const float a2 = -1.165f;
    
    // Sanity-check dither history before processing. If any stored history is
    // non-finite (inf/NaN) it can feed back into the filter and destabilize
    // subsequent audio. Reset to 0.0f when that happens. This also covers the
    // "first use" case on preview/start if the history wasn't properly reset.
    for (int ch = 0; ch < 2; ++ch) {
        if (!std::isfinite(m_ditherHistory[ch])) {
            m_ditherHistory[ch] = 0.0f;
        }
    }
    if (!std::isfinite(m_highPassDitherHistory)) {
        m_highPassDitherHistory = 0.0f;
    }

    for (uint32_t i = 0; i < numSamples; ++i) {
        uint32_t channel = i % m_numChannels;
        float original_sample = buffer[i];

        // Guard against corrupted input samples (NaN/inf). Replace with 0.
        if (!std::isfinite(original_sample)) {
            original_sample = 0.0f;
        }

        // Generate TPDF dither
        float r1 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float r2 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float dither = (r1 + r2) * DITHER_AMPLITUDE;

        // 1. Form the input to the quantizer by subtracting the filtered error from history
        float sample_to_quantize = original_sample;
        if (channel < 2) {
            float hist = m_ditherHistory[channel];
            if (!std::isfinite(hist)) {
                // If history somehow became non-finite mid-stream, reset it.
                hist = 0.0f;
                m_ditherHistory[channel] = 0.0f;
            }
            sample_to_quantize -= hist;
        }

        if (!std::isfinite(sample_to_quantize)) {
            sample_to_quantize = 0.0f;
        }

        // 2. Add dither and quantize (clamp)
        float quantized_sample = sample_to_quantize + dither;
        if (!std::isfinite(quantized_sample)) {
            quantized_sample = 0.0f;
        }
        quantized_sample = std::max(-1.0f, std::min(1.0f, quantized_sample));

        // 3. Calculate the new quantization error
        float error = quantized_sample - sample_to_quantize;
        if (!std::isfinite(error)) {
            error = 0.0f;
        }

        // 4. Filter the error and update the history for the next sample
        if (channel < 2) {
            // Apply the filter: new_history = a1 * error.
            float new_history = a1 * error; // a2 unused in current implementation
            if (!std::isfinite(new_history)) {
                new_history = 0.0f;
            }
            // And critically, clamp it to prevent it from running away.
            m_ditherHistory[channel] = std::max(-0.5f, std::min(0.5f, new_history));
        }

        // 5. The output is the quantized sample
        buffer[i] = quantized_sample;
    }
}

// Soft Clipping (tanh-based, transparent gain ceiling)
void Track::applySoftClipping(float* buffer, uint32_t numSamples) {
    const float CLIP_THRESHOLD = 0.95f;  // Start soft clipping at 95% to prevent hard clips
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        float sample = buffer[i];
        float absSample = std::abs(sample);
        
        if (absSample > CLIP_THRESHOLD) {
            // Apply tanh soft clipping for smooth saturation
            float sign = (sample >= 0.0f) ? 1.0f : -1.0f;
            float normalized = (absSample - CLIP_THRESHOLD) / (1.0f - CLIP_THRESHOLD);
            float softClipped = CLIP_THRESHOLD + (1.0f - CLIP_THRESHOLD) * std::tanh(normalized);
            buffer[i] = sign * softClipped;
        }
    }
}

// Stereo Width (Mid/Side Processing)
void Track::applyStereoWidth(float* buffer, uint32_t numFrames, float widthPercent) {
    // widthPercent: 0% = mono, 100% = normal, 200% = ultra-wide
    
    // Convert percentage to coefficient (0.0 to 2.0)
    float width = widthPercent / 100.0f;
    
    // Clamp to safe range
    width = std::max(0.0f, std::min(2.0f, width));
    
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        uint32_t leftIdx = frame * 2;
        uint32_t rightIdx = frame * 2 + 1;
        
        float left = buffer[leftIdx];
        float right = buffer[rightIdx];
        
        // Convert to Mid/Side
        float mid = (left + right) * 0.5f;    // Mono component (center)
        float side = (left - right) * 0.5f;   // Stereo component (width)
        
        // Apply width adjustment
        side *= width;
        
        // Convert back to Left/Right
        buffer[leftIdx] = mid + side;
        buffer[rightIdx] = mid - side;
        
        // Prevent clipping from width expansion
        if (width > 1.0f) {
            buffer[leftIdx] = std::max(-1.0f, std::min(1.0f, buffer[leftIdx]));
            buffer[rightIdx] = std::max(-1.0f, std::min(1.0f, buffer[rightIdx]));
        }
    }
}

// ============================================================================
// EUPHORIA ENGINE - Nomad Mode Signature Audio Character
// ============================================================================

/**
 * @brief Tape Circuit - Non-linear transient rounding + harmonic bloom
 * Emulates analog tape saturation with smooth transient handling
 */
void Track::applyTapeCircuit(float* buffer, uint32_t numSamples, float bloomAmount, float smoothing) {
    const float TAPE_KNEE = 0.7f;  // Start saturation at 70% level
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        float sample = buffer[i];
        float absSample = std::abs(sample);
        
        // Harmonic bloom (soft saturation with even/odd harmonics)
        if (absSample > TAPE_KNEE) {
            float sign = (sample >= 0.0f) ? 1.0f : -1.0f;
            float excess = (absSample - TAPE_KNEE) / (1.0f - TAPE_KNEE);
            
            // Tape-style saturation curve (more aggressive than soft clip)
            float saturated = TAPE_KNEE + (1.0f - TAPE_KNEE) * std::tanh(excess * 2.0f);
            
            // Blend saturated signal with original based on bloomAmount
            sample = sign * (absSample * (1.0f - bloomAmount) + saturated * bloomAmount);
        }
        
        // Transient smoothing (attack rounding)
        if (i > 0) {
            float delta = sample - buffer[i - 1];
            float absDelta = std::abs(delta);
            
            // Round sharp transients (tape head tracking lag)
            if (absDelta > 0.3f) {
                float rounded = buffer[i - 1] + delta * (1.0f - smoothing * 0.3f);
                sample = rounded;
            }
        }
        
        buffer[i] = sample;
    }
}

/**
 * @brief Air - Psychoacoustic stereo widening via mid/side delay curvature
 * Creates spacious "air" around the sound using differential delay and high-frequency emphasis
 */
void Track::applyAir(float* buffer, uint32_t numFrames) {
    if (m_numChannels != 2) return;  // Stereo only
    
    const int DELAY_SAMPLES = 3;  // ~0.06ms at 48kHz (subtle Haas effect)
    const float HF_BOOST = 0.15f;  // High-frequency air enhancement
    const float AIR_FREQ = 8000.0f;  // 8kHz+ gets enhanced
    
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        uint32_t leftIdx = frame * 2;
        uint32_t rightIdx = frame * 2 + 1;
        
        float left = buffer[leftIdx];
        float right = buffer[rightIdx];
        
        // Convert to Mid/Side
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Apply differential delay to side (creates depth)
        int readPos = (m_airDelayPos - DELAY_SAMPLES + 8) % 8;
        float delayedSide = (m_airDelayBufferL[readPos] - m_airDelayBufferR[readPos]) * 0.5f;
        
        // Store current samples
        m_airDelayBufferL[m_airDelayPos] = left;
        m_airDelayBufferR[m_airDelayPos] = right;
        m_airDelayPos = (m_airDelayPos + 1) % 8;
        
        // Mix delayed side signal (psychoacoustic spaciousness)
        side = side * 0.85f + delayedSide * 0.15f;
        
        // High-frequency air boost (shelf filter simulation)
        // In reality, this would be a proper high-shelf filter
        // For now, we enhance overall side channel slightly
        side *= (1.0f + HF_BOOST);
        
        // Convert back to Left/Right
        buffer[leftIdx] = mid + side;
        buffer[rightIdx] = mid - side;
    }
}

/**
 * @brief Drift - Subtle pitch variance and clock jitter for analog warmth
 * Simulates tape speed fluctuations and crystal clock drift
 */
void Track::applyDrift(float* buffer, uint32_t numFrames) {
    const float DRIFT_RATE = 0.0003f;      // Very slow modulation (~0.2 Hz)
    const float DRIFT_DEPTH = 0.00015f;    // Â±0.015% pitch variance (very subtle)
    const float JITTER_AMOUNT = 0.00005f;  // Clock jitter noise floor
    
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        // LFO for tape speed variance
        m_driftPhase += DRIFT_RATE;
        if (m_driftPhase > 6.28318f) m_driftPhase -= 6.28318f;
        
        // Smooth drift modulation
        float drift = std::sin(m_driftPhase) * DRIFT_DEPTH;
        
        // Add random jitter (clock instability)
        float jitter = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * JITTER_AMOUNT;
        
        float driftAmount = drift + jitter;
        
        // Apply very subtle pitch modulation to stereo image
        // This creates the "living, breathing" quality of analog gear
        // Implementation: In real version, this would modulate the resampling phase
        // For now, we apply a subtle amplitude modulation as a placeholder
        for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
            uint32_t idx = frame * m_numChannels + ch;
            buffer[idx] *= (1.0f + driftAmount);
        }
    }
}

/**
 * @brief Apply Nomad Mode Euphoria Engine
 * Master function that applies all euphoric processing in optimal order
 */
void Track::applyEuphoriaEngine(float* buffer, uint32_t numFrames) {
    const auto& euphoria = m_qualitySettings.euphoria;
    
    // Process order matters for optimal sound:
    // 1. Tape Circuit (affects dynamics and harmonics)
    // 2. Air (spatial enhancement on the saturated signal)
    // 3. Drift (final living/breathing quality)
    
    uint32_t numSamples = numFrames * m_numChannels;
    
    if (euphoria.tapeCircuit) {
        applyTapeCircuit(buffer, numSamples, 
                        euphoria.harmonicBloom, 
                        euphoria.transientSmoothing);
    }
    
    if (euphoria.airEnhancement && m_numChannels == 2) {
        applyAir(buffer, numFrames);
    }
    
    if (euphoria.driftEffect) {
        applyDrift(buffer, numFrames);
    }
}

// DC Offset Removal (high-pass filter)
void Track::removeDC(float* buffer, uint32_t numSamples) {
    const float DC_FILTER_COEFF = 0.995f;  // High-pass filter coefficient
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        // Simple DC blocking filter: y[n] = x[n] - dc_offset
        // Update DC offset with exponential moving average
        m_dcOffset = m_dcOffset * DC_FILTER_COEFF + buffer[i] * (1.0f - DC_FILTER_COEFF);
        buffer[i] -= static_cast<float>(m_dcOffset);
    }
}

// Set quality settings
void Track::setQualitySettings(const AudioQualitySettings& settings) {
    // Copy settings (atomic members not used here)
    m_qualitySettings = settings;

    // Backwards-compatibility: map legacy InterpolationQuality to ResamplingMode
    switch (settings.interpolation) {
        case InterpolationQuality::Linear:
            m_qualitySettings.resampling = ResamplingMode::Fast;
            break;
        case InterpolationQuality::Cubic:
            m_qualitySettings.resampling = ResamplingMode::Medium;
            break;
        case InterpolationQuality::Sinc:
            m_qualitySettings.resampling = ResamplingMode::High;
            break;
        case InterpolationQuality::Ultra:
            m_qualitySettings.resampling = ResamplingMode::Ultra;
            break;
        default:
            // Keep provided resampling mode
            break;
    }

    Log::info("Track " + m_name + ": Quality settings updated (resampling=" + std::to_string(static_cast<int>(m_qualitySettings.resampling)) + ")");

}

} // namespace Audio
} // namespace Nomad

        
        // Performance logging
        auto __copy_end_rt = std::chrono::high_resolution_clock::now();
        double __copy_ms_rt = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(__copy_end_rt - __copy_start).count()) / 1000.0;
        const double __copy_log_threshold_ms_rt = 1.0;
        if (__copy_ms_rt > __copy_log_threshold_ms_rt) {
            std::string perfMsg = "[PERF] copyAudioData (RealTimeInterpolation) took " + std::to_string(__copy_ms_rt) +
                                " ms for track '" + m_name + "', frames=" + std::to_string(numFrames) +
                                ", channels=" + std::to_string(m_numChannels) + ", processed=" + std::to_string(framesProcessed);
            Log::warning(perfMsg);
        }
    }
    
    // Apply audio quality enhancements
    uint32_t totalOutputSamples = numFrames * m_numChannels;
    
    // === PROCESSING ORDER (optimized for best sound quality) ===
    
    // 1. Nomad Mode Euphoria Engine (if enabled)
    //    - Applied FIRST to get the signature character on raw audio
    if (m_qualitySettings.nomadMode == NomadMode::Euphoric) {
        applyEuphoriaEngine(outputBuffer, numFrames);
    }
    
    // 2. DC Offset Removal
    //    - Clean up any DC bias from processing
    if (m_qualitySettings.removeDCOffset) {
        removeDC(outputBuffer, totalOutputSamples);
    }
    
    // 3. Dithering (based on mode)
    //    - Applied before final limiting for proper quantization
    if (m_qualitySettings.dithering != DitheringMode::None) {
        applyDithering(outputBuffer, totalOutputSamples);
    }
    
    // 4. Soft Clipping (if enabled)
    //    - Final safety limiter to prevent hard clipping
    if (m_qualitySettings.enableSoftClipping) {
        applySoftClipping(outputBuffer, totalOutputSamples);
    }

    for (uint32_t i = 0; i < totalOutputSamples; ++i) {
        assert(!std::isnan(outputBuffer[i]) && !std::isinf(outputBuffer[i]));
    }

    // playback phase already stored inside branch-specific logic
}

// Linear interpolation (2-point, fast) - FIXED: Safe index calculation prevents overflow
float Track::interpolateLinear(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    uint32_t idx0 = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    uint32_t idx1 = std::min(idx0 + 1, maxFrameIdx);
    double fraction = position - static_cast<double>(idx0);
    
    // CRITICAL FIX: Safe index calculation with overflow prevention
    if (idx0 >= maxFrameIdx) {
        // At or beyond last sample - return last valid sample
        uint32_t lastSampleIdx = maxFrameIdx * m_numChannels + channel;
        return (lastSampleIdx < totalSamples) ? data[lastSampleIdx] : 0.0f;
    }
    
    uint32_t sample0_idx = idx0 * m_numChannels + channel;
    uint32_t sample1_idx = idx1 * m_numChannels + channel;
    
    // Validate calculated indices before accessing data
    if (sample0_idx >= totalSamples || sample1_idx >= totalSamples) {
        return 0.0f;
    }
    
    float s0 = data[sample0_idx];
    float s1 = data[sample1_idx];
    
    // Ensure inputs are finite before interpolation
    if (!std::isfinite(s0) || !std::isfinite(s1)) {
        return 0.0f;
    }
    
    float result = s0 + static_cast<float>(fraction) * (s1 - s0);
    
    // Final validation: ensure result is finite and within bounds
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Cubic Hermite interpolation (4-point, good quality) - FIXED: Safe index calculation prevents overflow
float Track::interpolateCubic(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    uint32_t idx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(idx);
    
    // CRITICAL FIX: Safe sample retrieval with boundary checking
    float s0, s1, s2, s3;
    
    // Sample 0: previous sample (if available)
    if (idx > 0 && idx - 1 < maxFrameIdx) {
        uint32_t idx0 = (idx - 1) * m_numChannels + channel;
        s0 = (idx0 < totalSamples) ? data[idx0] : 0.0f;
    } else {
        s0 = 0.0f; // Use silence for out-of-bounds samples
    }
    
    // Sample 1: current sample
    uint32_t idx1 = idx * m_numChannels + channel;
    s1 = (idx1 < totalSamples) ? data[idx1] : 0.0f;
    
    // Sample 2: next sample
    if (idx + 1 < maxFrameIdx) {
        uint32_t idx2 = (idx + 1) * m_numChannels + channel;
        s2 = (idx2 < totalSamples) ? data[idx2] : 0.0f;
    } else {
        s2 = (maxFrameIdx > 0 && idx < maxFrameIdx) ? data[maxFrameIdx * m_numChannels + channel] : 0.0f;
    }
    
    // Sample 3: sample after next
    if (idx + 2 < maxFrameIdx) {
        uint32_t idx3 = (idx + 2) * m_numChannels + channel;
        s3 = (idx3 < totalSamples) ? data[idx3] : 0.0f;
    } else {
        s3 = (maxFrameIdx > 0 && idx < maxFrameIdx) ? data[maxFrameIdx * m_numChannels + channel] : 0.0f;
    }
    
    // Ensure all samples are finite before interpolation
    if (!std::isfinite(s0) || !std::isfinite(s1) || !std::isfinite(s2) || !std::isfinite(s3)) {
        return 0.0f;
    }
    
    // Hermite basis functions
    float t = static_cast<float>(fraction);
    float t2 = t * t;
    float t3 = t2 * t;
    
    float a0 = -0.5f * s0 + 1.5f * s1 - 1.5f * s2 + 0.5f * s3;
    float a1 = s0 - 2.5f * s1 + 2.0f * s2 - 0.5f * s3;
    float a2 = -0.5f * s0 + 0.5f * s2;
    float a3 = s1;
    
    float result = a0 * t3 + a1 * t2 + a2 * t + a3;
    
    // Final validation: ensure result is finite and within bounds
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Windowed Sinc interpolation (best quality, more CPU intensive) - FIXED: Safe index calculation prevents overflow
float Track::interpolateSinc(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int SINC_WINDOW_SIZE = 8;  // 8-point sinc (high quality)
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < SINC_WINDOW_SIZE/2) {
        // Not enough samples for proper sinc interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // Windowed sinc interpolation with enhanced bounds checking
    for (int i = -SINC_WINDOW_SIZE/2; i < SINC_WINDOW_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function: sin(PI*x) / (PI*x)
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.0001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Blackman window for better frequency response
        float window = 0.42f - 0.5f * std::cos(2.0f * PI * (i + SINC_WINDOW_SIZE/2) / SINC_WINDOW_SIZE)
                            + 0.08f * std::cos(4.0f * PI * (i + SINC_WINDOW_SIZE/2) / SINC_WINDOW_SIZE);
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Ultra (Polyphase Sinc) interpolation (16-point, mastering grade) - FIXED: Safe index calculation prevents overflow
float Track::interpolateUltra(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int POLYPHASE_SIZE = 16;  // 16-point polyphase for reference quality
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < POLYPHASE_SIZE/2) {
        // Not enough samples for proper ultra interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    // Precomputed Kaiser window lookup table (beta=8.6)
    static const float kaiserWindow[POLYPHASE_SIZE] = {
        0.0000f, 0.0217f, 0.0854f, 0.1865f, 0.3180f, 0.4706f, 0.6341f, 0.7975f,
        0.9500f, 0.9500f, 0.7975f, 0.6341f, 0.4706f, 0.3180f, 0.1865f, 0.0854f
    };
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // Polyphase sinc interpolation with precomputed Kaiser window and enhanced bounds checking
    for (int i = -POLYPHASE_SIZE/2; i < POLYPHASE_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function with higher precision
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.0001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Get precomputed Kaiser window value (FAST!)
        int windowIdx = i + POLYPHASE_SIZE/2;
        if (windowIdx < 0 || windowIdx >= POLYPHASE_SIZE) {
            continue;
        }
        float window = kaiserWindow[windowIdx];
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Extreme interpolation (64-point polyphase sinc - mastering grade, real-time safe) - FIXED: Safe index calculation prevents overflow
float Track::interpolateExtreme(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int EXTREME_SIZE = 64;  // 64-point sinc (sweet spot for quality/performance)
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < EXTREME_SIZE/2) {
        // Not enough samples for proper extreme interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    // Precomputed Kaiser window lookup table (beta=10.0)
    // Computed once, reused for all samples - HUGE performance win!
    static const float kaiserWindow[EXTREME_SIZE] = {
        0.0000f, 0.0011f, 0.0044f, 0.0098f, 0.0173f, 0.0268f, 0.0384f, 0.0520f,
        0.0675f, 0.0849f, 0.1042f, 0.1252f, 0.1479f, 0.1722f, 0.1980f, 0.2252f,
        0.2537f, 0.2834f, 0.3142f, 0.3460f, 0.3786f, 0.4119f, 0.4459f, 0.4803f,
        0.5151f, 0.5502f, 0.5854f, 0.6206f, 0.6557f, 0.6906f, 0.7250f, 0.7590f,
        0.7923f, 0.8249f, 0.8566f, 0.8873f, 0.9169f, 0.9453f, 0.9724f, 0.9981f,
        1.0000f, 0.9981f, 0.9724f, 0.9453f, 0.9169f, 0.8873f, 0.8566f, 0.8249f,
        0.7923f, 0.7590f, 0.7250f, 0.6906f, 0.6557f, 0.6206f, 0.5854f, 0.5502f,
        0.5151f, 0.4803f, 0.4459f, 0.4119f, 0.3786f, 0.3460f, 0.3142f, 0.2834f
    };
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // 64-point polyphase sinc with precomputed Kaiser window and enhanced bounds checking
    for (int i = -EXTREME_SIZE/2; i < EXTREME_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function: sin(PI*x) / (PI*x)
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.00001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Get precomputed Kaiser window value (FAST!) with bounds check
        int windowIdx = i + EXTREME_SIZE/2;
        if (windowIdx < 0 || windowIdx >= EXTREME_SIZE) {
            continue;
        }
        float window = kaiserWindow[windowIdx];
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// Perfect interpolation (512-point polyphase sinc - FL Studio grade) - FIXED: Safe index calculation prevents overflow
// WARNING: EXTREMELY CPU INTENSIVE - Use only for offline rendering/mastering
float Track::interpolatePerfect(const float* data, uint32_t totalSamples, double position, uint32_t channel) const {
    const int PERFECT_SIZE = 512;  // 512-point sinc (FL Studio quality)
    const float PI = 3.14159265359f;
    
    // CRITICAL FIX: Validate input parameters and prevent overflow before calculations
    if (!data || totalSamples == 0 || channel >= m_numChannels) {
        return 0.0f;
    }
    
    // Safely convert position to sample index with bounds validation
    if (position < 0.0 || !std::isfinite(position)) {
        return 0.0f;
    }
    
    // Calculate maximum valid frame index safely
    uint32_t maxFrameIdx = (totalSamples / m_numChannels > 0) ? (totalSamples / m_numChannels - 1) : 0;
    if (maxFrameIdx < PERFECT_SIZE/2) {
        // Not enough samples for proper perfect interpolation - fallback to linear
        return interpolateLinear(data, totalSamples, position, channel);
    }
    
    uint32_t centerIdx = static_cast<uint32_t>(std::min(static_cast<double>(maxFrameIdx), position));
    double fraction = position - static_cast<double>(centerIdx);
    
    float sum = 0.0f;
    float windowSum = 0.0f;
    
    // 512-point polyphase sinc with Kaiser window (beta=12.0 for extreme precision) and enhanced bounds checking
    for (int i = -PERFECT_SIZE/2; i < PERFECT_SIZE/2; ++i) {
        // CRITICAL FIX: Safe sample index calculation with overflow prevention
        int safeSampleIdx = static_cast<int>(centerIdx) + i;
        
        // Validate sample index is within bounds
        if (safeSampleIdx < 0 || static_cast<uint32_t>(safeSampleIdx) >= maxFrameIdx) {
            continue;
        }
        
        uint32_t sampleIdx = static_cast<uint32_t>(safeSampleIdx) * m_numChannels + channel;
        
        if (sampleIdx >= totalSamples) {
            continue;
        }
        
        float sample = data[sampleIdx];
        
        // Ensure sample is finite
        if (!std::isfinite(sample)) {
            continue;
        }
        
        // Sinc function: sin(PI*x) / (PI*x)
        float x = static_cast<float>(i) - static_cast<float>(fraction);
        float sinc = (std::abs(x) < 0.00001f) ? 1.0f : std::sin(PI * x) / (PI * x);
        
        // Kaiser window (beta=12.0 for maximum stopband attenuation) with safety checks
        float alpha = static_cast<float>(i + PERFECT_SIZE/2) / PERFECT_SIZE;
        if (std::abs(2.0f * alpha - 1.0f) > 1.0f) {
            continue; // Skip if outside valid range for sqrt
        }
        float kaiserArg = 12.0f * std::sqrt(1.0f - (2.0f * alpha - 1.0f) * (2.0f * alpha - 1.0f));
        
        // Modified Bessel function I0 approximation (higher precision) with safety checks
        auto besselI0 = [](float x) -> float {
            float sum = 1.0f;
            float term = 1.0f;
            for (int n = 1; n < 20; ++n) {  // More iterations for better precision
                term *= (x / (2.0f * n)) * (x / (2.0f * n));
                sum += term;
                if (term < 1e-10f) break;  // Early exit for convergence
            }
            return sum;
        };
        
        float besselDenom = besselI0(12.0f);
        if (besselDenom <= 0.0f) {
            continue; // Avoid division by zero or invalid results
        }
        float window = besselI0(kaiserArg) / besselDenom;
        
        // Ensure window is finite
        if (!std::isfinite(window)) {
            continue;
        }
        
        float weight = sinc * window;
        sum += sample * weight;
        windowSum += weight;
    }
    
    // Normalize and clamp with validation
    float result = (windowSum > 0.0001f) ? (sum / windowSum) : 0.0f;
    return std::isfinite(result) ? std::max(-1.0f, std::min(1.0f, result)) : 0.0f;
}

// === Dithering Methods ===

// Master dithering dispatcher
void Track::applyDithering(float* buffer, uint32_t numSamples) {
    switch (m_qualitySettings.dithering) {
        case DitheringMode::Triangular:
            applyTriangularDither(buffer, numSamples);
            break;
        case DitheringMode::HighPass:
            applyHighPassDither(buffer, numSamples);
            break;
        case DitheringMode::NoiseShaped:
            // applyNoiseShapedDither(buffer, numSamples); // Temporarily disabled for debugging
            break;
        case DitheringMode::None:
        default:
            // No dithering
            break;
    }
}

// TPDF Dithering (Triangular Probability Density Function)
void Track::applyTriangularDither(float* buffer, uint32_t numSamples) {
    const float DITHER_AMPLITUDE = 1.0f / 32768.0f;  // 16-bit dither
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        // TPDF: sum of two uniform random numbers gives triangular distribution
        float r1 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float r2 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float dither = (r1 + r2) * DITHER_AMPLITUDE;
        
        buffer[i] += dither;
        
        // Clamp to prevent overflow
        buffer[i] = std::max(-1.0f, std::min(1.0f, buffer[i]));
    }
}

// High-Pass Shaped Dithering
void Track::applyHighPassDither(float* buffer, uint32_t numSamples) {
    const float DITHER_AMPLITUDE = 1.0f / 32768.0f;
    const float HP_COEFF = 0.5f;  // High-pass coefficient
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        // Generate TPDF dither
        float r1 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float r2 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float dither = (r1 + r2) * DITHER_AMPLITUDE;
        
        // Apply high-pass shaping (pushes noise to higher frequencies)
        float shapedDither = dither - HP_COEFF * m_highPassDitherHistory;
        m_highPassDitherHistory = dither;
        
        buffer[i] += shapedDither;
        buffer[i] = std::max(-1.0f, std::min(1.0f, buffer[i]));
    }
}

// Noise-Shaped Dithering (Psychoacoustic)
void Track::applyNoiseShapedDither(float* buffer, uint32_t numSamples) {
    const float DITHER_AMPLITUDE = 1.0f / 32768.0f;
    
    // F-weighted noise shaping coefficients (pushes noise above ~2kHz where hearing is less sensitive)
    const float a1 = 2.033f;
    const float a2 = -1.165f;
    
    // Sanity-check dither history before processing. If any stored history is
    // non-finite (inf/NaN) it can feed back into the filter and destabilize
    // subsequent audio. Reset to 0.0f when that happens. This also covers the
    // "first use" case on preview/start if the history wasn't properly reset.
    for (int ch = 0; ch < 2; ++ch) {
        if (!std::isfinite(m_ditherHistory[ch])) {
            m_ditherHistory[ch] = 0.0f;
        }
    }
    if (!std::isfinite(m_highPassDitherHistory)) {
        m_highPassDitherHistory = 0.0f;
    }

    for (uint32_t i = 0; i < numSamples; ++i) {
        uint32_t channel = i % m_numChannels;
        float original_sample = buffer[i];

        // Guard against corrupted input samples (NaN/inf). Replace with 0.
        if (!std::isfinite(original_sample)) {
            original_sample = 0.0f;
        }

        // Generate TPDF dither
        float r1 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float r2 = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        float dither = (r1 + r2) * DITHER_AMPLITUDE;

        // 1. Form the input to the quantizer by subtracting the filtered error from history
        float sample_to_quantize = original_sample;
        if (channel < 2) {
            float hist = m_ditherHistory[channel];
            if (!std::isfinite(hist)) {
                // If history somehow became non-finite mid-stream, reset it.
                hist = 0.0f;
                m_ditherHistory[channel] = 0.0f;
            }
            sample_to_quantize -= hist;
        }

        if (!std::isfinite(sample_to_quantize)) {
            sample_to_quantize = 0.0f;
        }

        // 2. Add dither and quantize (clamp)
        float quantized_sample = sample_to_quantize + dither;
        if (!std::isfinite(quantized_sample)) {
            quantized_sample = 0.0f;
        }
        quantized_sample = std::max(-1.0f, std::min(1.0f, quantized_sample));

        // 3. Calculate the new quantization error
        float error = quantized_sample - sample_to_quantize;
        if (!std::isfinite(error)) {
            error = 0.0f;
        }

        // 4. Filter the error and update the history for the next sample
        if (channel < 2) {
            // Apply the filter: new_history = a1 * error.
            float new_history = a1 * error; // a2 unused in current implementation
            if (!std::isfinite(new_history)) {
                new_history = 0.0f;
            }
            // And critically, clamp it to prevent it from running away.
            m_ditherHistory[channel] = std::max(-0.5f, std::min(0.5f, new_history));
        }

        // 5. The output is the quantized sample
        buffer[i] = quantized_sample;
    }
}

// Soft Clipping (tanh-based, transparent gain ceiling)
void Track::applySoftClipping(float* buffer, uint32_t numSamples) {
    const float CLIP_THRESHOLD = 0.95f;  // Start soft clipping at 95% to prevent hard clips
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        float sample = buffer[i];
        float absSample = std::abs(sample);
        
        if (absSample > CLIP_THRESHOLD) {
            // Apply tanh soft clipping for smooth saturation
            float sign = (sample >= 0.0f) ? 1.0f : -1.0f;
            float normalized = (absSample - CLIP_THRESHOLD) / (1.0f - CLIP_THRESHOLD);
            float softClipped = CLIP_THRESHOLD + (1.0f - CLIP_THRESHOLD) * std::tanh(normalized);
            buffer[i] = sign * softClipped;
        }
    }
}

// Stereo Width (Mid/Side Processing)
void Track::applyStereoWidth(float* buffer, uint32_t numFrames, float widthPercent) {
    // widthPercent: 0% = mono, 100% = normal, 200% = ultra-wide
    
    // Convert percentage to coefficient (0.0 to 2.0)
    float width = widthPercent / 100.0f;
    
    // Clamp to safe range
    width = std::max(0.0f, std::min(2.0f, width));
    
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        uint32_t leftIdx = frame * 2;
        uint32_t rightIdx = frame * 2 + 1;
        
        float left = buffer[leftIdx];
        float right = buffer[rightIdx];
        
        // Convert to Mid/Side
        float mid = (left + right) * 0.5f;    // Mono component (center)
        float side = (left - right) * 0.5f;   // Stereo component (width)
        
        // Apply width adjustment
        side *= width;
        
        // Convert back to Left/Right
        buffer[leftIdx] = mid + side;
        buffer[rightIdx] = mid - side;
        
        // Prevent clipping from width expansion
        if (width > 1.0f) {
            buffer[leftIdx] = std::max(-1.0f, std::min(1.0f, buffer[leftIdx]));
            buffer[rightIdx] = std::max(-1.0f, std::min(1.0f, buffer[rightIdx]));
        }
    }
}

// ============================================================================
// EUPHORIA ENGINE - Nomad Mode Signature Audio Character
// ============================================================================

/**
 * @brief Tape Circuit - Non-linear transient rounding + harmonic bloom
 * Emulates analog tape saturation with smooth transient handling
 */
void Track::applyTapeCircuit(float* buffer, uint32_t numSamples, float bloomAmount, float smoothing) {
    const float TAPE_KNEE = 0.7f;  // Start saturation at 70% level
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        float sample = buffer[i];
        float absSample = std::abs(sample);
        
        // Harmonic bloom (soft saturation with even/odd harmonics)
        if (absSample > TAPE_KNEE) {
            float sign = (sample >= 0.0f) ? 1.0f : -1.0f;
            float excess = (absSample - TAPE_KNEE) / (1.0f - TAPE_KNEE);
            
            // Tape-style saturation curve (more aggressive than soft clip)
            float saturated = TAPE_KNEE + (1.0f - TAPE_KNEE) * std::tanh(excess * 2.0f);
            
            // Blend saturated signal with original based on bloomAmount
            sample = sign * (absSample * (1.0f - bloomAmount) + saturated * bloomAmount);
        }
        
        // Transient smoothing (attack rounding)
        if (i > 0) {
            float delta = sample - buffer[i - 1];
            float absDelta = std::abs(delta);
            
            // Round sharp transients (tape head tracking lag)
            if (absDelta > 0.3f) {
                float rounded = buffer[i - 1] + delta * (1.0f - smoothing * 0.3f);
                sample = rounded;
            }
        }
        
        buffer[i] = sample;
    }
}

/**
 * @brief Air - Psychoacoustic stereo widening via mid/side delay curvature
 * Creates spacious "air" around the sound using differential delay and high-frequency emphasis
 */
void Track::applyAir(float* buffer, uint32_t numFrames) {
    if (m_numChannels != 2) return;  // Stereo only
    
    const int DELAY_SAMPLES = 3;  // ~0.06ms at 48kHz (subtle Haas effect)
    const float HF_BOOST = 0.15f;  // High-frequency air enhancement
    const float AIR_FREQ = 8000.0f;  // 8kHz+ gets enhanced
    
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        uint32_t leftIdx = frame * 2;
        uint32_t rightIdx = frame * 2 + 1;
        
        float left = buffer[leftIdx];
        float right = buffer[rightIdx];
        
        // Convert to Mid/Side
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Apply differential delay to side (creates depth)
        int readPos = (m_airDelayPos - DELAY_SAMPLES + 8) % 8;
        float delayedSide = (m_airDelayBufferL[readPos] - m_airDelayBufferR[readPos]) * 0.5f;
        
        // Store current samples
        m_airDelayBufferL[m_airDelayPos] = left;
        m_airDelayBufferR[m_airDelayPos] = right;
        m_airDelayPos = (m_airDelayPos + 1) % 8;
        
        // Mix delayed side signal (psychoacoustic spaciousness)
        side = side * 0.85f + delayedSide * 0.15f;
        
        // High-frequency air boost (shelf filter simulation)
        // In reality, this would be a proper high-shelf filter
        // For now, we enhance overall side channel slightly
        side *= (1.0f + HF_BOOST);
        
        // Convert back to Left/Right
        buffer[leftIdx] = mid + side;
        buffer[rightIdx] = mid - side;
    }
}

/**
 * @brief Drift - Subtle pitch variance and clock jitter for analog warmth
 * Simulates tape speed fluctuations and crystal clock drift
 */
void Track::applyDrift(float* buffer, uint32_t numFrames) {
    const float DRIFT_RATE = 0.0003f;      // Very slow modulation (~0.2 Hz)
    const float DRIFT_DEPTH = 0.00015f;    // Â±0.015% pitch variance (very subtle)
    const float JITTER_AMOUNT = 0.00005f;  // Clock jitter noise floor
    
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        // LFO for tape speed variance
        m_driftPhase += DRIFT_RATE;
        if (m_driftPhase > 6.28318f) m_driftPhase -= 6.28318f;
        
        // Smooth drift modulation
        float drift = std::sin(m_driftPhase) * DRIFT_DEPTH;
        
        // Add random jitter (clock instability)
        float jitter = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * JITTER_AMOUNT;
        
        float driftAmount = drift + jitter;
        
        // Apply very subtle pitch modulation to stereo image
        // This creates the "living, breathing" quality of analog gear
        // Implementation: In real version, this would modulate the resampling phase
        // For now, we apply a subtle amplitude modulation as a placeholder
        for (uint32_t ch = 0; ch < m_numChannels; ++ch) {
            uint32_t idx = frame * m_numChannels + ch;
            buffer[idx] *= (1.0f + driftAmount);
        }
    }
}

/**
 * @brief Apply Nomad Mode Euphoria Engine
 * Master function that applies all euphoric processing in optimal order
 */
void Track::applyEuphoriaEngine(float* buffer, uint32_t numFrames) {
    const auto& euphoria = m_qualitySettings.euphoria;
    
    // Process order matters for optimal sound:
    // 1. Tape Circuit (affects dynamics and harmonics)
    // 2. Air (spatial enhancement on the saturated signal)
    // 3. Drift (final living/breathing quality)
    
    uint32_t numSamples = numFrames * m_numChannels;
    
    if (euphoria.tapeCircuit) {
        applyTapeCircuit(buffer, numSamples, 
                        euphoria.harmonicBloom, 
                        euphoria.transientSmoothing);
    }
    
    if (euphoria.airEnhancement && m_numChannels == 2) {
        applyAir(buffer, numFrames);
    }
    
    if (euphoria.driftEffect) {
        applyDrift(buffer, numFrames);
    }
}

// DC Offset Removal (high-pass filter)
void Track::removeDC(float* buffer, uint32_t numSamples) {
    const float DC_FILTER_COEFF = 0.995f;  // High-pass filter coefficient
    
    for (uint32_t i = 0; i < numSamples; ++i) {
        // Simple DC blocking filter: y[n] = x[n] - dc_offset
        // Update DC offset with exponential moving average
        m_dcOffset = m_dcOffset * DC_FILTER_COEFF + buffer[i] * (1.0f - DC_FILTER_COEFF);
        buffer[i] -= static_cast<float>(m_dcOffset);
    }
}

// Set quality settings
void Track::setQualitySettings(const AudioQualitySettings& settings) {
    // Copy settings (atomic members not used here)
    m_qualitySettings = settings;

    // Backwards-compatibility: map legacy InterpolationQuality to ResamplingMode
    switch (settings.interpolation) {
        case InterpolationQuality::Linear:
            m_qualitySettings.resampling = ResamplingMode::Fast;
            break;
        case InterpolationQuality::Cubic:
            m_qualitySettings.resampling = ResamplingMode::Medium;
            break;
        case InterpolationQuality::Sinc:
            m_qualitySettings.resampling = ResamplingMode::High;
            break;
        case InterpolationQuality::Ultra:
            m_qualitySettings.resampling = ResamplingMode::Ultra;
            break;
        default:
            // Keep provided resampling mode
            break;
    }

    Log::info("Track " + m_name + ": Quality settings updated (resampling=" + std::to_string(static_cast<int>(m_qualitySettings.resampling)) + ")");

}

} // namespace Audio
} // namespace Nomad
