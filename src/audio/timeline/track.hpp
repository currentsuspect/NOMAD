/**
 * @file track.hpp
 * @brief Timeline track with clips and regions
 * @author Nomad Team
 * @date 2025
 * 
 * Represents a track in the arrangement view containing
 * audio/MIDI clips and automation lanes.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../mixer/channel.hpp"

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace nomad::audio {

/**
 * @brief Track type
 */
enum class TrackType : u8 {
    Audio,       ///< Audio track
    Instrument,  ///< MIDI/Instrument track
    Bus,         ///< Bus/group track
    Master,      ///< Master track
    Automation,  ///< Automation-only track
    Folder       ///< Folder track (for organization)
};

/**
 * @brief Unique identifier for tracks
 */
using TrackId = u32;
constexpr TrackId INVALID_TRACK_ID = 0;

/**
 * @brief Unique identifier for clips
 */
using ClipId = u32;
constexpr ClipId INVALID_CLIP_ID = 0;

/**
 * @brief Base clip class
 */
class Clip {
public:
    Clip(ClipId id, i64 startSample, i64 lengthSamples)
        : m_id(id)
        , m_startSample(startSample)
        , m_lengthSamples(lengthSamples)
    {}
    
    virtual ~Clip() = default;
    
    [[nodiscard]] ClipId id() const noexcept { return m_id; }
    
    [[nodiscard]] i64 startSample() const noexcept { return m_startSample; }
    void setStartSample(i64 sample) noexcept { m_startSample = sample; }
    
    [[nodiscard]] i64 lengthSamples() const noexcept { return m_lengthSamples; }
    void setLengthSamples(i64 samples) noexcept { m_lengthSamples = samples; }
    
    [[nodiscard]] i64 endSample() const noexcept { 
        return m_startSample + m_lengthSamples; 
    }
    
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }
    
    [[nodiscard]] u32 color() const noexcept { return m_color; }
    void setColor(u32 color) noexcept { m_color = color; }
    
    [[nodiscard]] bool isMuted() const noexcept { return m_muted; }
    void setMuted(bool muted) noexcept { m_muted = muted; }
    
    /**
     * @brief Check if clip contains a sample position
     */
    [[nodiscard]] bool contains(i64 samplePos) const noexcept {
        return samplePos >= m_startSample && samplePos < endSample();
    }
    
    /**
     * @brief Check if clip overlaps with a range
     */
    [[nodiscard]] bool overlaps(i64 rangeStart, i64 rangeEnd) const noexcept {
        return m_startSample < rangeEnd && endSample() > rangeStart;
    }

protected:
    ClipId m_id;
    i64 m_startSample;
    i64 m_lengthSamples;
    std::string m_name;
    u32 m_color = 0x4A90D9;  // Default blue
    bool m_muted = false;
};

/**
 * @brief Audio clip referencing a sample/audio file
 */
class AudioClip : public Clip {
public:
    AudioClip(ClipId id, i64 startSample, i64 lengthSamples, u32 samplePoolId)
        : Clip(id, startSample, lengthSamples)
        , m_samplePoolId(samplePoolId)
    {}
    
    /**
     * @brief Get sample pool ID (reference to loaded audio)
     */
    [[nodiscard]] u32 samplePoolId() const noexcept { return m_samplePoolId; }
    
    /**
     * @brief Get offset into the source audio
     */
    [[nodiscard]] i64 sourceOffset() const noexcept { return m_sourceOffset; }
    void setSourceOffset(i64 offset) noexcept { m_sourceOffset = offset; }
    
    /**
     * @brief Get gain in dB
     */
    [[nodiscard]] f32 gain() const noexcept { return m_gainDb; }
    void setGain(f32 db) noexcept { m_gainDb = db; }
    
    /**
     * @brief Get fade in length in samples
     */
    [[nodiscard]] i64 fadeIn() const noexcept { return m_fadeInSamples; }
    void setFadeIn(i64 samples) noexcept { m_fadeInSamples = samples; }
    
    /**
     * @brief Get fade out length in samples
     */
    [[nodiscard]] i64 fadeOut() const noexcept { return m_fadeOutSamples; }
    void setFadeOut(i64 samples) noexcept { m_fadeOutSamples = samples; }
    
    /**
     * @brief Check if pitch shifting is enabled
     */
    [[nodiscard]] bool isPitchShifted() const noexcept { return m_pitchShift != 0.0f; }
    
    /**
     * @brief Get pitch shift in semitones
     */
    [[nodiscard]] f32 pitchShift() const noexcept { return m_pitchShift; }
    void setPitchShift(f32 semitones) noexcept { m_pitchShift = semitones; }
    
    /**
     * @brief Check if time stretching is enabled
     */
    [[nodiscard]] bool isTimeStretched() const noexcept { return m_stretchRatio != 1.0f; }
    
    /**
     * @brief Get time stretch ratio (1.0 = original speed)
     */
    [[nodiscard]] f64 stretchRatio() const noexcept { return m_stretchRatio; }
    void setStretchRatio(f64 ratio) noexcept { m_stretchRatio = ratio; }

private:
    u32 m_samplePoolId;           ///< Reference to audio in sample pool
    i64 m_sourceOffset = 0;       ///< Offset into source audio
    f32 m_gainDb = 0.0f;          ///< Clip gain
    i64 m_fadeInSamples = 0;      ///< Fade in length
    i64 m_fadeOutSamples = 0;     ///< Fade out length
    f32 m_pitchShift = 0.0f;      ///< Pitch shift in semitones
    f64 m_stretchRatio = 1.0;     ///< Time stretch ratio
};

/**
 * @brief Timeline track
 * 
 * A track contains clips and is associated with a mixer channel.
 * Tracks are displayed as lanes in the arrangement view.
 */
class Track {
public:
    explicit Track(TrackId id, TrackType type, std::string name)
        : m_id(id)
        , m_type(type)
        , m_name(std::move(name))
    {}
    
    //=========================================================================
    // Identity
    //=========================================================================
    
    [[nodiscard]] TrackId id() const noexcept { return m_id; }
    [[nodiscard]] TrackType type() const noexcept { return m_type; }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }
    
    [[nodiscard]] u32 color() const noexcept { return m_color; }
    void setColor(u32 color) noexcept { m_color = color; }
    
    //=========================================================================
    // Height & Visibility
    //=========================================================================
    
    [[nodiscard]] u32 height() const noexcept { return m_height; }
    void setHeight(u32 height) noexcept { m_height = height; }
    
    [[nodiscard]] bool isVisible() const noexcept { return m_visible; }
    void setVisible(bool visible) noexcept { m_visible = visible; }
    
    [[nodiscard]] bool isCollapsed() const noexcept { return m_collapsed; }
    void setCollapsed(bool collapsed) noexcept { m_collapsed = collapsed; }
    
    //=========================================================================
    // Mixer Channel Association
    //=========================================================================
    
    [[nodiscard]] ChannelId channelId() const noexcept { return m_channelId; }
    void setChannelId(ChannelId id) noexcept { m_channelId = id; }
    
    //=========================================================================
    // Clips
    //=========================================================================
    
    /**
     * @brief Add a clip to the track
     */
    void addClip(std::unique_ptr<Clip> clip) {
        m_clips.push_back(std::move(clip));
        sortClips();
    }
    
    /**
     * @brief Remove a clip by ID
     */
    bool removeClip(ClipId clipId) {
        auto it = std::find_if(m_clips.begin(), m_clips.end(),
            [clipId](const auto& clip) { return clip->id() == clipId; });
        if (it != m_clips.end()) {
            m_clips.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Get clip by ID
     */
    [[nodiscard]] Clip* getClip(ClipId clipId) {
        auto it = std::find_if(m_clips.begin(), m_clips.end(),
            [clipId](const auto& clip) { return clip->id() == clipId; });
        return it != m_clips.end() ? it->get() : nullptr;
    }
    
    /**
     * @brief Get all clips
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Clip>>& clips() const noexcept {
        return m_clips;
    }
    
    /**
     * @brief Get clips at a specific position
     */
    [[nodiscard]] std::vector<Clip*> clipsAt(i64 samplePos) {
        std::vector<Clip*> result;
        for (auto& clip : m_clips) {
            if (clip->contains(samplePos)) {
                result.push_back(clip.get());
            }
        }
        return result;
    }
    
    /**
     * @brief Get clips in a range
     */
    [[nodiscard]] std::vector<Clip*> clipsInRange(i64 startSample, i64 endSample) {
        std::vector<Clip*> result;
        for (auto& clip : m_clips) {
            if (clip->overlaps(startSample, endSample)) {
                result.push_back(clip.get());
            }
        }
        return result;
    }
    
    /**
     * @brief Get total track length (end of last clip)
     */
    [[nodiscard]] i64 length() const noexcept {
        i64 maxEnd = 0;
        for (const auto& clip : m_clips) {
            maxEnd = std::max(maxEnd, clip->endSample());
        }
        return maxEnd;
    }
    
    //=========================================================================
    // State
    //=========================================================================
    
    [[nodiscard]] bool isMuted() const noexcept { return m_muted; }
    void setMuted(bool muted) noexcept { m_muted = muted; }
    
    [[nodiscard]] bool isSoloed() const noexcept { return m_soloed; }
    void setSoloed(bool soloed) noexcept { m_soloed = soloed; }
    
    [[nodiscard]] bool isRecordArmed() const noexcept { return m_recordArmed; }
    void setRecordArmed(bool armed) noexcept { m_recordArmed = armed; }
    
    [[nodiscard]] bool isLocked() const noexcept { return m_locked; }
    void setLocked(bool locked) noexcept { m_locked = locked; }
    
    //=========================================================================
    // Folder Tracks
    //=========================================================================
    
    [[nodiscard]] TrackId parentId() const noexcept { return m_parentId; }
    void setParentId(TrackId id) noexcept { m_parentId = id; }

private:
    void sortClips() {
        std::sort(m_clips.begin(), m_clips.end(),
            [](const auto& a, const auto& b) {
                return a->startSample() < b->startSample();
            });
    }
    
    // Identity
    TrackId m_id;
    TrackType m_type;
    std::string m_name;
    u32 m_color = 0x4A90D9;
    
    // Display
    u32 m_height = 80;
    bool m_visible = true;
    bool m_collapsed = false;
    
    // Association
    ChannelId m_channelId = INVALID_CHANNEL_ID;
    TrackId m_parentId = INVALID_TRACK_ID;
    
    // Content
    std::vector<std::unique_ptr<Clip>> m_clips;
    
    // State
    bool m_muted = false;
    bool m_soloed = false;
    bool m_recordArmed = false;
    bool m_locked = false;
};

} // namespace nomad::audio
