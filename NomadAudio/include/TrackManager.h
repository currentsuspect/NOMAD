// Â© 2025 Nomad Studios â€” All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "Track.h"
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace Nomad {
namespace Audio {

/**
 * @brief Thread pool for parallel audio processing
 * 
 * Distributes track processing across multiple CPU cores for reduced latency
 * Uses lock-free design and thread affinity for real-time performance
 */
class AudioThreadPool {
public:
    AudioThreadPool(size_t numThreads);
    ~AudioThreadPool();
    
    // Submit a task to the thread pool
    void enqueue(std::function<void()> task);
    
    // Wait for all tasks to complete
    void waitForCompletion();
    
    // Get number of worker threads
    size_t getThreadCount() const { return m_workers.size(); }
    
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::condition_variable m_completionCondition;
    
    std::atomic<bool> m_stop{false};
    std::atomic<size_t> m_activeTasks{0};
    
    void workerThread();
};

/**
 * @brief Manages multiple audio tracks for the DAW
 *
 * Coordinates playback, recording, and mixing of multiple tracks.
 * Provides high-level DAW functionality like transport control,
 * track management, and audio routing.
 * 
 * MULTI-THREADING:
 * - Processes tracks in parallel using thread pool
 * - Distributes CPU load across all cores
 * - Lock-free audio buffer mixing
 * - Real-time thread priorities
 */
class TrackManager {
public:
    TrackManager();
    ~TrackManager();

    // Track Management
    std::shared_ptr<Track> addTrack(const std::string& name = "");
    std::shared_ptr<Track> getTrack(size_t index);
    std::shared_ptr<const Track> getTrack(size_t index) const;
    size_t getTrackCount() const { return m_tracks.size(); }

    void removeTrack(size_t index);
    void clearAllTracks();

    // Transport Control
    void play();
    void pause();
    void stop();
    void record();

    bool isPlaying() const { return m_isPlaying.load(); }
    bool isRecording() const { return m_isRecording.load(); }

    // Position Control
    void setPosition(double seconds);
    double getPosition() const { return m_positionSeconds.load(); }

    double getTotalDuration() const;
    
    // Position update callback (called during playback to update transport UI)
    void setOnPositionUpdate(std::function<void(double)> callback) { m_onPositionUpdate = callback; }

    // Audio output callback (called after processing to update VU meters, visualizers, etc.)
    // Parameters: leftChannel, rightChannel, numSamples, sampleRate
    void setOnAudioOutput(std::function<void(const float*, const float*, size_t, double)> callback) { 
        m_onAudioOutput = callback; 
    }

    // Audio Processing
    void processAudio(float* outputBuffer, uint32_t numFrames, double streamTime);
    
    // Sample rate management (called when audio device sample rate changes)
    void setOutputSampleRate(uint32_t sampleRate);
    
    // Multi-threading control
    void setMultiThreadingEnabled(bool enabled) { m_multiThreadingEnabled = enabled; }
    bool isMultiThreadingEnabled() const { return m_multiThreadingEnabled; }
    
    void setThreadCount(size_t count);
    size_t getThreadCount() const { return m_threadPool ? m_threadPool->getThreadCount() : 1; }
    
    // Performance monitoring
    double getAudioLoadPercent() const { return m_audioLoadPercent.load(); }

    // Mixer Integration
    void updateMixer();

    // Solo/Mute Management
    void clearAllSolos();

    // Transport command queued to be applied at audio-block boundary to avoid clicks
    enum class PendingTransportCommand : uint8_t { None = 0, Play = 1, Pause = 2, Stop = 3 };

private:
    // Track collection
    std::vector<std::shared_ptr<Track>> m_tracks;

    // Transport state
    std::atomic<bool> m_isPlaying{false};
    std::atomic<bool> m_isRecording{false};
    std::atomic<double> m_positionSeconds{0.0};
    
    // Audio device sample rate
    std::atomic<uint32_t> m_outputSampleRate{48000};

    // Track ID counter
    std::atomic<uint32_t> m_nextTrackId{1};
    
    // Callbacks
    std::function<void(double)> m_onPositionUpdate;
    std::function<void(const float*, const float*, size_t, double)> m_onAudioOutput;
    
    // Multi-threading
    std::unique_ptr<AudioThreadPool> m_threadPool;
    std::atomic<bool> m_multiThreadingEnabled{true};  // Enabled by default
    
    // Performance tracking
    std::atomic<double> m_audioLoadPercent{0.0};
    
    // Per-track temporary buffers for parallel processing
    std::vector<std::vector<float>> m_trackBuffers;

    // Track creation helper
    std::string generateTrackName() const;
    
    // Processing helpers
    void processAudioSingleThreaded(float* outputBuffer, uint32_t numFrames, double streamTime);
    void processAudioMultiThreaded(float* outputBuffer, uint32_t numFrames, double streamTime);

    std::atomic<PendingTransportCommand> m_pendingTransportCommand{PendingTransportCommand::None};

    // Master fade (applied on audio thread). Values are expressed in samples.
    // Fade duration is configurable via constant in CPP. These are manipulated
    // by the audio thread when a pending transport command is applied.
    std::atomic<int> m_masterFadeSamplesRemaining{0};
    std::atomic<float> m_masterFadeCurrentGain{1.0f};
    float m_masterFadeTargetGain{1.0f};

    // Helper to set a pending transport command from the main thread
    void queueTransportCommand(PendingTransportCommand cmd);
    // Apply any queued transport command on the audio thread (sampleRate required for fade length)
    void applyQueuedTransportCommandIfNeeded(uint32_t sampleRate);

    static PendingTransportCommand s_lastAppliedCommand;
};

} // namespace Audio
} // namespace Nomad
