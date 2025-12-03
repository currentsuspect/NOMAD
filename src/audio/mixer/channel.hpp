/**
 * @file channel.hpp
 * @brief Mixer channel strip
 * @author Nomad Team
 * @date 2025
 * 
 * Represents a single channel in the mixer with volume, pan,
 * mute/solo, and insert processing chain.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../../dsp/util/buffer.hpp"

#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <cmath>

namespace nomad::audio {

// Forward declarations
class INode;

/**
 * @brief Channel type
 */
enum class ChannelType : u8 {
    Audio,       ///< Standard audio channel
    Instrument,  ///< Instrument/MIDI channel
    Bus,         ///< Bus/group channel
    Master,      ///< Master output channel
    Return,      ///< Aux return channel
    Send         ///< Send channel
};

/**
 * @brief Channel meter data
 */
struct ChannelMeters {
    f32 peakL = 0.0f;
    f32 peakR = 0.0f;
    f32 rmsL = 0.0f;
    f32 rmsR = 0.0f;
    bool clippingL = false;
    bool clippingR = false;
    
    void reset() noexcept {
        peakL = peakR = 0.0f;
        rmsL = rmsR = 0.0f;
        clippingL = clippingR = false;
    }
};

/**
 * @brief Send configuration
 */
struct SendConfig {
    u32 targetBusId = 0;
    f32 level = 0.0f;      ///< Send level in dB
    bool preFader = false;  ///< Pre-fader send
    bool enabled = true;
    
    [[nodiscard]] f32 linearGain() const noexcept {
        return std::pow(10.0f, level / 20.0f);
    }
};

/**
 * @brief Unique identifier for channels
 */
using ChannelId = u32;
constexpr ChannelId INVALID_CHANNEL_ID = 0;

/**
 * @brief Mixer channel strip
 * 
 * A channel represents a single strip in the mixer with:
 * - Volume fader with dB scale
 * - Pan control
 * - Mute/Solo/Record arm
 * - Insert effect chain
 * - Send routing
 * - Metering
 */
class Channel {
public:
    explicit Channel(ChannelId id, ChannelType type, std::string name)
        : m_id(id)
        , m_type(type)
        , m_name(std::move(name))
    {}
    
    //=========================================================================
    // Identity
    //=========================================================================
    
    [[nodiscard]] ChannelId id() const noexcept { return m_id; }
    [[nodiscard]] ChannelType type() const noexcept { return m_type; }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }
    
    //=========================================================================
    // Volume & Pan
    //=========================================================================
    
    /**
     * @brief Get volume in dB
     */
    [[nodiscard]] f32 volume() const noexcept {
        return m_volumeDb.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Set volume in dB
     * @param db Volume in decibels (-inf to +12 typical)
     */
    void setVolume(f32 db) noexcept {
        m_volumeDb.store(std::clamp(db, -96.0f, 12.0f), std::memory_order_release);
    }
    
    /**
     * @brief Get linear volume gain
     */
    [[nodiscard]] f32 linearGain() const noexcept {
        f32 db = volume();
        if (db <= -96.0f) return 0.0f;
        return std::pow(10.0f, db / 20.0f);
    }
    
    /**
     * @brief Get pan position (-1 = left, 0 = center, +1 = right)
     */
    [[nodiscard]] f32 pan() const noexcept {
        return m_pan.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Set pan position
     */
    void setPan(f32 pan) noexcept {
        m_pan.store(std::clamp(pan, -1.0f, 1.0f), std::memory_order_release);
    }
    
    /**
     * @brief Get left/right gains from pan (constant power)
     */
    void getPanGains(f32& leftGain, f32& rightGain) const noexcept {
        f32 p = (pan() + 1.0f) * 0.5f; // 0 to 1
        // Constant power pan law
        leftGain = std::cos(p * 1.5707963f);   // pi/2
        rightGain = std::sin(p * 1.5707963f);
    }
    
    //=========================================================================
    // Mute / Solo / Record
    //=========================================================================
    
    [[nodiscard]] bool isMuted() const noexcept {
        return m_muted.load(std::memory_order_acquire);
    }
    
    void setMuted(bool muted) noexcept {
        m_muted.store(muted, std::memory_order_release);
    }
    
    [[nodiscard]] bool isSoloed() const noexcept {
        return m_soloed.load(std::memory_order_acquire);
    }
    
    void setSoloed(bool soloed) noexcept {
        m_soloed.store(soloed, std::memory_order_release);
    }
    
    [[nodiscard]] bool isRecordArmed() const noexcept {
        return m_recordArmed.load(std::memory_order_acquire);
    }
    
    void setRecordArmed(bool armed) noexcept {
        m_recordArmed.store(armed, std::memory_order_release);
    }
    
    /**
     * @brief Check if channel should be audible
     * 
     * Considers mute state and global solo state.
     * 
     * @param anySoloed Whether any channel in the mixer is soloed
     */
    [[nodiscard]] bool isAudible(bool anySoloed) const noexcept {
        if (isMuted()) return false;
        if (anySoloed && !isSoloed()) return false;
        return true;
    }
    
    //=========================================================================
    // Routing
    //=========================================================================
    
    /**
     * @brief Get output bus ID
     */
    [[nodiscard]] ChannelId outputBus() const noexcept {
        return m_outputBus;
    }
    
    /**
     * @brief Set output bus
     */
    void setOutputBus(ChannelId busId) noexcept {
        m_outputBus = busId;
    }
    
    /**
     * @brief Get sends
     */
    [[nodiscard]] const std::vector<SendConfig>& sends() const noexcept {
        return m_sends;
    }
    
    /**
     * @brief Add a send
     */
    void addSend(const SendConfig& send) {
        m_sends.push_back(send);
    }
    
    /**
     * @brief Remove a send
     */
    bool removeSend(u32 targetBusId) {
        auto it = std::find_if(m_sends.begin(), m_sends.end(),
            [targetBusId](const SendConfig& s) { return s.targetBusId == targetBusId; });
        if (it != m_sends.end()) {
            m_sends.erase(it);
            return true;
        }
        return false;
    }
    
    //=========================================================================
    // Processing
    //=========================================================================
    
    /**
     * @brief Process audio through the channel
     * 
     * Applies volume, pan, and routes to outputs.
     * 
     * @rt-safety Real-time safe
     * 
     * @param buffer Audio buffer to process (in-place)
     * @param anySoloed Whether any channel is soloed (for solo logic)
     */
    void process(dsp::AudioBuffer& buffer, bool anySoloed) noexcept {
        if (!isAudible(anySoloed)) {
            buffer.clear();
            return;
        }
        
        const f32 gain = linearGain();
        f32 gainL, gainR;
        getPanGains(gainL, gainR);
        gainL *= gain;
        gainR *= gain;
        
        // Apply gain and pan
        for (u32 i = 0; i < buffer.frameCount(); ++i) {
            buffer.channel(0)[i] *= gainL;
            if (buffer.channelCount() > 1) {
                buffer.channel(1)[i] *= gainR;
            }
        }
        
        // Update meters
        updateMeters(buffer);
    }
    
    //=========================================================================
    // Metering
    //=========================================================================
    
    /**
     * @brief Get current meter readings
     */
    [[nodiscard]] ChannelMeters meters() const noexcept {
        return m_meters;
    }
    
    /**
     * @brief Reset meters (clear peak hold)
     */
    void resetMeters() noexcept {
        m_meters.reset();
    }

private:
    void updateMeters(const dsp::AudioBuffer& buffer) noexcept {
        // Calculate peak and RMS
        f32 peakL = 0.0f, peakR = 0.0f;
        f32 sumL = 0.0f, sumR = 0.0f;
        
        const f32* left = buffer.channel(0);
        const f32* right = buffer.channelCount() > 1 ? buffer.channel(1) : left;
        
        for (u32 i = 0; i < buffer.frameCount(); ++i) {
            f32 absL = std::abs(left[i]);
            f32 absR = std::abs(right[i]);
            
            peakL = std::max(peakL, absL);
            peakR = std::max(peakR, absR);
            sumL += left[i] * left[i];
            sumR += right[i] * right[i];
        }
        
        // Update with decay
        constexpr f32 peakDecay = 0.9995f;
        m_meters.peakL = std::max(peakL, m_meters.peakL * peakDecay);
        m_meters.peakR = std::max(peakR, m_meters.peakR * peakDecay);
        m_meters.rmsL = std::sqrt(sumL / buffer.frameCount());
        m_meters.rmsR = std::sqrt(sumR / buffer.frameCount());
        m_meters.clippingL = peakL >= 1.0f;
        m_meters.clippingR = peakR >= 1.0f;
    }
    
    // Identity
    ChannelId m_id;
    ChannelType m_type;
    std::string m_name;
    
    // Volume/Pan (atomic for RT-safe access from UI)
    std::atomic<f32> m_volumeDb{0.0f};
    std::atomic<f32> m_pan{0.0f};
    
    // State
    std::atomic<bool> m_muted{false};
    std::atomic<bool> m_soloed{false};
    std::atomic<bool> m_recordArmed{false};
    
    // Routing
    ChannelId m_outputBus = INVALID_CHANNEL_ID;
    std::vector<SendConfig> m_sends;
    
    // Metering
    ChannelMeters m_meters;
    
    // Insert chain (node IDs)
    std::vector<u32> m_inserts;
};

} // namespace nomad::audio
