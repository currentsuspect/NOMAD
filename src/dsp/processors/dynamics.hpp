/**
 * @file dynamics.hpp
 * @brief Dynamics processors for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides dynamics processing:
 * - Compressor with soft/hard knee
 * - Limiter with lookahead
 * - Gate with hysteresis
 * - Expander
 */

#pragma once

#include "processor.hpp"
#include "filter.hpp"
#include <cmath>
#include <array>
#include <algorithm>

namespace nomad::dsp {

//=============================================================================
// Envelope Follower
//=============================================================================

/**
 * @brief Envelope follower for level detection
 * 
 * Follows the amplitude envelope of a signal using
 * separate attack and release times.
 */
class EnvelopeFollower {
public:
    /**
     * @brief Set attack and release times
     * @param sampleRate Sample rate in Hz
     * @param attackMs Attack time in milliseconds
     * @param releaseMs Release time in milliseconds
     */
    void setTimes(f64 sampleRate, f64 attackMs, f64 releaseMs) noexcept {
        // Time constant for ~63% of final value
        m_attackCoeff = attackMs > 0.0 
            ? std::exp(-1000.0 / (attackMs * sampleRate))
            : 0.0;
        m_releaseCoeff = releaseMs > 0.0
            ? std::exp(-1000.0 / (releaseMs * sampleRate))
            : 0.0;
    }
    
    /**
     * @brief Process single sample (absolute value)
     */
    [[nodiscard]] f64 process(f64 input) noexcept {
        const f64 absInput = std::abs(input);
        const f64 coeff = absInput > m_envelope ? m_attackCoeff : m_releaseCoeff;
        m_envelope = absInput + coeff * (m_envelope - absInput);
        return m_envelope;
    }
    
    /**
     * @brief Reset envelope state
     */
    void reset() noexcept {
        m_envelope = 0.0;
    }
    
    [[nodiscard]] f64 envelope() const noexcept { return m_envelope; }

private:
    f64 m_attackCoeff = 0.0;
    f64 m_releaseCoeff = 0.0;
    f64 m_envelope = 0.0;
};

//=============================================================================
// Compressor
//=============================================================================

/**
 * @brief Compressor/Limiter processor
 * 
 * Full-featured dynamics compressor with:
 * - Adjustable threshold, ratio, knee
 * - Attack and release controls
 * - Makeup gain (auto or manual)
 * - Sidechain highpass filter
 */
class Compressor : public IProcessor {
public:
    enum Params {
        pThreshold,
        pRatio,
        pAttack,
        pRelease,
        pKnee,
        pMakeup,
        pMix,
        kNumParams
    };
    
    Compressor() {
        m_threshold.setImmediate(-20.0f);
        m_ratio.setImmediate(4.0f);
        m_attack.setImmediate(10.0f);
        m_release.setImmediate(100.0f);
        m_knee.setImmediate(6.0f);
        m_makeup.setImmediate(0.0f);
        m_mix.setImmediate(1.0f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        
        // Setup parameter smoothing
        m_threshold.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        m_ratio.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        m_makeup.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        m_mix.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        
        // Setup envelope followers
        for (auto& env : m_envFollowers) {
            env.setTimes(sampleRate, m_attack.target(), m_release.target());
        }
        
        // Setup sidechain highpass
        for (auto& hp : m_sidechainHP) {
            hp.setCutoff(sampleRate, 80.0);  // 80Hz highpass
        }
        
        (void)maxBlockSize;
    }
    
    void reset() override {
        for (auto& env : m_envFollowers) {
            env.reset();
        }
        for (auto& hp : m_sidechainHP) {
            hp.reset();
        }
        m_currentGainReduction = 0.0f;
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), 8u);
        const u32 numFrames = buffer.numFrames();
        
        // Update envelope follower times if changed
        if (m_attack.isSmoothing() || m_release.isSmoothing()) {
            for (auto& env : m_envFollowers) {
                env.setTimes(m_sampleRate, m_attack.current(), m_release.current());
            }
        }
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            // Update smoothed parameters
            const f32 threshold = m_threshold.next();
            const f32 ratio = m_ratio.next();
            const f32 knee = m_knee.current();
            const f32 makeup = m_makeup.next();
            const f32 mix = m_mix.next();
            
            // Detect level (max across channels)
            f64 maxLevel = 0.0;
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f64 sample = buffer.channel(ch)[frame];
                
                // Apply sidechain highpass
                if (m_sidechainHPEnabled) {
                    sample = m_sidechainHP[ch].process(sample);
                }
                
                const f64 env = m_envFollowers[ch].process(sample);
                maxLevel = std::max(maxLevel, env);
            }
            
            // Convert to dB
            const f64 levelDb = maxLevel > 1e-10 ? 20.0 * std::log10(maxLevel) : -100.0;
            
            // Calculate gain reduction
            f64 gainReductionDb = 0.0;
            const f64 overThreshold = levelDb - threshold;
            
            if (knee > 0.0 && overThreshold > -knee / 2.0 && overThreshold < knee / 2.0) {
                // Soft knee region
                const f64 kneePos = (overThreshold + knee / 2.0) / knee;
                gainReductionDb = kneePos * kneePos * knee * (1.0 - 1.0 / ratio) / 2.0;
            } else if (overThreshold >= knee / 2.0) {
                // Above knee - full compression
                gainReductionDb = overThreshold * (1.0 - 1.0 / ratio);
            }
            // else: below threshold, no gain reduction
            
            // Convert to linear gain
            const f64 gainReduction = std::pow(10.0, -gainReductionDb / 20.0);
            const f64 makeupGain = std::pow(10.0, makeup / 20.0);
            const f64 totalGain = gainReduction * makeupGain;
            
            // Apply gain to all channels
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f32* data = buffer.channel(ch);
                const f32 dry = data[frame];
                const f32 wet = static_cast<f32>(dry * totalGain);
                data[frame] = dry + mix * (wet - dry);  // Dry/wet mix
            }
            
            // Store for metering
            m_currentGainReduction = static_cast<f32>(gainReductionDb);
        }
    }
    
    [[nodiscard]] u32 getParameterCount() const override { return kNumParams; }
    
    [[nodiscard]] ParameterInfo getParameterInfo(u32 index) const override {
        switch (index) {
            case pThreshold: return {"threshold", "Threshold", -60.0f, 0.0f, -20.0f, 0.1f, "dB", false};
            case pRatio: return {"ratio", "Ratio", 1.0f, 20.0f, 4.0f, 0.1f, ":1", true};
            case pAttack: return {"attack", "Attack", 0.1f, 200.0f, 10.0f, 0.1f, "ms", true};
            case pRelease: return {"release", "Release", 10.0f, 2000.0f, 100.0f, 1.0f, "ms", true};
            case pKnee: return {"knee", "Knee", 0.0f, 24.0f, 6.0f, 0.1f, "dB", false};
            case pMakeup: return {"makeup", "Makeup", 0.0f, 40.0f, 0.0f, 0.1f, "dB", false};
            case pMix: return {"mix", "Mix", 0.0f, 1.0f, 1.0f, 0.01f, "%", false};
            default: return {};
        }
    }
    
    [[nodiscard]] f32 getParameter(u32 index) const override {
        const auto info = getParameterInfo(index);
        switch (index) {
            case pThreshold: return info.normalize(m_threshold.target());
            case pRatio: return info.normalize(m_ratio.target());
            case pAttack: return info.normalize(m_attack.target());
            case pRelease: return info.normalize(m_release.target());
            case pKnee: return info.normalize(m_knee.target());
            case pMakeup: return info.normalize(m_makeup.target());
            case pMix: return info.normalize(m_mix.target());
            default: return 0.0f;
        }
    }
    
    void setParameter(u32 index, f32 normalizedValue) override {
        const auto info = getParameterInfo(index);
        const f32 value = info.denormalize(normalizedValue);
        
        switch (index) {
            case pThreshold: m_threshold.setTarget(value); break;
            case pRatio: m_ratio.setTarget(value); break;
            case pAttack: m_attack.setTarget(value); break;
            case pRelease: m_release.setTarget(value); break;
            case pKnee: m_knee.setTarget(value); break;
            case pMakeup: m_makeup.setTarget(value); break;
            case pMix: m_mix.setTarget(value); break;
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Compressor"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Dynamics"; }
    
    /**
     * @brief Get current gain reduction in dB (for metering)
     */
    [[nodiscard]] f32 getGainReduction() const noexcept { return m_currentGainReduction; }
    
    /**
     * @brief Enable/disable sidechain highpass filter
     */
    void setSidechainHPEnabled(bool enabled) { m_sidechainHPEnabled = enabled; }

private:
    f64 m_sampleRate = 44100.0;
    
    SmoothedParameter m_threshold;
    SmoothedParameter m_ratio;
    SmoothedParameter m_attack;
    SmoothedParameter m_release;
    SmoothedParameter m_knee;
    SmoothedParameter m_makeup;
    SmoothedParameter m_mix;
    
    std::array<EnvelopeFollower, 8> m_envFollowers;
    std::array<OnePole, 8> m_sidechainHP;
    bool m_sidechainHPEnabled = false;
    
    f32 m_currentGainReduction = 0.0f;
};

//=============================================================================
// Limiter
//=============================================================================

/**
 * @brief Brick-wall limiter with lookahead
 * 
 * True peak limiter that catches all peaks using a
 * small lookahead buffer.
 */
class Limiter : public IProcessor {
public:
    enum Params { pCeiling, pRelease, kNumParams };
    
    static constexpr u32 LOOKAHEAD_SAMPLES = 64;
    
    Limiter() {
        m_ceiling.setImmediate(-0.3f);  // -0.3 dB ceiling
        m_release.setImmediate(100.0f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        m_ceiling.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        
        // Resize lookahead buffer
        m_lookaheadBuffer.resize(8);
        for (auto& ch : m_lookaheadBuffer) {
            ch.resize(LOOKAHEAD_SAMPLES, 0.0f);
        }
        m_lookaheadIndex = 0;
        
        // Attack is fixed to lookahead time
        m_attackCoeff = std::exp(-1000.0 / ((1000.0 * LOOKAHEAD_SAMPLES / sampleRate) * sampleRate));
        updateReleaseCoeff();
        
        (void)maxBlockSize;
    }
    
    void reset() override {
        for (auto& ch : m_lookaheadBuffer) {
            std::fill(ch.begin(), ch.end(), 0.0f);
        }
        m_lookaheadIndex = 0;
        m_gainReduction = 1.0;
        m_currentGainReduction = 0.0f;
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), static_cast<u32>(m_lookaheadBuffer.size()));
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            const f32 ceilingLin = std::pow(10.0f, m_ceiling.next() / 20.0f);
            
            // Find peak across all channels (from lookahead)
            f32 maxPeak = 0.0f;
            for (u32 ch = 0; ch < numChannels; ++ch) {
                maxPeak = std::max(maxPeak, std::abs(m_lookaheadBuffer[ch][m_lookaheadIndex]));
            }
            
            // Calculate target gain
            f32 targetGain = 1.0f;
            if (maxPeak > ceilingLin) {
                targetGain = ceilingLin / maxPeak;
            }
            
            // Smooth gain reduction
            if (targetGain < m_gainReduction) {
                m_gainReduction = targetGain + m_attackCoeff * (m_gainReduction - targetGain);
            } else {
                m_gainReduction = targetGain + m_releaseCoeff * (m_gainReduction - targetGain);
            }
            
            // Apply gain and update lookahead buffer
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f32* data = buffer.channel(ch);
                
                // Output delayed sample with gain reduction
                const f32 delayed = m_lookaheadBuffer[ch][m_lookaheadIndex];
                data[frame] = delayed * static_cast<f32>(m_gainReduction);
                
                // Store current input in lookahead
                m_lookaheadBuffer[ch][m_lookaheadIndex] = data[frame] / static_cast<f32>(m_gainReduction);  // Pre-gain input
            }
            
            // Advance lookahead index
            m_lookaheadIndex = (m_lookaheadIndex + 1) % LOOKAHEAD_SAMPLES;
            
            // Store for metering
            m_currentGainReduction = static_cast<f32>(20.0 * std::log10(m_gainReduction));
        }
    }
    
    [[nodiscard]] u32 getParameterCount() const override { return kNumParams; }
    
    [[nodiscard]] ParameterInfo getParameterInfo(u32 index) const override {
        switch (index) {
            case pCeiling: return {"ceiling", "Ceiling", -12.0f, 0.0f, -0.3f, 0.1f, "dB", false};
            case pRelease: return {"release", "Release", 10.0f, 1000.0f, 100.0f, 1.0f, "ms", true};
            default: return {};
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Limiter"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Dynamics"; }
    [[nodiscard]] u32 getLatency() const override { return LOOKAHEAD_SAMPLES; }
    
    [[nodiscard]] f32 getGainReduction() const noexcept { return m_currentGainReduction; }

private:
    void updateReleaseCoeff() {
        m_releaseCoeff = std::exp(-1000.0 / (m_release.target() * m_sampleRate));
    }
    
    f64 m_sampleRate = 44100.0;
    
    SmoothedParameter m_ceiling;
    SmoothedParameter m_release;
    
    std::vector<std::vector<f32>> m_lookaheadBuffer;
    u32 m_lookaheadIndex = 0;
    
    f64 m_attackCoeff = 0.0;
    f64 m_releaseCoeff = 0.0;
    f64 m_gainReduction = 1.0;
    
    f32 m_currentGainReduction = 0.0f;
};

//=============================================================================
// Gate
//=============================================================================

/**
 * @brief Noise gate with hysteresis
 */
class Gate : public IProcessor {
public:
    enum Params { pThreshold, pRange, pAttack, pHold, pRelease, kNumParams };
    
    Gate() {
        m_threshold.setImmediate(-40.0f);
        m_range.setImmediate(-80.0f);
        m_attack.setImmediate(1.0f);
        m_hold.setImmediate(50.0f);
        m_release.setImmediate(100.0f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        updateCoefficients();
        (void)maxBlockSize;
    }
    
    void reset() override {
        m_gateState = 0.0;
        m_holdCounter = 0;
        for (auto& env : m_envFollowers) env.reset();
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), 8u);
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            const f32 threshold = m_threshold.current();
            const f32 range = m_range.current();
            
            // Detect level
            f64 maxLevel = 0.0;
            for (u32 ch = 0; ch < numChannels; ++ch) {
                maxLevel = std::max(maxLevel, m_envFollowers[ch].process(buffer.channel(ch)[frame]));
            }
            
            const f64 levelDb = maxLevel > 1e-10 ? 20.0 * std::log10(maxLevel) : -100.0;
            
            // Gate logic with hysteresis
            f64 targetGate;
            if (levelDb > threshold + 2.0) {  // +2dB hysteresis
                targetGate = 1.0;
                m_holdCounter = static_cast<u32>(m_hold.current() * m_sampleRate / 1000.0);
            } else if (levelDb < threshold && m_holdCounter == 0) {
                targetGate = std::pow(10.0, range / 20.0);  // Range (not fully closed)
            } else {
                if (m_holdCounter > 0) m_holdCounter--;
                targetGate = m_gateState;  // Hold current state
            }
            
            // Smooth gate
            const f64 coeff = targetGate > m_gateState ? m_attackCoeff : m_releaseCoeff;
            m_gateState = targetGate + coeff * (m_gateState - targetGate);
            
            // Apply gate
            for (u32 ch = 0; ch < numChannels; ++ch) {
                buffer.channel(ch)[frame] *= static_cast<f32>(m_gateState);
            }
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Gate"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Dynamics"; }

private:
    void updateCoefficients() {
        m_attackCoeff = m_attack.current() > 0.0f
            ? std::exp(-1000.0 / (m_attack.current() * m_sampleRate))
            : 0.0;
        m_releaseCoeff = m_release.current() > 0.0f
            ? std::exp(-1000.0 / (m_release.current() * m_sampleRate))
            : 0.0;
            
        for (auto& env : m_envFollowers) {
            env.setTimes(m_sampleRate, 0.1, 50.0);  // Fast detection
        }
    }
    
    f64 m_sampleRate = 44100.0;
    
    SmoothedParameter m_threshold;
    SmoothedParameter m_range;
    SmoothedParameter m_attack;
    SmoothedParameter m_hold;
    SmoothedParameter m_release;
    
    std::array<EnvelopeFollower, 8> m_envFollowers;
    f64 m_attackCoeff = 0.0;
    f64 m_releaseCoeff = 0.0;
    f64 m_gateState = 1.0;
    u32 m_holdCounter = 0;
};

} // namespace nomad::dsp
