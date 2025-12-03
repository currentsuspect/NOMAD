// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "MixerBus.h"
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include "SamplePool.h"

namespace Nomad {
namespace Audio {

/**
 * @brief Audio track states
 */
enum class TrackState {
    Empty,      // No audio data
    Loaded,     // Audio file loaded
    Recording,  // Currently recording
    Playing,    // Currently playing
    Paused,     // Playback paused
    Stopped     // Playback stopped
};

/**
 * @brief Interpolation quality modes
 */
enum class InterpolationQuality {
    Linear,     // Fast, basic quality (2-point linear)
    Cubic,      // Good quality (4-point cubic Hermite)
    Sinc,       // Best quality (8-point windowed sinc)
    Ultra       // Mastering grade (16-point polyphase sinc)
};

/**
 * @brief Resampling mode for sample rate conversion
 */
enum class ResamplingMode {
    Fast,       // Linear interpolation (2-point)
    Medium,     // Cubic interpolation (4-point)
    High,       // Windowed sinc (8-point)
    Ultra,      // Polyphase sinc (16-point, reference grade)
    Extreme,    // Polyphase sinc (64-point, mastering grade)
    Perfect     // Polyphase sinc (512-point, FL Studio grade - extreme quality)
};

/**
 * @brief Dithering modes
 */
enum class DitheringMode {
    None,           // No dithering
    Triangular,     // TPDF (Triangular Probability Density Function)
    HighPass,       // High-pass shaped dither
    NoiseShaped     // Psychoacoustic noise shaping (pushes noise above hearing range)
};

/**
 * @brief Internal processing precision
 */
enum class InternalPrecision {
    Float32,    // 32-bit float (realtime)
    Float64     // 64-bit double (mastering, reduces rounding errors)
};

/**
 * @brief Oversampling modes
 */
enum class OversamplingMode {
    None,       // No oversampling
    Auto,       // Automatic (enable for nonlinear effects only)
    Force2x,    // Force 2x oversampling
    Force4x     // Force 4x oversampling (mastering)
};

/**
 * @brief Nomad Mode - Sonic character toggle
 */
enum class NomadMode {
    Off,            // Disabled (bypass all Nomad Mode processing)
    Transparent,    // Clinical precision, reference-grade (default)
    Euphoric        // Analog soul: harmonic warmth, smooth transients, rich tails
};

/**
 * @brief Quality presets for easy configuration
 */
enum class QualityPreset {
    Custom,         // User-defined settings
    Economy,        // Low CPU: Linear, no oversampling, 32-bit
    Balanced,       // Recommended: Cubic, auto oversampling, 32-bit
    HighFidelity,   // High quality: Sinc, 2x oversampling, noise-shaped dither
    Mastering       // Maximum: Ultra sinc, 4x oversampling, 64-bit, full processing
};

/**
 * @brief Comprehensive audio quality settings
 */
struct AudioQualitySettings {
    // Core Quality
    ResamplingMode resampling{ResamplingMode::Medium};
    DitheringMode dithering{DitheringMode::Triangular};
    InternalPrecision precision{InternalPrecision::Float32};
    OversamplingMode oversampling{OversamplingMode::None};
    
    // Legacy compatibility (maps to resampling)
    InterpolationQuality interpolation{InterpolationQuality::Cubic};
    
    // Processing options
    bool removeDCOffset{true};          // Remove DC bias from audio
    bool enableSoftClipping{false};     // Soft-clip protection on output
    bool autoGainNormalization{false};  // LUFS-based auto gain (future)
    
    // Nomad Mode - Sonic Character
    NomadMode nomadMode{NomadMode::Off};  // Off / Transparent / Euphoric
    
    // Euphoria Engine Settings (active when nomadMode == Euphoric)
    struct EuphoriaSettings {
        bool tapeCircuit{true};         // Non-linear transient rounding + harmonic bloom
        bool airEnhancement{true};       // Psychoacoustic stereo widening (mid/side delay)
        bool driftEffect{false};         // Subtle detune & clock variance (warmth)
        float harmonicBloom{0.15f};     // Harmonic saturation amount (0.0 - 1.0)
        float transientSmoothing{0.25f}; // Transient rounding (0.0 - 1.0)
    } euphoria;
    
    // Anti-aliasing filter steepness (for resampling)
    enum class FilterSteepness { Soft, Medium, Steep } antiAliasingFilter{FilterSteepness::Medium};
    
    // Quality preset tracking
    QualityPreset preset{QualityPreset::Balanced};
    
    AudioQualitySettings() = default;
    
    // Preset constructors
    static AudioQualitySettings Economy();
    static AudioQualitySettings Balanced();
    static AudioQualitySettings HighFidelity();
    static AudioQualitySettings Mastering();
    
    // Apply preset
    void applyPreset(QualityPreset preset);
};

/**
 * @brief Audio track class for DAW
 *
 * Manages individual audio tracks with:
 * - Track properties (name, color, volume, pan, mute, solo)
 * - Audio data management (sample buffers, file loading)
 * - Recording functionality
 * - Real-time parameter control
 */
class Track {
public:
    Track(const std::string& name = "Track", uint32_t trackId = 0);
    ~Track();

    // Track Properties
    void setName(const std::string& name);
    const std::string& getName() const { return m_name; }

    void setColor(uint32_t color);  // ARGB format
    uint32_t getColor() const { return m_color; }

    uint32_t getTrackId() const { return m_trackId; }

    // Audio Parameters (thread-safe)
    void setVolume(float volume);  // 0.0 to 1.0
    float getVolume() const { return m_volume.load(); }

    void setPan(float pan);  // -1.0 (left) to 1.0 (right)
    float getPan() const { return m_pan.load(); }

    void setMute(bool mute);
    bool isMuted() const { return m_muted.load(); }

    void setSolo(bool solo);
    bool isSoloed() const { return m_soloed.load(); }
    
    // System track flag (preview, test sound, etc - not affected by transport)
    void setSystemTrack(bool isSystem) { m_isSystemTrack = isSystem; }
    bool isSystemTrack() const { return m_isSystemTrack; }

    // Track State
    void setState(TrackState state);
    TrackState getState() const { return m_state.load(); }

    // Audio Data Management
    bool loadAudioFile(const std::string& filePath);
    bool generatePreviewTone(const std::string& filePath);
    bool generateDemoAudio(const std::string& filePath);
    void setAudioData(const float* data, uint32_t numSamples, uint32_t sampleRate, uint32_t numChannels);
    void clearAudioData();
    
    /**
     * Provide read-only access to the track's stored audio samples.
     * @returns A const reference to the vector containing the track's audio samples.
     */
    
    /**
     * Get the sample rate used by the track's audio data.
     * @returns The sample rate in hertz.
     */
    
    /**
     * Get the number of channels for the track's audio data.
     * @returns The number of channels (for example, 1 = mono, 2 = stereo).
     */
    const std::vector<float>& getAudioData() const;
    uint32_t getSampleRate() const { return m_sampleRate; }
    uint32_t getNumChannels() const { return m_numChannels; }

    // Recording
    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_state.load() == TrackState::Recording; }

    // Playback Control
    void play();
    void pause();
    void stop();
    bool isPlaying() const { return m_state.load() == TrackState::Playing; }

    // Position Control
    void setPosition(double seconds);
    double getPosition() const { return m_positionSeconds.load(); }
    double getDuration() const { return m_durationSeconds.load(); }
    
    /**
 * Set the track's start position within the project timeline.
 *
 * @param seconds Start position in the timeline, in seconds.
 */
    void setStartPositionInTimeline(double seconds) { m_startPositionInTimeline.store(seconds); }
    double getStartPositionInTimeline() const { return m_startPositionInTimeline.load(); }
    /**
 * Set the source file path associated with the track.
 * @param path Filesystem path to the audio source; stored on the track for reference and streaming.
 */
void setSourcePath(const std::string& path) { m_sourcePath = path; }
    const std::string& getSourcePath() const { return m_sourcePath; }

    // Audio Processing
    void processAudio(float* outputBuffer, uint32_t numFrames, double streamTime, double outputSampleRate);

    /**
 * Access the track's mixer bus.
 *
 * @returns Pointer to the track's MixerBus, or `nullptr` if no mixer bus is assigned.
 */
    MixerBus* getMixerBus() { return m_mixerBus.get(); }
    const MixerBus* getMixerBus() const { return m_mixerBus.get(); }
    
    // Latency Compensation
    void setLatencyCompensation(double inputLatencyMs, double outputLatencyMs);
    double getLatencyCompensationMs() const { return m_latencyCompensationMs; }
    
    // Audio Quality Settings
    void setQualitySettings(const AudioQualitySettings& settings);
    const AudioQualitySettings& getQualitySettings() const { return m_qualitySettings; }

private:
    // Track identification
    std::string m_name;
    uint32_t m_trackId;
    uint32_t m_color;  // ARGB format
    bool m_isSystemTrack{false};  // System tracks (preview, test sound) aren't affected by transport

    // Audio parameters (atomic for thread safety)
    std::atomic<float> m_volume{1.0f};
    std::atomic<float> m_pan{0.0f};
    std::atomic<bool> m_muted{false};
    std::atomic<bool> m_soloed{false};

    // Track state
    std::atomic<TrackState> m_state{TrackState::Empty};
    std::atomic<double> m_positionSeconds{0.0};
    std::atomic<double> m_durationSeconds{0.0};
    std::atomic<double> m_startPositionInTimeline{0.0};  // Where sample starts in timeline (seconds)

    // Audio data
    std::vector<float> m_audioData;  // Interleaved stereo samples (streaming/recording/temp)
    std::shared_ptr<AudioBuffer> m_sampleBuffer; // Shared decoded buffer from SamplePool (non-streaming)
    uint32_t m_sampleRate{48000};
    /**
 * Number of audio channels for the track (1 = mono, 2 = stereo, etc.).
 *
 * Defaults to 2 (stereo).
 */
uint32_t m_numChannels{2};       /**
 * Number of channels in the source audio; set to 2 when downmixed to stereo.
 *
 * Represents the source channel count tracked by the Track instance.
 */
    uint32_t m_sourceChannels{2};    // Original channel count on load
    std::string m_sourcePath;
    std::atomic<double> m_playbackPhase{0.0};  // For sample-accurate playback
    mutable std::mutex m_audioDataMutex;
    bool m_streaming{false};
    std::thread m_streamThread;
    std::atomic<bool> m_streamStop{false};
    std::condition_variable m_streamCv;
    std::mutex m_streamMutex;
    /**
 * Base frame index for streaming operations.
 *
 * Serves as the absolute frame offset (sample frame, i.e., one sample per channel) corresponding
 * to the start of the current stream buffer; used to compute positions and seek offsets when
 * streaming audio from a file. Defaults to 0.
 */
uint64_t m_streamBaseFrame{0};     /**
 * Absolute frame index corresponding to m_audioData[0].
 *
 * Acts as the global frame offset for streamed audio data, allowing
 * conversion between buffer-local sample indices and the overall stream timeline.
 */
    uint64_t m_streamTotalFrames{0};   // total frames in source
    bool m_streamEof{false};
    /**
 * Number of bytes per audio sample for the active stream.
 *
 * Holds the byte width used when reading streamed audio data (e.g., 1, 2, 3, or 4 bytes per sample).
 * A value of 0 indicates the stream's sample size is not yet determined.
 */
uint32_t m_streamBytesPerSample{0};
    /**
 * Offset within the current stream data buffer where the next read/write will occur.
 *
 * Represents the byte offset measured from the start of the stream data (file or buffer).
 */
uint32_t m_streamDataOffset{0};
    std::ifstream m_streamFile;

    // Mixer integration
    std::unique_ptr<MixerBus> m_mixerBus;

    // Recording state
    std::vector<float> m_recordingBuffer;
    std::atomic<bool> m_isRecording{false};
    
    /**
 * Fill the provided buffer with silence (zeros) for the given number of frames.
 * @param buffer Pointer to the output buffer to clear. Must be at least numFrames * channels sized by caller.
 * @param numFrames Number of audio frames to generate.
 */
/**
 * Copy track audio data into the provided output buffer, performing any necessary resampling to match the output sample rate.
 * @param outputBuffer Destination buffer to receive copied audio samples.
 * @param numFrames Number of output frames to produce.
 * @param outputSampleRate Sample rate of the output buffer used for resampling decisions.
 */
/**
 * Trim internal streaming buffers based on the current playback frame to free consumed data and update stream state.
 * @param currentFrame Current absolute frame index in the stream timeline.
 */
/**
 * Stop any active streaming operations and join/cleanup streaming thread resources.
 */
/**
 * Initialize WAV streaming from a file with the specified format and starting frame.
 * @param filePath Path to the WAV file to stream.
 * @param sampleRate Sample rate of the WAV file.
 * @param channels Number of channels in the WAV file.
 * @param bitsPerSample Bits per sample in the WAV file (e.g., 16, 24, 32).
 * @param dataOffset Byte offset within the file where PCM data begins.
 * @param dataSize Size in bytes of the PCM data region.
 * @param startFrame Frame index within the WAV data at which to begin streaming.
 * @returns `true` if streaming was successfully started, `false` otherwise.
 */
/**
 * Thread entry point that performs continuous WAV streaming into internal buffers for the specified channel count.
 * @param channels Number of channels to stream.
 */
/**
 * Perform linear interpolation of the interleaved audio data at the given fractional sample position for the specified channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples (per-channel * channels) available in data.
 * @param position Fractional sample index (in samples, not frames) to sample from.
 * @param channel Channel index to read from within interleaved data.
 * @returns Interpolated sample value.
 */
/**
 * Perform cubic interpolation of the interleaved audio data at the given fractional sample position for the specified channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples (per-channel * channels) available in data.
 * @param position Fractional sample index (in samples, not frames) to sample from.
 * @param channel Channel index to read from within interleaved data.
 * @returns Interpolated sample value.
 */
/**
 * Perform band-limited sinc interpolation of the interleaved audio data at the given fractional sample position for the specified channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples (per-channel * channels) available in data.
 * @param position Fractional sample index (in samples, not frames) to sample from.
 * @param channel Channel index to read from within interleaved data.
 * @returns Interpolated sample value.
 */
/**
 * Perform "ultra" quality interpolation of the interleaved audio data at the given fractional sample position for the specified channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples (per-channel * channels) available in data.
 * @param position Fractional sample index (in samples, not frames) to sample from.
 * @param channel Channel index to read from within interleaved data.
 * @returns Interpolated sample value.
 */
/**
 * Perform "extreme" quality interpolation of the interleaved audio data at the given fractional sample position for the specified channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples (per-channel * channels) available in data.
 * @param position Fractional sample index (in samples, not frames) to sample from.
 * @param channel Channel index to read from within interleaved data.
 * @returns Interpolated sample value.
 */
/**
 * Perform "perfect" quality interpolation of the interleaved audio data at the given fractional sample position for the specified channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples (per-channel * channels) available in data.
 * @param position Fractional sample index (in samples, not frames) to sample from.
 * @param channel Channel index to read from within interleaved data.
 * @returns Interpolated sample value.
 */
/**
 * Apply the configured dithering strategy to the provided buffer of samples.
 * @param buffer Buffer of samples to dither (in-place).
 * @param numSamples Number of samples in the buffer.
 */
/**
 * Apply triangular dithering to the provided buffer of samples.
 * @param buffer Buffer of samples to dither (in-place).
 * @param numSamples Number of samples in the buffer.
 */
/**
 * Apply high-pass shaped dithering to the provided buffer of samples.
 * @param buffer Buffer of samples to dither (in-place).
 * @param numSamples Number of samples in the buffer.
 */
/**
 * Apply noise-shaped dithering using internal noise shaping history to the provided buffer.
 * @param buffer Buffer of samples to dither (in-place).
 * @param numSamples Number of samples in the buffer.
 */
/**
 * Remove accumulated DC offset from the provided buffer and update internal DC tracking.
 * @param buffer Buffer of samples to process (in-place).
 * @param numSamples Number of samples in the buffer.
 */
/**
 * Apply soft clipping to the provided buffer to tame peaks and introduce gentle saturation.
 * @param buffer Buffer of samples to process (in-place).
 * @param numSamples Number of samples in the buffer.
 */
/**
 * Adjust stereo image width of an interleaved buffer according to width percentage.
 * @param buffer Interleaved stereo buffer to modify (in-place).
 * @param numFrames Number of frames in the buffer.
 * @param widthPercent Stereo width percentage where 100.0 preserves original width.
 */
/**
 * Apply the Euphoria (Nomad) processing chain to the buffer when enabled by quality settings.
 * @param buffer Interleaved buffer to process (in-place).
 * @param numFrames Number of frames in the buffer.
 */
/**
 * Apply tape-circuit style processing to the buffer introducing bloom and smoothing.
 * @param buffer Buffer of samples to process (in-place).
 * @param numSamples Number of samples in the buffer.
 * @param bloomAmount Amount of harmonic bloom to apply.
 * @param smoothing Amount of transient smoothing to apply.
 */
/**
 * Apply an "air" enhancement effect to the buffer to emphasize high-frequency presence.
 * @param buffer Interleaved buffer to process (in-place).
 * @param numFrames Number of frames in the buffer.
 */
/**
 * Apply subtle drift modulation to the buffer to simulate analog timing variations.
 * @param buffer Interleaved buffer to process (in-place).
 * @param numFrames Number of frames in the buffer.
 */
    double m_latencyCompensationMs{0.0};  // Total input + output latency for recording
    
    // Audio quality settings
    AudioQualitySettings m_qualitySettings;
    /**
 * Fill the provided buffer with silence (zeros).
 * @param buffer Pointer to the interleaved output buffer to clear.
 * @param numFrames Number of audio frames to generate (each frame contains all channels).
 */

/**
 * Copy internal audio data into the provided output buffer, performing sample-rate conversion if needed.
 * @param outputBuffer Pointer to the destination interleaved buffer.
 * @param numFrames Number of audio frames to produce.
 * @param outputSampleRate Target sample rate for the output buffer.
 */

/**
 * Trim internal streaming buffers based on the current frame position to free consumed data.
 * @param currentFrame Absolute frame index indicating the current playback/stream position.
 */

/**
 * Stop any active streaming operation and release associated resources.
 */

/**
 * Initialize WAV streaming from a file and prepare internal state for background streaming.
 * @param filePath Path to the WAV file to stream.
 * @param sampleRate Sample rate of the WAV file.
 * @param channels Number of channels in the WAV file.
 * @param bitsPerSample Bits per sample in the WAV file (e.g., 16, 24, 32).
 * @param dataOffset Byte offset in the file where sample data begins.
 * @param dataSize Total size in bytes of the sample data.
 * @param startFrame Optional starting frame within the file to begin streaming from (default 0).
 * @returns `true` if streaming was successfully started and internal state initialized, `false` otherwise.
 */

/**
 * Background thread entry used to stream WAV data into internal buffers.
 * @param channels Number of channels to stream (used to deinterleave/interleave as needed).
 */

/**
 * Linear interpolation of sample data at a fractional position for a specific channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples per channel in `data`.
 * @param position Fractional sample position (0..totalSamples-1) to read from.
 * @param channel Channel index to read (0-based).
 * @returns Interpolated sample value at the requested position and channel.
 */

/**
 * Cubic interpolation of sample data at a fractional position for a specific channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples per channel in `data`.
 * @param position Fractional sample position (0..totalSamples-1) to read from.
 * @param channel Channel index to read (0-based).
 * @returns Interpolated sample value at the requested position and channel.
 */

/**
 * Sinc-based interpolation of sample data at a fractional position for a specific channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples per channel in `data`.
 * @param position Fractional sample position (0..totalSamples-1) to read from.
 * @param channel Channel index to read (0-based).
 * @returns Interpolated sample value at the requested position and channel.
 */

/**
 * High-quality "Ultra" interpolation of sample data at a fractional position for a specific channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples per channel in `data`.
 * @param position Fractional sample position (0..totalSamples-1) to read from.
 * @param channel Channel index to read (0-based).
 * @returns Interpolated sample value at the requested position and channel.
 */

/**
 * Extreme-quality interpolation of sample data at a fractional position for a specific channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples per channel in `data`.
 * @param position Fractional sample position (0..totalSamples-1) to read from.
 * @param channel Channel index to read (0-based).
 * @returns Interpolated sample value at the requested position and channel.
 */

/**
 * "Perfect" interpolation of sample data at a fractional position for a specific channel.
 * @param data Pointer to interleaved sample data.
 * @param totalSamples Total number of samples per channel in `data`.
 * @param position Fractional sample position (0..totalSamples-1) to read from.
 * @param channel Channel index to read (0-based).
 * @returns Interpolated sample value at the requested position and channel.
 */

/**
 * Apply the configured dithering scheme to the provided buffer.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 */

/**
 * Apply triangular dithering to the buffer.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 */

/**
 * Apply high-pass dithering to the buffer.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 */

/**
 * Apply noise-shaped dithering using internal dither history to the buffer.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 */

/**
 * Remove accumulated DC offset from the buffer and update internal DC accumulator.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 */

/**
 * Apply soft-clipping to prevent harsh digital clipping and shape peaks.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 */

/**
 * Apply stereo width adjustment to the buffer.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numFrames Number of audio frames in `buffer`.
 * @param widthPercent Stereo width percentage where 100.0 preserves original width.
 */

/**
 * Apply the Euphoria (Nomad) processing chain to the buffer.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numFrames Number of audio frames in `buffer`.
 */

/**
 * Apply tape-circuit-style processing to the buffer for warm saturation and bloom.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numSamples Number of samples in `buffer` (not frames).
 * @param bloomAmount Amount of harmonic bloom to apply (0.0..1.0).
 * @param smoothing Transient smoothing amount (0.0..1.0).
 */

/**
 * Apply "air" enhancement to the buffer to emphasize high-frequency content.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numFrames Number of audio frames in `buffer`.
 */

/**
 * Apply subtle drift modulation to the buffer to emulate analog instability.
 * @param buffer Interleaved audio buffer to process in-place.
 * @param numFrames Number of audio frames in `buffer`.
 */
double m_dcOffset{0.0};  // Accumulated DC offset for removal
    
    // Dithering state (for noise shaping)
    float m_ditherHistory[2]{0.0f, 0.0f};  // Per-channel dither history for noise shaping

    // Temporary buffer reused per process call to avoid allocations
    mutable std::vector<float> m_tempBuffer;

    // Internal audio processing
    void generateSilence(float* buffer, uint32_t numFrames);
    void copyAudioData(float* outputBuffer, uint32_t numFrames, double outputSampleRate);
    void trimStreamBuffer(uint64_t currentFrame);
    void stopStreaming();
    bool startWavStreaming(const std::string& filePath, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, uint32_t dataOffset, uint64_t dataSize, uint64_t startFrame = 0);
    void streamWavThread(uint32_t channels);
    
    // Interpolation methods
    float interpolateLinear(const float* data, uint32_t totalSamples, double position, uint32_t channel) const;
    float interpolateCubic(const float* data, uint32_t totalSamples, double position, uint32_t channel) const;
    float interpolateSinc(const float* data, uint32_t totalSamples, double position, uint32_t channel) const;
    float interpolateUltra(const float* data, uint32_t totalSamples, double position, uint32_t channel) const;
    float interpolateExtreme(const float* data, uint32_t totalSamples, double position, uint32_t channel) const;
    float interpolatePerfect(const float* data, uint32_t totalSamples, double position, uint32_t channel) const;
    
    // Audio quality processing
    void applyDithering(float* buffer, uint32_t numSamples);
    void applyTriangularDither(float* buffer, uint32_t numSamples);
    void applyHighPassDither(float* buffer, uint32_t numSamples);
    void applyNoiseShapedDither(float* buffer, uint32_t numSamples);
    void removeDC(float* buffer, uint32_t numSamples);
    void applySoftClipping(float* buffer, uint32_t numSamples);
    void applyStereoWidth(float* buffer, uint32_t numFrames, float widthPercent);
    
    // Euphoria Engine (Nomad Mode signature processing)
    void applyEuphoriaEngine(float* buffer, uint32_t numFrames);
    void applyTapeCircuit(float* buffer, uint32_t numSamples, float bloomAmount, float smoothing);
    void applyAir(float* buffer, uint32_t numFrames);
    void applyDrift(float* buffer, uint32_t numFrames);
};

} // namespace Audio
} // namespace Nomad