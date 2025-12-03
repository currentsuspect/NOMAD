/**
 * @file filter.hpp
 * @brief Filter processors for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides various filter implementations:
 * - Biquad filters (LP, HP, BP, Notch, Peak, Shelf)
 * - State Variable Filter (SVF) for analog modeling
 * - One-pole filters for smoothing
 */

#pragma once

#include "processor.hpp"
#include <cmath>
#include <array>
#include <algorithm>

namespace nomad::dsp {

//=============================================================================
// Biquad Filter
//=============================================================================

/**
 * @brief Filter type enumeration
 */
enum class FilterType : u8 {
    LowPass,
    HighPass,
    BandPass,
    Notch,
    Peak,
    LowShelf,
    HighShelf,
    AllPass
};

/**
 * @brief Biquad filter coefficients
 */
struct BiquadCoeffs {
    f64 b0 = 1.0, b1 = 0.0, b2 = 0.0;  // Feedforward
    f64 a1 = 0.0, a2 = 0.0;             // Feedback (a0 normalized to 1)
    
    /**
     * @brief Calculate coefficients for given filter type
     * @param type Filter type
     * @param sampleRate Sample rate in Hz
     * @param frequency Cutoff/center frequency in Hz
     * @param q Q factor (resonance)
     * @param gainDb Gain in dB (for peak/shelf filters)
     */
    void calculate(FilterType type, f64 sampleRate, f64 frequency, f64 q, f64 gainDb = 0.0) {
        const f64 w0 = 2.0 * 3.14159265358979323846 * frequency / sampleRate;
        const f64 cosw0 = std::cos(w0);
        const f64 sinw0 = std::sin(w0);
        const f64 alpha = sinw0 / (2.0 * q);
        const f64 A = std::pow(10.0, gainDb / 40.0);  // For peaking/shelving
        
        f64 b0_temp, b1_temp, b2_temp, a0_temp, a1_temp, a2_temp;
        
        switch (type) {
            case FilterType::LowPass:
                b0_temp = (1.0 - cosw0) / 2.0;
                b1_temp = 1.0 - cosw0;
                b2_temp = (1.0 - cosw0) / 2.0;
                a0_temp = 1.0 + alpha;
                a1_temp = -2.0 * cosw0;
                a2_temp = 1.0 - alpha;
                break;
                
            case FilterType::HighPass:
                b0_temp = (1.0 + cosw0) / 2.0;
                b1_temp = -(1.0 + cosw0);
                b2_temp = (1.0 + cosw0) / 2.0;
                a0_temp = 1.0 + alpha;
                a1_temp = -2.0 * cosw0;
                a2_temp = 1.0 - alpha;
                break;
                
            case FilterType::BandPass:
                b0_temp = alpha;
                b1_temp = 0.0;
                b2_temp = -alpha;
                a0_temp = 1.0 + alpha;
                a1_temp = -2.0 * cosw0;
                a2_temp = 1.0 - alpha;
                break;
                
            case FilterType::Notch:
                b0_temp = 1.0;
                b1_temp = -2.0 * cosw0;
                b2_temp = 1.0;
                a0_temp = 1.0 + alpha;
                a1_temp = -2.0 * cosw0;
                a2_temp = 1.0 - alpha;
                break;
                
            case FilterType::Peak:
                b0_temp = 1.0 + alpha * A;
                b1_temp = -2.0 * cosw0;
                b2_temp = 1.0 - alpha * A;
                a0_temp = 1.0 + alpha / A;
                a1_temp = -2.0 * cosw0;
                a2_temp = 1.0 - alpha / A;
                break;
                
            case FilterType::LowShelf: {
                const f64 sqrtA = std::sqrt(A);
                const f64 sqrtA2alpha = 2.0 * sqrtA * alpha;
                b0_temp = A * ((A + 1.0) - (A - 1.0) * cosw0 + sqrtA2alpha);
                b1_temp = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
                b2_temp = A * ((A + 1.0) - (A - 1.0) * cosw0 - sqrtA2alpha);
                a0_temp = (A + 1.0) + (A - 1.0) * cosw0 + sqrtA2alpha;
                a1_temp = -2.0 * ((A - 1.0) + (A + 1.0) * cosw0);
                a2_temp = (A + 1.0) + (A - 1.0) * cosw0 - sqrtA2alpha;
                break;
            }
                
            case FilterType::HighShelf: {
                const f64 sqrtA = std::sqrt(A);
                const f64 sqrtA2alpha = 2.0 * sqrtA * alpha;
                b0_temp = A * ((A + 1.0) + (A - 1.0) * cosw0 + sqrtA2alpha);
                b1_temp = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
                b2_temp = A * ((A + 1.0) + (A - 1.0) * cosw0 - sqrtA2alpha);
                a0_temp = (A + 1.0) - (A - 1.0) * cosw0 + sqrtA2alpha;
                a1_temp = 2.0 * ((A - 1.0) - (A + 1.0) * cosw0);
                a2_temp = (A + 1.0) - (A - 1.0) * cosw0 - sqrtA2alpha;
                break;
            }
                
            case FilterType::AllPass:
                b0_temp = 1.0 - alpha;
                b1_temp = -2.0 * cosw0;
                b2_temp = 1.0 + alpha;
                a0_temp = 1.0 + alpha;
                a1_temp = -2.0 * cosw0;
                a2_temp = 1.0 - alpha;
                break;
                
            default:
                b0_temp = 1.0; b1_temp = 0.0; b2_temp = 0.0;
                a0_temp = 1.0; a1_temp = 0.0; a2_temp = 0.0;
                break;
        }
        
        // Normalize by a0
        const f64 invA0 = 1.0 / a0_temp;
        b0 = b0_temp * invA0;
        b1 = b1_temp * invA0;
        b2 = b2_temp * invA0;
        a1 = a1_temp * invA0;
        a2 = a2_temp * invA0;
    }
};

/**
 * @brief Single-channel biquad filter state
 */
struct BiquadState {
    f64 z1 = 0.0, z2 = 0.0;  // Delay elements
    
    void reset() noexcept {
        z1 = z2 = 0.0;
    }
    
    /**
     * @brief Process single sample using transposed direct form II
     */
    [[nodiscard]] f64 process(f64 input, const BiquadCoeffs& c) noexcept {
        const f64 output = c.b0 * input + z1;
        z1 = c.b1 * input - c.a1 * output + z2;
        z2 = c.b2 * input - c.a2 * output;
        return output;
    }
};

/**
 * @brief Biquad filter processor
 * 
 * Standard IIR biquad filter with multiple filter types.
 * Uses transposed direct form II for numerical stability.
 */
class BiquadFilter : public IProcessor {
public:
    // Parameter indices
    enum Params { pFrequency, pQ, pGain, pType, kNumParams };
    
    BiquadFilter() {
        m_frequency.setImmediate(1000.0f);
        m_q.setImmediate(0.707f);
        m_gain.setImmediate(0.0f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        m_frequency.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        m_q.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        m_gain.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        updateCoefficients();
        (void)maxBlockSize;
    }
    
    void reset() override {
        for (auto& state : m_states) {
            state.reset();
        }
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), static_cast<u32>(m_states.size()));
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            // Update coefficients if parameters are smoothing
            if (m_frequency.isSmoothing() || m_q.isSmoothing() || m_gain.isSmoothing()) {
                m_frequency.next();
                m_q.next();
                m_gain.next();
                updateCoefficients();
            }
            
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f32* data = buffer.channel(ch);
                data[frame] = static_cast<f32>(m_states[ch].process(data[frame], m_coeffs));
            }
        }
    }
    
    [[nodiscard]] u32 getParameterCount() const override { return kNumParams; }
    
    [[nodiscard]] ParameterInfo getParameterInfo(u32 index) const override {
        switch (index) {
            case pFrequency:
                return {"frequency", "Frequency", 20.0f, 20000.0f, 1000.0f, 0.0f, "Hz", true};
            case pQ:
                return {"q", "Q", 0.1f, 20.0f, 0.707f, 0.0f, "", true};
            case pGain:
                return {"gain", "Gain", -24.0f, 24.0f, 0.0f, 0.1f, "dB", false};
            case pType:
                return {"type", "Type", 0.0f, 7.0f, 0.0f, 1.0f, "", false};
            default:
                return {};
        }
    }
    
    [[nodiscard]] f32 getParameter(u32 index) const override {
        const auto info = getParameterInfo(index);
        switch (index) {
            case pFrequency: return info.normalize(m_frequency.target());
            case pQ: return info.normalize(m_q.target());
            case pGain: return info.normalize(m_gain.target());
            case pType: return info.normalize(static_cast<f32>(m_filterType));
            default: return 0.0f;
        }
    }
    
    void setParameter(u32 index, f32 normalizedValue) override {
        const auto info = getParameterInfo(index);
        const f32 value = info.denormalize(normalizedValue);
        
        switch (index) {
            case pFrequency: m_frequency.setTarget(value); break;
            case pQ: m_q.setTarget(value); break;
            case pGain: m_gain.setTarget(value); break;
            case pType: m_filterType = static_cast<FilterType>(static_cast<u8>(value)); break;
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Biquad Filter"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Filter"; }
    
    // Direct setters for convenience
    void setFrequency(f32 hz) { m_frequency.setTarget(hz); }
    void setQ(f32 q) { m_q.setTarget(q); }
    void setGain(f32 db) { m_gain.setTarget(db); }
    void setFilterType(FilterType type) { m_filterType = type; }

private:
    void updateCoefficients() {
        m_coeffs.calculate(
            m_filterType,
            m_sampleRate,
            m_frequency.current(),
            m_q.current(),
            m_gain.current()
        );
    }
    
    f64 m_sampleRate = 44100.0;
    FilterType m_filterType = FilterType::LowPass;
    
    SmoothedParameter m_frequency;
    SmoothedParameter m_q;
    SmoothedParameter m_gain;
    
    BiquadCoeffs m_coeffs;
    std::array<BiquadState, 8> m_states;  // Up to 8 channels
};

//=============================================================================
// State Variable Filter (SVF)
//=============================================================================

/**
 * @brief State Variable Filter
 * 
 * Analog-modeled SVF with simultaneous LP/HP/BP outputs.
 * More musical resonance than biquad at high Q values.
 * Based on the topology-preserving transform (TPT).
 */
class SVFilter : public IProcessor {
public:
    enum Params { pFrequency, pResonance, pMode, kNumParams };
    enum class Mode : u8 { LowPass, HighPass, BandPass, Notch };
    
    SVFilter() {
        m_frequency.setImmediate(1000.0f);
        m_resonance.setImmediate(0.0f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        m_frequency.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        m_resonance.setSmoothingTime(static_cast<f32>(sampleRate), 5.0f);
        updateCoefficients();
        (void)maxBlockSize;
    }
    
    void reset() override {
        for (auto& s : m_ic1eq) s = 0.0;
        for (auto& s : m_ic2eq) s = 0.0;
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), 8u);
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            if (m_frequency.isSmoothing() || m_resonance.isSmoothing()) {
                m_frequency.next();
                m_resonance.next();
                updateCoefficients();
            }
            
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f32* data = buffer.channel(ch);
                f64 v0 = data[frame];
                
                // TPT SVF equations
                f64 v3 = v0 - m_ic2eq[ch];
                f64 v1 = m_a1 * m_ic1eq[ch] + m_a2 * v3;
                f64 v2 = m_ic2eq[ch] + m_a2 * m_ic1eq[ch] + m_a3 * v3;
                
                m_ic1eq[ch] = 2.0 * v1 - m_ic1eq[ch];
                m_ic2eq[ch] = 2.0 * v2 - m_ic2eq[ch];
                
                // Select output based on mode
                f64 output;
                switch (m_mode) {
                    case Mode::LowPass:  output = v2; break;
                    case Mode::HighPass: output = v0 - m_k * v1 - v2; break;
                    case Mode::BandPass: output = v1; break;
                    case Mode::Notch:    output = v0 - m_k * v1; break;
                    default: output = v2; break;
                }
                
                data[frame] = static_cast<f32>(output);
            }
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "SVF"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Filter"; }
    
    void setFrequency(f32 hz) { m_frequency.setTarget(hz); }
    void setResonance(f32 res) { m_resonance.setTarget(res); }
    void setMode(Mode mode) { m_mode = mode; }

private:
    void updateCoefficients() {
        const f64 g = std::tan(3.14159265358979323846 * m_frequency.current() / m_sampleRate);
        m_k = 2.0 - 2.0 * m_resonance.current();  // Resonance: 0 = no res, 1 = self-oscillation
        
        m_a1 = 1.0 / (1.0 + g * (g + m_k));
        m_a2 = g * m_a1;
        m_a3 = g * m_a2;
    }
    
    f64 m_sampleRate = 44100.0;
    Mode m_mode = Mode::LowPass;
    
    SmoothedParameter m_frequency;
    SmoothedParameter m_resonance;
    
    f64 m_k = 2.0;
    f64 m_a1 = 0.0, m_a2 = 0.0, m_a3 = 0.0;
    
    std::array<f64, 8> m_ic1eq{};
    std::array<f64, 8> m_ic2eq{};
};

//=============================================================================
// One-Pole Filter (for smoothing)
//=============================================================================

/**
 * @brief Simple one-pole lowpass filter
 * 
 * Useful for parameter smoothing and simple filtering.
 */
class OnePole {
public:
    OnePole() = default;
    
    /**
     * @brief Set cutoff frequency
     */
    void setCutoff(f64 sampleRate, f64 frequencyHz) noexcept {
        m_coeff = 1.0 - std::exp(-2.0 * 3.14159265358979323846 * frequencyHz / sampleRate);
    }
    
    /**
     * @brief Set time constant directly
     */
    void setTimeConstant(f64 sampleRate, f64 timeMs) noexcept {
        if (timeMs <= 0.0) {
            m_coeff = 1.0;
        } else {
            m_coeff = 1.0 - std::exp(-1000.0 / (timeMs * sampleRate));
        }
    }
    
    /**
     * @brief Process single sample
     */
    [[nodiscard]] f64 process(f64 input) noexcept {
        m_state += m_coeff * (input - m_state);
        return m_state;
    }
    
    /**
     * @brief Reset filter state
     */
    void reset(f64 value = 0.0) noexcept {
        m_state = value;
    }
    
    [[nodiscard]] f64 state() const noexcept { return m_state; }

private:
    f64 m_coeff = 1.0;
    f64 m_state = 0.0;
};

} // namespace nomad::dsp
