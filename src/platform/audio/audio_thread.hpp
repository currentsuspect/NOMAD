/**
 * @file audio_thread.hpp
 * @brief Real-time audio thread management for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides the audio thread abstraction that handles real-time
 * audio processing. It enforces strict real-time safety constraints.
 * 
 * @rt-safety The audio callback must be strictly real-time safe:
 *            - No memory allocation
 *            - No blocking operations
 *            - No system calls that may block
 *            - No mutex locks (use lock-free structures)
 *            - No I/O operations
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../../core/base/assert.hpp"
#include "../../core/threading/lock_free_queue.hpp"

#include <atomic>
#include <functional>
#include <thread>
#include <chrono>

namespace nomad::platform {

/**
 * @brief Audio processing statistics
 * 
 * Updated from a non-real-time thread to avoid impacting audio performance.
 */
struct AudioStats {
    std::atomic<f32> cpuLoad{0.0f};         ///< CPU load as fraction (0.0 - 1.0+)
    std::atomic<u64> callbackCount{0};       ///< Total callbacks processed
    std::atomic<u64> xrunCount{0};           ///< Buffer underrun/overrun count
    std::atomic<u64> lastProcessTimeNs{0};   ///< Last callback duration in nanoseconds
    std::atomic<f64> bufferDurationNs{0.0};  ///< Expected buffer duration in nanoseconds
};

/**
 * @brief CPU load measurement mode
 */
enum class CpuLoadMode : u8 {
    Disabled,       ///< No CPU measurement (safest for RT)
    NonRealTime,    ///< Measure from non-RT thread via timestamps
    RealTime,       ///< Measure in RT callback (use with caution)
};

/**
 * @brief Configuration for audio thread
 */
struct AudioThreadConfig {
    u32 sampleRate = 44100;
    u32 bufferSize = 256;
    u32 numInputChannels = 2;
    u32 numOutputChannels = 2;
    CpuLoadMode cpuLoadMode = CpuLoadMode::NonRealTime;  ///< Default to safe mode
    bool enableXrunDetection = true;
};

/**
 * @brief Real-time audio processing callback
 * 
 * @param inputBuffer Pointer to interleaved input samples (may be nullptr)
 * @param outputBuffer Pointer to interleaved output samples
 * @param numFrames Number of frames to process
 * @param userData User-provided context
 * 
 * @rt-safety This callback MUST be real-time safe. Avoid:
 *            - Memory allocation (new, malloc, std::vector resize)
 *            - Blocking (mutex, condition variable, sleep)
 *            - System calls (file I/O, console output)
 *            - Exceptions
 */
using AudioCallback = void (*)(
    const float* inputBuffer,
    float* outputBuffer,
    u32 numFrames,
    void* userData
);

/**
 * @brief Timestamp message for non-RT CPU load calculation
 */
struct TimestampMessage {
    u64 callbackId;
    u64 startTimeNs;
    u64 endTimeNs;
};

/**
 * @brief Audio thread manager
 * 
 * Manages the real-time audio thread and provides safe communication
 * between the audio thread and other application threads.
 * 
 * @security CPU load measurement is moved off the audio thread by default
 *           to prevent priority inversion and unbounded latency. The audio
 *           callback only stores lightweight timestamps in a lock-free queue,
 *           and a separate non-RT thread computes the actual CPU load.
 */
class AudioThread {
public:
    AudioThread() = default;
    ~AudioThread() { stop(); }
    
    // Non-copyable, non-movable
    AudioThread(const AudioThread&) = delete;
    AudioThread& operator=(const AudioThread&) = delete;
    AudioThread(AudioThread&&) = delete;
    AudioThread& operator=(AudioThread&&) = delete;
    
    /**
     * @brief Initialize audio thread with configuration
     * @param config Audio thread configuration
     * @return true if initialization successful
     */
    bool initialize(const AudioThreadConfig& config) {
        m_config = config;
        m_stats.bufferDurationNs.store(
            (static_cast<f64>(config.bufferSize) / config.sampleRate) * 1e9,
            std::memory_order_relaxed
        );
        m_initialized = true;
        return true;
    }
    
    /**
     * @brief Set the audio processing callback
     * @param callback Function to call for audio processing
     * @param userData User context passed to callback
     */
    void setCallback(AudioCallback callback, void* userData) {
        m_callback = callback;
        m_userData = userData;
    }
    
    /**
     * @brief Start audio processing
     * @return true if started successfully
     */
    bool start() {
        if (!m_initialized || m_running.load(std::memory_order_acquire)) {
            return false;
        }
        
        m_running.store(true, std::memory_order_release);
        
        // Start the statistics processing thread if CPU load measurement is enabled
        if (m_config.cpuLoadMode == CpuLoadMode::NonRealTime) {
            m_statsThreadRunning.store(true, std::memory_order_release);
            m_statsThread = std::thread(&AudioThread::statsThreadFunc, this);
        }
        
        // Note: Actual audio device start would happen here
        // This is a skeleton - platform-specific implementation needed
        
        return true;
    }
    
    /**
     * @brief Stop audio processing
     */
    void stop() {
        m_running.store(false, std::memory_order_release);
        
        // Stop stats thread
        m_statsThreadRunning.store(false, std::memory_order_release);
        if (m_statsThread.joinable()) {
            m_statsThread.join();
        }
    }
    
    /**
     * @brief Check if audio is running
     */
    [[nodiscard]] bool isRunning() const noexcept {
        return m_running.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get current audio statistics
     */
    [[nodiscard]] const AudioStats& stats() const noexcept {
        return m_stats;
    }
    
    /**
     * @brief Get current configuration
     */
    [[nodiscard]] const AudioThreadConfig& config() const noexcept {
        return m_config;
    }
    
    /**
     * @brief Process audio (called from audio device callback)
     * 
     * @rt-safety This method is called from the real-time audio thread.
     *            All operations must be RT-safe.
     * 
     * @param inputBuffer Input audio samples
     * @param outputBuffer Output audio samples
     * @param numFrames Number of frames to process
     */
    void process(const float* inputBuffer, float* outputBuffer, u32 numFrames) {
        const u64 callbackId = m_stats.callbackCount.fetch_add(1, std::memory_order_relaxed);
        
        // RT-safe timestamp capture (only if measuring CPU load)
        u64 startTimeNs = 0;
        if (m_config.cpuLoadMode != CpuLoadMode::Disabled) {
            // Note: On most platforms, reading the TSC or clock is RT-safe
            // However, we minimize work here by just capturing the value
            startTimeNs = getCurrentTimeNs();
        }
        
        // Call user callback
        if (m_callback) {
            m_callback(inputBuffer, outputBuffer, numFrames, m_userData);
        } else {
            // Silence output if no callback set
            const usize sampleCount = static_cast<usize>(numFrames) * m_config.numOutputChannels;
            for (usize i = 0; i < sampleCount; ++i) {
                outputBuffer[i] = 0.0f;
            }
        }
        
        // RT-safe timestamp capture and stats update
        if (m_config.cpuLoadMode == CpuLoadMode::NonRealTime) {
            // Queue timestamp for non-RT processing (lock-free push)
            const u64 endTimeNs = getCurrentTimeNs();
            TimestampMessage msg{callbackId, startTimeNs, endTimeNs};
            m_timestampQueue.tryPush(msg);  // Dropping is acceptable
        } else if (m_config.cpuLoadMode == CpuLoadMode::RealTime) {
            // Direct update in RT context (use with caution)
            // This only uses atomic stores, which are typically RT-safe
            const u64 endTimeNs = getCurrentTimeNs();
            const u64 durationNs = endTimeNs - startTimeNs;
            m_stats.lastProcessTimeNs.store(durationNs, std::memory_order_relaxed);
            
            const f64 bufferDurationNs = m_stats.bufferDurationNs.load(std::memory_order_relaxed);
            if (bufferDurationNs > 0) {
                const f32 load = static_cast<f32>(durationNs / bufferDurationNs);
                m_stats.cpuLoad.store(load, std::memory_order_relaxed);
            }
        }
    }
    
    /**
     * @brief Report an xrun (buffer underrun/overrun)
     * 
     * @rt-safety Safe to call from RT context (atomic increment)
     */
    void reportXrun() noexcept {
        m_stats.xrunCount.fetch_add(1, std::memory_order_relaxed);
    }

private:
    /**
     * @brief Get current time in nanoseconds
     * 
     * Uses the most efficient available time source. On most platforms,
     * this compiles to a single instruction (RDTSC on x86, CNTVCT on ARM).
     * 
     * @rt-safety Generally RT-safe, but may vary by platform.
     *            The NonRealTime mode moves heavy processing off the audio thread.
     */
    [[nodiscard]] static u64 getCurrentTimeNs() noexcept {
        // Use steady_clock which typically maps to hardware counters
        // This is generally RT-safe as it doesn't make system calls on most platforms
        auto now = std::chrono::steady_clock::now();
        return static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()
            ).count()
        );
    }
    
    /**
     * @brief Statistics processing thread (non-RT)
     * 
     * Processes timestamps from the audio thread and computes CPU load
     * without impacting real-time performance.
     */
    void statsThreadFunc() {
        while (m_statsThreadRunning.load(std::memory_order_acquire)) {
            // Process all queued timestamps
            TimestampMessage msg;
            while (m_timestampQueue.tryPop(msg)) {
                const u64 durationNs = msg.endTimeNs - msg.startTimeNs;
                m_stats.lastProcessTimeNs.store(durationNs, std::memory_order_relaxed);
                
                const f64 bufferDurationNs = m_stats.bufferDurationNs.load(std::memory_order_relaxed);
                if (bufferDurationNs > 0) {
                    const f32 load = static_cast<f32>(durationNs / bufferDurationNs);
                    
                    // Exponential moving average for smooth CPU load display
                    const f32 currentLoad = m_stats.cpuLoad.load(std::memory_order_relaxed);
                    const f32 smoothedLoad = currentLoad * 0.9f + load * 0.1f;
                    m_stats.cpuLoad.store(smoothedLoad, std::memory_order_relaxed);
                }
            }
            
            // Sleep to avoid spinning (this is a non-RT thread)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    AudioThreadConfig m_config{};
    AudioCallback m_callback = nullptr;
    void* m_userData = nullptr;
    
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    
    // Statistics
    AudioStats m_stats;
    
    // Non-RT stats processing
    core::SPSCQueue<TimestampMessage, 256> m_timestampQueue;
    std::thread m_statsThread;
    std::atomic<bool> m_statsThreadRunning{false};
};

} // namespace nomad::platform
