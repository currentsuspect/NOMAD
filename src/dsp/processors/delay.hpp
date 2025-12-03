/**
 * @file delay.hpp
 * @brief Delay-based effects for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides delay-based effects:
 * - Simple delay with feedback
 * - Ping-pong delay
 * - Modulated delay (chorus/flanger)
 */

#pragma once

#include "processor.hpp"
#include "filter.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

namespace nomad::dsp {

//=============================================================================
// Delay Line
//=============================================================================

/**
 * @brief Fractional delay line with interpolation
 * 
 * Supports linear and cubic interpolation for smooth
 * modulated delays.
 */
class DelayLine {
public:
    /**
     * @brief Prepare delay line
     * @param maxDelaySamples Maximum delay in samples
     */
    void prepare(u32 maxDelaySamples) {
        m_buffer.resize(maxDelaySamples + 4, 0.0f);  // +4 for cubic interpolation
        m_mask = static_cast<u32>(m_buffer.size()) - 1;
        m_writeIndex = 0;
    }
    
    /**
     * @brief Reset delay line to silence
     */
    void reset() noexcept {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    }
    
    /**
     * @brief Write sample to delay line
     */
    void write(f32 sample) noexcept {
        m_buffer[m_writeIndex] = sample;
        m_writeIndex = (m_writeIndex + 1) & m_mask;
    }
    
    /**
     * @brief Read from delay line with linear interpolation
     * @param delaySamples Delay time in fractional samples
     */
    [[nodiscard]] f32 readLinear(f32 delaySamples) const noexcept {
        const f32 readPos = static_cast<f32>(m_writeIndex) - delaySamples;
        const i32 readIdx = static_cast<i32>(std::floor(readPos));
        const f32 frac = readPos - readIdx;
        
        const u32 idx0 = (readIdx + m_buffer.size()) & m_mask;
        const u32 idx1 = (idx0 + 1) & m_mask;
        
        return m_buffer[idx0] * (1.0f - frac) + m_buffer[idx1] * frac;
    }
    
    /**
     * @brief Read from delay line with cubic interpolation
     * @param delaySamples Delay time in fractional samples
     */
    [[nodiscard]] f32 readCubic(f32 delaySamples) const noexcept {
        const f32 readPos = static_cast<f32>(m_writeIndex) - delaySamples;
        const i32 readIdx = static_cast<i32>(std::floor(readPos));
        const f32 frac = readPos - readIdx;
        
        const u32 idx0 = (readIdx - 1 + m_buffer.size()) & m_mask;
        const u32 idx1 = (readIdx + m_buffer.size()) & m_mask;
        const u32 idx2 = (readIdx + 1 + m_buffer.size()) & m_mask;
        const u32 idx3 = (readIdx + 2 + m_buffer.size()) & m_mask;
        
        const f32 y0 = m_buffer[idx0];
        const f32 y1 = m_buffer[idx1];
        const f32 y2 = m_buffer[idx2];
        const f32 y3 = m_buffer[idx3];
        
        // Cubic Hermite interpolation
        const f32 c0 = y1;
        const f32 c1 = 0.5f * (y2 - y0);
        const f32 c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const f32 c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
    
    /**
     * @brief Read at integer delay
     */
    [[nodiscard]] f32 read(u32 delaySamples) const noexcept {
        const u32 idx = (m_writeIndex - delaySamples + m_buffer.size()) & m_mask;
        return m_buffer[idx];
    }
    
    [[nodiscard]] u32 maxDelay() const noexcept { return static_cast<u32>(m_buffer.size()) - 4; }

private:
    std::vector<f32> m_buffer;
    u32 m_writeIndex = 0;
    u32 m_mask = 0;
};

//=============================================================================
// Simple Delay
//=============================================================================

/**
 * @brief Simple delay effect with feedback and filtering
 */
class Delay : public IProcessor {
public:
    enum Params { pTime, pFeedback, pMix, pHighCut, kNumParams };
    
    static constexpr f32 MAX_DELAY_MS = 2000.0f;
    
    Delay() {
        m_time.setImmediate(250.0f);
        m_feedback.setImmediate(0.5f);
        m_mix.setImmediate(0.5f);
        m_highCut.setImmediate(8000.0f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        
        const u32 maxDelaySamples = static_cast<u32>(MAX_DELAY_MS * sampleRate / 1000.0) + 1;
        
        m_delayLines.resize(2);
        for (auto& dl : m_delayLines) {
            dl.prepare(maxDelaySamples);
        }
        
        m_filters.resize(2);
        for (auto& f : m_filters) {
            f.setCutoff(sampleRate, m_highCut.target());
        }
        
        m_time.setSmoothingTime(static_cast<f32>(sampleRate), 50.0f);
        m_feedback.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        m_mix.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        
        (void)maxBlockSize;
    }
    
    void reset() override {
        for (auto& dl : m_delayLines) dl.reset();
        for (auto& f : m_filters) f.reset();
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), 2u);
        const u32 numFrames = buffer.numFrames();
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            const f32 delayMs = m_time.next();
            const f32 delaySamples = delayMs * static_cast<f32>(m_sampleRate) / 1000.0f;
            const f32 feedback = m_feedback.next();
            const f32 mix = m_mix.next();
            
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f32* data = buffer.channel(ch);
                const f32 dry = data[frame];
                
                // Read from delay
                f32 delayed = m_delayLines[ch].readLinear(delaySamples);
                
                // Apply feedback filter
                delayed = static_cast<f32>(m_filters[ch].process(delayed));
                
                // Write to delay (input + filtered feedback)
                m_delayLines[ch].write(dry + delayed * feedback);
                
                // Output with dry/wet mix
                data[frame] = dry * (1.0f - mix) + delayed * mix;
            }
        }
    }
    
    [[nodiscard]] u32 getParameterCount() const override { return kNumParams; }
    
    [[nodiscard]] ParameterInfo getParameterInfo(u32 index) const override {
        switch (index) {
            case pTime: return {"time", "Time", 1.0f, MAX_DELAY_MS, 250.0f, 1.0f, "ms", true};
            case pFeedback: return {"feedback", "Feedback", 0.0f, 0.99f, 0.5f, 0.01f, "%", false};
            case pMix: return {"mix", "Mix", 0.0f, 1.0f, 0.5f, 0.01f, "%", false};
            case pHighCut: return {"highCut", "High Cut", 200.0f, 20000.0f, 8000.0f, 1.0f, "Hz", true};
            default: return {};
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Delay"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Delay"; }
    [[nodiscard]] u32 getTailLength() const override { 
        // Estimate tail based on feedback decay
        const f32 fb = m_feedback.target();
        if (fb < 0.01f) return 0;
        // Time for feedback to decay to -60dB
        const f32 decayTime = -60.0f / (20.0f * std::log10(fb));
        return static_cast<u32>(decayTime * m_time.target() * m_sampleRate / 1000.0f);
    }

private:
    f64 m_sampleRate = 44100.0;
    
    SmoothedParameter m_time;
    SmoothedParameter m_feedback;
    SmoothedParameter m_mix;
    SmoothedParameter m_highCut;
    
    std::vector<DelayLine> m_delayLines;
    std::vector<OnePole> m_filters;
};

//=============================================================================
// Ping Pong Delay
//=============================================================================

/**
 * @brief Stereo ping-pong delay
 */
class PingPongDelay : public IProcessor {
public:
    enum Params { pTime, pFeedback, pMix, pPan, kNumParams };
    
    static constexpr f32 MAX_DELAY_MS = 2000.0f;
    
    PingPongDelay() {
        m_time.setImmediate(250.0f);
        m_feedback.setImmediate(0.5f);
        m_mix.setImmediate(0.5f);
        m_pan.setImmediate(1.0f);  // Full ping-pong
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        
        const u32 maxDelaySamples = static_cast<u32>(MAX_DELAY_MS * sampleRate / 1000.0) + 1;
        
        m_leftDelay.prepare(maxDelaySamples);
        m_rightDelay.prepare(maxDelaySamples);
        
        m_time.setSmoothingTime(static_cast<f32>(sampleRate), 50.0f);
        m_feedback.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        m_mix.setSmoothingTime(static_cast<f32>(sampleRate), 10.0f);
        
        (void)maxBlockSize;
    }
    
    void reset() override {
        m_leftDelay.reset();
        m_rightDelay.reset();
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        if (buffer.numChannels() < 2) return;  // Requires stereo
        
        const u32 numFrames = buffer.numFrames();
        f32* leftData = buffer.channel(0);
        f32* rightData = buffer.channel(1);
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            const f32 delayMs = m_time.next();
            const f32 delaySamples = delayMs * static_cast<f32>(m_sampleRate) / 1000.0f;
            const f32 feedback = m_feedback.next();
            const f32 mix = m_mix.next();
            const f32 pan = m_pan.current();  // Pan width
            
            const f32 dryL = leftData[frame];
            const f32 dryR = rightData[frame];
            
            // Read from delays
            const f32 delayedL = m_leftDelay.readLinear(delaySamples);
            const f32 delayedR = m_rightDelay.readLinear(delaySamples);
            
            // Ping-pong: cross-feed between channels
            const f32 toLeft = dryL + delayedR * feedback * pan + delayedL * feedback * (1.0f - pan);
            const f32 toRight = dryR + delayedL * feedback * pan + delayedR * feedback * (1.0f - pan);
            
            m_leftDelay.write(toLeft);
            m_rightDelay.write(toRight);
            
            // Output
            leftData[frame] = dryL * (1.0f - mix) + delayedL * mix;
            rightData[frame] = dryR * (1.0f - mix) + delayedR * mix;
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Ping Pong Delay"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Delay"; }

private:
    f64 m_sampleRate = 44100.0;
    
    SmoothedParameter m_time;
    SmoothedParameter m_feedback;
    SmoothedParameter m_mix;
    SmoothedParameter m_pan;
    
    DelayLine m_leftDelay;
    DelayLine m_rightDelay;
};

//=============================================================================
// Chorus
//=============================================================================

/**
 * @brief Chorus effect using modulated delay
 */
class Chorus : public IProcessor {
public:
    enum Params { pRate, pDepth, pMix, kNumParams };
    
    Chorus() {
        m_rate.setImmediate(1.0f);
        m_depth.setImmediate(0.5f);
        m_mix.setImmediate(0.5f);
    }
    
    void prepare(f64 sampleRate, u32 maxBlockSize) override {
        m_sampleRate = sampleRate;
        
        // Max delay ~30ms for chorus
        const u32 maxDelaySamples = static_cast<u32>(30.0 * sampleRate / 1000.0) + 1;
        
        m_delayLines.resize(2);
        for (auto& dl : m_delayLines) {
            dl.prepare(maxDelaySamples);
        }
        
        m_lfoPhase.setFrequency(m_rate.target(), sampleRate);
        
        (void)maxBlockSize;
    }
    
    void reset() override {
        for (auto& dl : m_delayLines) dl.reset();
        m_lfoPhase.reset();
    }
    
    void process(AudioBuffer& buffer, const ProcessContext& context) override {
        (void)context;
        
        const u32 numChannels = std::min(buffer.numChannels(), 2u);
        const u32 numFrames = buffer.numFrames();
        
        // Base delay ~7ms
        const f32 baseDelay = 7.0f * static_cast<f32>(m_sampleRate) / 1000.0f;
        // Max modulation ~3ms
        const f32 modRange = 3.0f * static_cast<f32>(m_sampleRate) / 1000.0f;
        
        for (u32 frame = 0; frame < numFrames; ++frame) {
            m_lfoPhase.setFrequency(m_rate.current(), m_sampleRate);
            const f64 lfoPhase = m_lfoPhase.next();
            const f32 depth = m_depth.current();
            const f32 mix = m_mix.current();
            
            // Stereo LFO with 90-degree offset
            const f32 lfoL = static_cast<f32>(std::sin(lfoPhase * 6.283185307179586));
            const f32 lfoR = static_cast<f32>(std::sin((lfoPhase + 0.25) * 6.283185307179586));
            
            for (u32 ch = 0; ch < numChannels; ++ch) {
                f32* data = buffer.channel(ch);
                const f32 dry = data[frame];
                
                // Modulated delay time
                const f32 lfo = ch == 0 ? lfoL : lfoR;
                const f32 delaySamples = baseDelay + lfo * modRange * depth;
                
                m_delayLines[ch].write(dry);
                const f32 wet = m_delayLines[ch].readCubic(delaySamples);
                
                data[frame] = dry * (1.0f - mix * 0.5f) + wet * mix;
            }
        }
    }
    
    [[nodiscard]] std::string_view getName() const override { return "Chorus"; }
    [[nodiscard]] std::string_view getCategory() const override { return "Modulation"; }

private:
    f64 m_sampleRate = 44100.0;
    
    SmoothedParameter m_rate;
    SmoothedParameter m_depth;
    SmoothedParameter m_mix;
    
    PhaseAccumulator m_lfoPhase;
    std::vector<DelayLine> m_delayLines;
};

} // namespace nomad::dsp
