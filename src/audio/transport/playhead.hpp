/**
 * @file playhead.hpp
 * @brief Transport playhead with sub-sample accuracy
 * @author Nomad Team
 * @date 2025
 * 
 * Manages playback position, tempo, time signature, and synchronization.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"

#include <atomic>
#include <cmath>

namespace nomad::audio {

/**
 * @brief Time signature representation
 */
struct TimeSignature {
    u8 numerator = 4;    ///< Beats per bar
    u8 denominator = 4;  ///< Beat unit (4 = quarter note)
    
    TimeSignature() = default;
    TimeSignature(u8 num, u8 denom) : numerator(num), denominator(denom) {}
    
    [[nodiscard]] f64 beatsPerBar() const noexcept {
        return static_cast<f64>(numerator);
    }
    
    [[nodiscard]] f64 beatUnit() const noexcept {
        return static_cast<f64>(denominator);
    }
};

/**
 * @brief Musical position (bars, beats, ticks)
 */
struct MusicalPosition {
    i32 bar = 1;         ///< Bar number (1-based)
    i32 beat = 1;        ///< Beat within bar (1-based)
    i32 tick = 0;        ///< Tick within beat (0-959, 960 PPQ)
    
    static constexpr i32 TICKS_PER_BEAT = 960;  ///< Pulses per quarter note
    
    MusicalPosition() = default;
    MusicalPosition(i32 b, i32 bt, i32 t) : bar(b), beat(bt), tick(t) {}
    
    /**
     * @brief Convert to total ticks from start
     */
    [[nodiscard]] i64 toTicks(const TimeSignature& timeSig) const noexcept {
        i64 totalBeats = static_cast<i64>(bar - 1) * timeSig.numerator + (beat - 1);
        return totalBeats * TICKS_PER_BEAT + tick;
    }
    
    /**
     * @brief Create from total ticks
     */
    [[nodiscard]] static MusicalPosition fromTicks(i64 totalTicks, const TimeSignature& timeSig) noexcept {
        MusicalPosition pos;
        i64 totalBeats = totalTicks / TICKS_PER_BEAT;
        pos.tick = static_cast<i32>(totalTicks % TICKS_PER_BEAT);
        pos.bar = static_cast<i32>(totalBeats / timeSig.numerator) + 1;
        pos.beat = static_cast<i32>(totalBeats % timeSig.numerator) + 1;
        return pos;
    }
};

/**
 * @brief Loop region
 */
struct LoopRegion {
    i64 startSample = 0;
    i64 endSample = 0;
    bool enabled = false;
    
    [[nodiscard]] bool isValid() const noexcept {
        return endSample > startSample;
    }
    
    [[nodiscard]] i64 length() const noexcept {
        return endSample - startSample;
    }
    
    [[nodiscard]] bool contains(i64 sample) const noexcept {
        return sample >= startSample && sample < endSample;
    }
};

/**
 * @brief Transport state
 */
enum class TransportState : u8 {
    Stopped,
    Playing,
    Recording,
    Paused
};

/**
 * @brief Playhead - manages transport position and timing
 * 
 * Provides sample-accurate and musical position tracking with
 * tempo, time signature, and loop support.
 * 
 * @rt-safety Position updates are real-time safe. Tempo/time signature
 *            changes may require smoothing.
 */
class Playhead {
public:
    Playhead() = default;
    
    //=========================================================================
    // Position
    //=========================================================================
    
    /**
     * @brief Get current position in samples
     */
    [[nodiscard]] i64 positionSamples() const noexcept {
        return m_positionSamples.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get current position in seconds
     */
    [[nodiscard]] f64 positionSeconds() const noexcept {
        return static_cast<f64>(positionSamples()) / m_sampleRate;
    }
    
    /**
     * @brief Get current position in beats
     */
    [[nodiscard]] f64 positionBeats() const noexcept {
        return secondsToBeats(positionSeconds());
    }
    
    /**
     * @brief Get current musical position
     */
    [[nodiscard]] MusicalPosition musicalPosition() const noexcept {
        f64 beats = positionBeats();
        i64 ticks = static_cast<i64>(beats * MusicalPosition::TICKS_PER_BEAT);
        return MusicalPosition::fromTicks(ticks, m_timeSignature);
    }
    
    /**
     * @brief Set position in samples
     */
    void setPositionSamples(i64 samples) noexcept {
        m_positionSamples.store(samples, std::memory_order_release);
    }
    
    /**
     * @brief Set position in seconds
     */
    void setPositionSeconds(f64 seconds) noexcept {
        setPositionSamples(static_cast<i64>(seconds * m_sampleRate));
    }
    
    /**
     * @brief Set position in beats
     */
    void setPositionBeats(f64 beats) noexcept {
        setPositionSeconds(beatsToSeconds(beats));
    }
    
    //=========================================================================
    // Transport Control
    //=========================================================================
    
    /**
     * @brief Advance position by buffer size
     * @param bufferSize Number of samples to advance
     * @return true if loop wrapped
     * 
     * @rt-safety Real-time safe
     */
    bool advance(u32 bufferSize) noexcept {
        i64 pos = m_positionSamples.load(std::memory_order_relaxed);
        pos += bufferSize;
        
        // Handle looping
        bool looped = false;
        if (m_loop.enabled && m_loop.isValid()) {
            while (pos >= m_loop.endSample) {
                pos = m_loop.startSample + (pos - m_loop.endSample);
                looped = true;
            }
        }
        
        m_positionSamples.store(pos, std::memory_order_release);
        return looped;
    }
    
    /**
     * @brief Get transport state
     */
    [[nodiscard]] TransportState state() const noexcept {
        return m_state.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Set transport state
     */
    void setState(TransportState state) noexcept {
        m_state.store(state, std::memory_order_release);
    }
    
    /**
     * @brief Check if playing
     */
    [[nodiscard]] bool isPlaying() const noexcept {
        TransportState s = state();
        return s == TransportState::Playing || s == TransportState::Recording;
    }
    
    /**
     * @brief Check if recording
     */
    [[nodiscard]] bool isRecording() const noexcept {
        return state() == TransportState::Recording;
    }
    
    /**
     * @brief Start playback
     */
    void play() noexcept {
        setState(TransportState::Playing);
    }
    
    /**
     * @brief Stop playback
     */
    void stop() noexcept {
        setState(TransportState::Stopped);
    }
    
    /**
     * @brief Pause playback
     */
    void pause() noexcept {
        setState(TransportState::Paused);
    }
    
    /**
     * @brief Start recording
     */
    void record() noexcept {
        setState(TransportState::Recording);
    }
    
    /**
     * @brief Return to start
     */
    void returnToZero() noexcept {
        setPositionSamples(0);
    }
    
    //=========================================================================
    // Tempo & Time Signature
    //=========================================================================
    
    /**
     * @brief Get tempo in BPM
     */
    [[nodiscard]] f64 tempo() const noexcept {
        return m_tempo;
    }
    
    /**
     * @brief Set tempo in BPM
     */
    void setTempo(f64 bpm) noexcept {
        m_tempo = std::clamp(bpm, 20.0, 999.0);
    }
    
    /**
     * @brief Get time signature
     */
    [[nodiscard]] const TimeSignature& timeSignature() const noexcept {
        return m_timeSignature;
    }
    
    /**
     * @brief Set time signature
     */
    void setTimeSignature(TimeSignature timeSig) noexcept {
        m_timeSignature = timeSig;
    }
    
    /**
     * @brief Convert seconds to beats at current tempo
     */
    [[nodiscard]] f64 secondsToBeats(f64 seconds) const noexcept {
        return seconds * (m_tempo / 60.0);
    }
    
    /**
     * @brief Convert beats to seconds at current tempo
     */
    [[nodiscard]] f64 beatsToSeconds(f64 beats) const noexcept {
        return beats * (60.0 / m_tempo);
    }
    
    /**
     * @brief Convert samples to beats
     */
    [[nodiscard]] f64 samplesToBeats(i64 samples) const noexcept {
        return secondsToBeats(static_cast<f64>(samples) / m_sampleRate);
    }
    
    /**
     * @brief Convert beats to samples
     */
    [[nodiscard]] i64 beatsToSamples(f64 beats) const noexcept {
        return static_cast<i64>(beatsToSeconds(beats) * m_sampleRate);
    }
    
    //=========================================================================
    // Looping
    //=========================================================================
    
    /**
     * @brief Get loop region
     */
    [[nodiscard]] const LoopRegion& loop() const noexcept {
        return m_loop;
    }
    
    /**
     * @brief Set loop region
     */
    void setLoop(const LoopRegion& loop) noexcept {
        m_loop = loop;
    }
    
    /**
     * @brief Enable/disable looping
     */
    void setLoopEnabled(bool enabled) noexcept {
        m_loop.enabled = enabled;
    }
    
    /**
     * @brief Check if looping is enabled
     */
    [[nodiscard]] bool isLooping() const noexcept {
        return m_loop.enabled && m_loop.isValid();
    }
    
    //=========================================================================
    // Configuration
    //=========================================================================
    
    /**
     * @brief Set sample rate
     */
    void setSampleRate(u32 sampleRate) noexcept {
        m_sampleRate = sampleRate;
    }
    
    /**
     * @brief Get sample rate
     */
    [[nodiscard]] u32 sampleRate() const noexcept {
        return m_sampleRate;
    }

private:
    // Position (atomic for RT-safe access)
    std::atomic<i64> m_positionSamples{0};
    std::atomic<TransportState> m_state{TransportState::Stopped};
    
    // Tempo & timing
    f64 m_tempo = 120.0;
    TimeSignature m_timeSignature;
    u32 m_sampleRate = 44100;
    
    // Looping
    LoopRegion m_loop;
};

} // namespace nomad::audio
