/**
 * @file oscillator.hpp
 * @brief Oscillator implementations for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides alias-free oscillator implementations:
 * - Basic waveforms (sine, saw, square, triangle)
 * - PolyBLEP anti-aliasing
 * - Wavetable oscillator
 */

#pragma once

#include "processor.hpp"
#include <cmath>
#include <array>
#include <vector>

namespace nomad::dsp {

//=============================================================================
// Phase Accumulator
//=============================================================================

/**
 * @brief Phase accumulator with modulation support
 */
class PhaseAccumulator {
public:
    /**
     * @brief Set frequency
     * @param frequency Frequency in Hz
     * @param sampleRate Sample rate in Hz
     */
    void setFrequency(f64 frequency, f64 sampleRate) noexcept {
        m_increment = frequency / sampleRate;
    }
    
    /**
     * @brief Advance phase and return current value
     * @return Phase in range [0, 1)
     */
    [[nodiscard]] f64 next() noexcept {
        const f64 phase = m_phase;
        m_phase += m_increment;
        if (m_phase >= 1.0) m_phase -= 1.0;
        return phase;
    }
    
    /**
     * @brief Get current phase without advancing
     */
    [[nodiscard]] f64 phase() const noexcept { return m_phase; }
    
    /**
     * @brief Get phase increment (normalized frequency)
     */
    [[nodiscard]] f64 increment() const noexcept { return m_increment; }
    
    /**
     * @brief Reset phase
     */
    void reset(f64 phase = 0.0) noexcept { m_phase = phase; }
    
    /**
     * @brief Set phase directly
     */
    void setPhase(f64 phase) noexcept { m_phase = phase - std::floor(phase); }
    
    /**
     * @brief Add phase modulation
     */
    void modulate(f64 amount) noexcept {
        m_phase += amount;
        while (m_phase < 0.0) m_phase += 1.0;
        while (m_phase >= 1.0) m_phase -= 1.0;
    }

private:
    f64 m_phase = 0.0;
    f64 m_increment = 0.0;
};

//=============================================================================
// PolyBLEP Anti-Aliasing
//=============================================================================

/**
 * @brief PolyBLEP (Polynomial Band-Limited Step) function
 * 
 * Used to smooth discontinuities in waveforms to reduce aliasing.
 */
[[nodiscard]] inline f64 polyBlep(f64 t, f64 dt) noexcept {
    // t = phase position of discontinuity, dt = phase increment
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    } else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

//=============================================================================
// Basic Waveform Oscillator
//=============================================================================

/**
 * @brief Waveform type
 */
enum class Waveform : u8 {
    Sine,
    Saw,
    Square,
    Triangle,
    Pulse
};

/**
 * @brief Basic oscillator with PolyBLEP anti-aliasing
 * 
 * Generates standard waveforms with minimal aliasing using
 * polynomial band-limited step functions.
 */
class Oscillator : public IProcessor {
public:
    enum Params { pFrequency, pWaveform, pPulseWidth, pPhase, kNumParams };
    
    static constexpr f64 TWO_PI = 6.283185307179586476925286766559;
    
    Oscillator() {
        m_frequency.setImmediate(440.0f);
        m_pulseWidth.setImmediate(0.5f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        m_frequency.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        m_pulseWidth.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        m_phase.setFrequency(m_frequency.target(), sampleRate);
        (void)maxBlockSize;
    }
    
    void reset() override {
        m_phase.reset();
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = buffer.numChannels();
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            // Update frequency if smoothing
            if (m_frequency.isSmoothing()) {
                m_phase.setFrequency(m_frequency.next(), m_sampleRate);
            }
            
            const f64 pw = m_pulseWidth.next();
            
            // Generate sample
            const f64 phase = m_phase.next();
            const f64 dt = m_phase.increment();
            const f64 sample = generateSample(phase, dt, pw);
            
            // Write to all channels (mono oscillator, duplicated)
            for (u32 ch = 0; ch < numChannels; ++ch) {
                buffer.channel(ch)[frame] = static_cast<f32>(sample);
            }
        }
    }
    
    [[nodiscard]] u32 getParameterCount() const override { return kNumParams; }
    
    [[nodiscard]] ParameterInfo getParameterInfo(u32 index) const override {
        switch (index) {
            case pFrequency: return {"frequency", "Frequency", 20.0f, 20000.0f, 440.0f, 0.0f, "Hz", true};
            case pWaveform: return {"waveform", "Waveform", 0.0f, 4.0f, 1.0f, 1.0f, "", false};
            case pPulseWidth: return {"pulseWidth", "Pulse Width", 0.01f, 0.99f, 0.5f, 0.01f, "", false};
            case pPhase: return {"phase", "Phase", 0.0f, 1.0f, 0.0f, 0.01f, "", false};
            default: return {};
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Oscillator"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Generator"; }
    [[nodiscard]] bool hasState() const override { return true; }
    
    // Direct setters
    void setFrequency(f32 hz) { m_frequency.setTarget(hz); }
    void setWaveform(Waveform wf) { m_waveform = wf; }
    void setPulseWidth(f32 pw) { m_pulseWidth.setTarget(pw); }
    
    /**
     * @brief Generate single sample at given phase
     */
    [[nodiscard]] f64 generateSample(f64 phase, f64 dt, f64 pw) const noexcept {
        switch (m_waveform) {
            case Waveform::Sine:
                return std::sin(phase * TWO_PI);
                
            case Waveform::Saw: {
                f64 saw = 2.0 * phase - 1.0;
                saw -= polyBlep(phase, dt);
                return saw;
            }
                
            case Waveform::Square: {
                f64 square = phase < 0.5 ? 1.0 : -1.0;
                square += polyBlep(phase, dt);
                square -= polyBlep(std::fmod(phase + 0.5, 1.0), dt);
                return square;
            }
                
            case Waveform::Triangle: {
                // Integrate square wave for triangle
                f64 square = phase < 0.5 ? 1.0 : -1.0;
                square += polyBlep(phase, dt);
                square -= polyBlep(std::fmod(phase + 0.5, 1.0), dt);
                // Leaky integrator
                m_triState = 0.99 * m_triState + dt * 4.0 * square;
                return m_triState;
            }
                
            case Waveform::Pulse: {
                f64 pulse = phase < pw ? 1.0 : -1.0;
                pulse += polyBlep(phase, dt);
                pulse -= polyBlep(std::fmod(phase + (1.0 - pw), 1.0), dt);
                return pulse;
            }
                
            default:
                return 0.0;
        }
    }

private:
    f64 m_sampleRate = 44100.0;
    Waveform m_waveform = Waveform::Saw;
    
    PhaseAccumulator m_phase;
    SmoothedParameter m_frequency;
    SmoothedParameter m_pulseWidth;
    
    mutable f64 m_triState = 0.0;  // For triangle wave integration
};

//=============================================================================
// Wavetable Oscillator
//=============================================================================

/**
 * @brief Single wavetable with multiple mipmap levels
 */
class Wavetable {
public:
    static constexpr u32 TABLE_SIZE = 2048;
    static constexpr u32 NUM_MIPMAPS = 10;  // Covers 20Hz to 20kHz
    
    Wavetable() {
        m_tables.resize(NUM_MIPMAPS);
        for (auto& table : m_tables) {
            table.resize(TABLE_SIZE + 1, 0.0f);  // +1 for interpolation wrap
        }
    }
    
    /**
     * @brief Fill wavetable from arbitrary waveform function
     * @param generator Function that takes phase [0,1] and returns sample
     */
    template <typename Func>
    void generate(Func&& generator) {
        // Generate base table
        for (u32 i = 0; i <= TABLE_SIZE; ++i) {
            const f64 phase = static_cast<f64>(i) / TABLE_SIZE;
            m_tables[0][i] = static_cast<f32>(generator(phase));
        }
        
        // Generate mipmaps (progressively filtered)
        for (u32 level = 1; level < NUM_MIPMAPS; ++level) {
            // Simple lowpass: average adjacent samples
            for (u32 i = 0; i <= TABLE_SIZE; ++i) {
                const u32 i2 = (i * 2) % TABLE_SIZE;
                const u32 i2n = (i2 + 1) % TABLE_SIZE;
                m_tables[level][i] = (m_tables[level - 1][i2] + m_tables[level - 1][i2n]) * 0.5f;
            }
        }
    }
    
    /**
     * @brief Read from wavetable with linear interpolation
     * @param phase Phase in [0, 1)
     * @param mipLevel Mipmap level (based on frequency)
     */
    [[nodiscard]] f32 read(f64 phase, u32 mipLevel) const noexcept {
        mipLevel = std::min(mipLevel, NUM_MIPMAPS - 1);
        const f64 idx = phase * TABLE_SIZE;
        const u32 idx0 = static_cast<u32>(idx);
        const f32 frac = static_cast<f32>(idx - idx0);
        
        return m_tables[mipLevel][idx0] * (1.0f - frac) + 
               m_tables[mipLevel][idx0 + 1] * frac;
    }
    
    /**
     * @brief Calculate appropriate mipmap level for frequency
     */
    [[nodiscard]] static u32 getMipLevel(f64 frequency, f64 sampleRate) noexcept {
        // Each mipmap level doubles the allowed frequency
        const f64 nyquist = sampleRate * 0.5;
        const f64 ratio = nyquist / (frequency * TABLE_SIZE);
        return static_cast<u32>(std::max(0.0, std::log2(ratio)));
    }

private:
    std::vector<std::vector<f32>> m_tables;
};

/**
 * @brief Wavetable oscillator with morphing
 */
class WavetableOscillator : public IProcessor {
public:
    enum Params { pFrequency, pPosition, kNumParams };
    
    WavetableOscillator() {
        m_frequency.setImmediate(440.0f);
        m_position.setImmediate(0.0f);
        
        // Initialize with basic waveforms
        initDefaultWavetables();
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        m_frequency.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        m_position.setSmoothingTime(static_cast<f32>(sampleRate), 20.0f);
        m_phase.setFrequency(m_frequency.target(), sampleRate);
        (void)maxBlockSize;
    }
    
    void reset() override {
        m_phase.reset();
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        if (m_wavetables.empty()) return;
        
        const u32 numChannels = buffer.numChannels();
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            const f32 freq = m_frequency.next();
            m_phase.setFrequency(freq, m_sampleRate);
            
            const f64 phase = m_phase.next();
            const u32 mipLevel = Wavetable::getMipLevel(freq, m_sampleRate);
            
            // Morph between wavetables
            const f32 pos = m_position.next() * (m_wavetables.size() - 1);
            const u32 tableIdx = static_cast<u32>(pos);
            const f32 tableFrac = pos - tableIdx;
            
            f32 sample;
            if (tableIdx >= m_wavetables.size() - 1) {
                sample = m_wavetables.back().read(phase, mipLevel);
            } else {
                const f32 s0 = m_wavetables[tableIdx].read(phase, mipLevel);
                const f32 s1 = m_wavetables[tableIdx + 1].read(phase, mipLevel);
                sample = s0 * (1.0f - tableFrac) + s1 * tableFrac;
            }
            
            for (u32 ch = 0; ch < numChannels; ++ch) {
                buffer.channel(ch)[frame] = sample;
            }
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Wavetable"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Generator"; }
    
    void setFrequency(f32 hz) { m_frequency.setTarget(hz); }
    void setPosition(f32 pos) { m_position.setTarget(std::clamp(pos, 0.0f, 1.0f)); }
    
    /**
     * @brief Add a wavetable
     */
    void addWavetable(Wavetable table) {
        m_wavetables.push_back(std::move(table));
    }
    
    /**
     * @brief Clear all wavetables
     */
    void clearWavetables() {
        m_wavetables.clear();
    }

private:
    void initDefaultWavetables() {
        // Sine
        Wavetable sine;
        sine.generate([](f64 phase) { return std::sin(phase * 6.283185307179586); });
        m_wavetables.push_back(std::move(sine));
        
        // Triangle
        Wavetable tri;
        tri.generate([](f64 phase) {
            return phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase;
        });
        m_wavetables.push_back(std::move(tri));
        
        // Saw
        Wavetable saw;
        saw.generate([](f64 phase) { return 2.0 * phase - 1.0; });
        m_wavetables.push_back(std::move(saw));
        
        // Square
        Wavetable square;
        square.generate([](f64 phase) { return phase < 0.5 ? 1.0 : -1.0; });
        m_wavetables.push_back(std::move(square));
    }
    
    f64 m_sampleRate = 44100.0;
    PhaseAccumulator m_phase;
    SmoothedParameter m_frequency;
    SmoothedParameter m_position;
    std::vector<Wavetable> m_wavetables;
};

} // namespace nomad::dsp
