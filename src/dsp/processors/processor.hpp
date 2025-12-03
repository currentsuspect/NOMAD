/**
 * @file processor.hpp
 * @brief Base DSP processor interface for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file defines the core interface for all DSP processors.
 * All processors follow a consistent API for parameter handling,
 * state management, and audio processing.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../util/buffer.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

namespace nomad::dsp {

//=============================================================================
// Parameter Types
//=============================================================================

/**
 * @brief Parameter value range and metadata
 */
struct ParameterInfo {
    std::string id;
    std::string name;
    f32 minValue = 0.0f;
    f32 maxValue = 1.0f;
    f32 defaultValue = 0.0f;
    f32 step = 0.0f;          ///< 0 = continuous
    std::string unit;         ///< "Hz", "dB", "ms", "%"
    bool isLogarithmic = false;
    
    /**
     * @brief Convert normalized [0,1] to actual value
     */
    [[nodiscard]] f32 denormalize(f32 normalized) const noexcept {
        if (isLogarithmic && minValue > 0) {
            const f32 logMin = std::log(minValue);
            const f32 logMax = std::log(maxValue);
            return std::exp(logMin + normalized * (logMax - logMin));
        }
        return minValue + normalized * (maxValue - minValue);
    }
    
    /**
     * @brief Convert actual value to normalized [0,1]
     */
    [[nodiscard]] f32 normalize(f32 value) const noexcept {
        if (isLogarithmic && minValue > 0) {
            const f32 logMin = std::log(minValue);
            const f32 logMax = std::log(maxValue);
            return (std::log(value) - logMin) / (logMax - logMin);
        }
        return (value - minValue) / (maxValue - minValue);
    }
};

/**
 * @brief Parameter value with smoothing support
 */
class SmoothedParameter {
public:
    SmoothedParameter(f32 initialValue = 0.0f) noexcept
        : m_current(initialValue), m_target(initialValue) {}
    
    /**
     * @brief Set target value for smoothing
     */
    void setTarget(f32 value) noexcept {
        m_target = value;
    }
    
    /**
     * @brief Set value immediately without smoothing
     */
    void setImmediate(f32 value) noexcept {
        m_current = value;
        m_target = value;
    }
    
    /**
     * @brief Configure smoothing time
     * @param sampleRate Current sample rate
     * @param timeMs Smoothing time in milliseconds
     */
    void setSmoothingTime(f32 sampleRate, f32 timeMs) noexcept {
        if (timeMs <= 0.0f) {
            m_coeff = 1.0f;
        } else {
            // One-pole filter coefficient for ~3 time constants
            m_coeff = 1.0f - std::exp(-1000.0f / (timeMs * sampleRate));
        }
    }
    
    /**
     * @brief Get next smoothed value (call once per sample)
     */
    [[nodiscard]] f32 next() noexcept {
        m_current += m_coeff * (m_target - m_current);
        return m_current;
    }
    
    /**
     * @brief Get current value without advancing
     */
    [[nodiscard]] f32 current() const noexcept { return m_current; }
    
    /**
     * @brief Get target value
     */
    [[nodiscard]] f32 target() const noexcept { return m_target; }
    
    /**
     * @brief Check if smoothing is complete
     */
    [[nodiscard]] bool isSmoothing() const noexcept {
        return std::abs(m_target - m_current) > 1e-6f;
    }

private:
    f32 m_current;
    f32 m_target;
    f32 m_coeff = 1.0f;
};

//=============================================================================
// Processor Interface
//=============================================================================

/**
 * @brief Processing context passed to process() calls
 */
struct ProcessContext {
    f64 sampleRate = 44100.0;
    u32 blockSize = 256;
    f64 tempo = 120.0;           ///< BPM (optional, for tempo-sync)
    f64 positionSamples = 0.0;   ///< Current position in samples
    bool isPlaying = false;
    bool isRealTime = true;      ///< False for offline rendering
};

/**
 * @brief Base class for all DSP processors
 * 
 * Processors are stateful audio processing units that can be
 * connected in a graph. They follow a prepare → process → reset lifecycle.
 * 
 * @rt-safety The process() method must be real-time safe.
 */
class IProcessor {
public:
    virtual ~IProcessor() = default;
    
    //=========================================================================
    // Lifecycle
    //=========================================================================
    
    /**
     * @brief Prepare processor for playback
     * @param sampleRate Sample rate in Hz
     * @param maxBlockSize Maximum expected block size
     * 
     * Called before processing begins. Allocate any needed resources here.
     * This is NOT called from the audio thread.
     */
    virtual void prepare(f64 sampleRate, u32 maxBlockSize) = 0;
    
    /**
     * @brief Reset processor state
     * 
     * Clear delay lines, reset filters, etc. Called on transport stop
     * or when clearing state is needed.
     * 
     * @rt-safety Must be real-time safe (no allocations).
     */
    virtual void reset() = 0;
    
    /**
     * @brief Process audio buffer in-place
     * @param buffer Audio buffer to process
     * @param context Processing context
     * 
     * @rt-safety Must be real-time safe.
     */
    virtual void process(AudioBuffer& buffer, const ProcessContext& context) = 0;
    
    //=========================================================================
    // Parameters
    //=========================================================================
    
    /**
     * @brief Get number of parameters
     */
    [[nodiscard]] virtual u32 getParameterCount() const { return 0; }
    
    /**
     * @brief Get parameter info by index
     */
    [[nodiscard]] virtual ParameterInfo getParameterInfo(u32 index) const {
        (void)index;
        return {};
    }
    
    /**
     * @brief Get parameter value (normalized 0-1)
     */
    [[nodiscard]] virtual f32 getParameter(u32 index) const {
        (void)index;
        return 0.0f;
    }
    
    /**
     * @brief Set parameter value (normalized 0-1)
     * @rt-safety Must be real-time safe.
     */
    virtual void setParameter(u32 index, f32 value) {
        (void)index;
        (void)value;
    }
    
    //=========================================================================
    // Metadata
    //=========================================================================
    
    /**
     * @brief Get processor name
     */
    [[nodiscard]] virtual std::string_view getName() const = 0;
    
    /**
     * @brief Get processor category (e.g., "Filter", "Dynamics", "Delay")
     */
    [[nodiscard]] virtual std::string_view getCategory() const { return ""; }
    
    /**
     * @brief Get latency in samples (for delay compensation)
     */
    [[nodiscard]] virtual u32 getLatency() const { return 0; }
    
    /**
     * @brief Check if processor has internal state
     */
    [[nodiscard]] virtual bool hasState() const { return true; }
    
    /**
     * @brief Get tail length in samples (for reverb, delay)
     */
    [[nodiscard]] virtual u32 getTailLength() const { return 0; }
};

/**
 * @brief Factory function type for creating processors
 */
using ProcessorFactory = std::function<std::unique_ptr<IProcessor>()>;

} // namespace nomad::dsp
