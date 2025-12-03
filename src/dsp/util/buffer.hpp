/**
 * @file buffer.hpp
 * @brief Audio buffer types and utilities
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides audio buffer types for DSP processing.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../../core/base/assert.hpp"
#include "../../core/memory/allocator.hpp"

#include <algorithm>
#include <cstring>

namespace nomad::dsp {

/**
 * @brief Non-interleaved audio buffer
 * 
 * Stores audio data with separate arrays for each channel.
 * This is the preferred format for DSP processing.
 */
class AudioBuffer {
public:
    /// Default constructor - creates empty buffer
    AudioBuffer() = default;

    /// Create buffer with specified channels and frames
    AudioBuffer(ChannelCount channels, FrameCount frames)
        : channels_(channels), frames_(frames) {
        if (channels > 0 && frames > 0) {
            allocate();
        }
    }

    /// Destructor
    ~AudioBuffer() {
        deallocate();
    }

    /// Move constructor
    AudioBuffer(AudioBuffer&& other) noexcept
        : data_(other.data_)
        , channels_(other.channels_)
        , frames_(other.frames_)
        , ownsMemory_(other.ownsMemory_) {
        other.data_ = nullptr;
        other.channels_ = 0;
        other.frames_ = 0;
        other.ownsMemory_ = false;
    }

    /// Move assignment
    AudioBuffer& operator=(AudioBuffer&& other) noexcept {
        if (this != &other) {
            deallocate();
            data_ = other.data_;
            channels_ = other.channels_;
            frames_ = other.frames_;
            ownsMemory_ = other.ownsMemory_;
            other.data_ = nullptr;
            other.channels_ = 0;
            other.frames_ = 0;
            other.ownsMemory_ = false;
        }
        return *this;
    }

    // Non-copyable (use copyFrom instead)
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    //-------------------------------------------------------------------------
    // Accessors
    //-------------------------------------------------------------------------

    /// Get pointer to channel data
    [[nodiscard]] Sample* getChannel(ChannelCount channel) {
        NOMAD_ASSERT(channel < channels_);
        return data_[channel];
    }

    /// Get const pointer to channel data
    [[nodiscard]] const Sample* getChannel(ChannelCount channel) const {
        NOMAD_ASSERT(channel < channels_);
        return data_[channel];
    }

    /// Get pointer to channel data (operator[])
    [[nodiscard]] Sample* operator[](ChannelCount channel) {
        return getChannel(channel);
    }

    /// Get const pointer to channel data (operator[])
    [[nodiscard]] const Sample* operator[](ChannelCount channel) const {
        return getChannel(channel);
    }

    /// Get sample at specific position
    [[nodiscard]] Sample& getSample(ChannelCount channel, FrameCount frame) {
        NOMAD_ASSERT(channel < channels_ && frame < frames_);
        return data_[channel][frame];
    }

    /// Get const sample at specific position
    [[nodiscard]] const Sample& getSample(ChannelCount channel, FrameCount frame) const {
        NOMAD_ASSERT(channel < channels_ && frame < frames_);
        return data_[channel][frame];
    }

    /// Get number of channels
    [[nodiscard]] ChannelCount getChannelCount() const { return channels_; }

    /// Get number of frames
    [[nodiscard]] FrameCount getFrameCount() const { return frames_; }

    /// Check if buffer is empty
    [[nodiscard]] bool isEmpty() const { return channels_ == 0 || frames_ == 0; }

    /// Get total sample count (channels * frames)
    [[nodiscard]] usize getTotalSamples() const {
        return static_cast<usize>(channels_) * static_cast<usize>(frames_);
    }

    /// Get raw channel pointer array
    [[nodiscard]] Sample** getData() { return data_; }
    [[nodiscard]] Sample* const* getData() const { return data_; }

    //-------------------------------------------------------------------------
    // Operations
    //-------------------------------------------------------------------------

    /// Clear all samples to zero
    void clear() {
        for (ChannelCount c = 0; c < channels_; ++c) {
            std::memset(data_[c], 0, frames_ * sizeof(Sample));
        }
    }

    /// Clear a specific channel
    void clearChannel(ChannelCount channel) {
        NOMAD_ASSERT(channel < channels_);
        std::memset(data_[channel], 0, frames_ * sizeof(Sample));
    }

    /// Copy from another buffer
    void copyFrom(const AudioBuffer& other) {
        const auto channelsToCopy = std::min(channels_, other.channels_);
        const auto framesToCopy = std::min(frames_, other.frames_);
        
        for (ChannelCount c = 0; c < channelsToCopy; ++c) {
            std::memcpy(data_[c], other.data_[c], framesToCopy * sizeof(Sample));
        }
    }

    /// Add samples from another buffer
    void addFrom(const AudioBuffer& other, Sample gain = 1.0f) {
        const auto channelsToCopy = std::min(channels_, other.channels_);
        const auto framesToCopy = std::min(frames_, other.frames_);
        
        for (ChannelCount c = 0; c < channelsToCopy; ++c) {
            for (FrameCount f = 0; f < framesToCopy; ++f) {
                data_[c][f] += other.data_[c][f] * gain;
            }
        }
    }

    /// Apply gain to all samples
    void applyGain(Sample gain) {
        for (ChannelCount c = 0; c < channels_; ++c) {
            for (FrameCount f = 0; f < frames_; ++f) {
                data_[c][f] *= gain;
            }
        }
    }

    /// Apply gain to a specific channel
    void applyGain(ChannelCount channel, Sample gain) {
        NOMAD_ASSERT(channel < channels_);
        for (FrameCount f = 0; f < frames_; ++f) {
            data_[channel][f] *= gain;
        }
    }

    /// Get peak amplitude across all channels
    [[nodiscard]] Sample getPeakAmplitude() const {
        Sample peak = 0.0f;
        for (ChannelCount c = 0; c < channels_; ++c) {
            for (FrameCount f = 0; f < frames_; ++f) {
                peak = std::max(peak, std::abs(data_[c][f]));
            }
        }
        return peak;
    }

    /// Resize buffer (reallocates if necessary)
    void resize(ChannelCount channels, FrameCount frames) {
        if (channels == channels_ && frames == frames_) return;
        
        deallocate();
        channels_ = channels;
        frames_ = frames;
        if (channels > 0 && frames > 0) {
            allocate();
        }
    }

    //-------------------------------------------------------------------------
    // Static Factory Methods
    //-------------------------------------------------------------------------

    /// Create a mono buffer
    static AudioBuffer mono(FrameCount frames) {
        return AudioBuffer(1, frames);
    }

    /// Create a stereo buffer
    static AudioBuffer stereo(FrameCount frames) {
        return AudioBuffer(2, frames);
    }

private:
    void allocate() {
        auto& allocator = memory::getAudioAllocator();
        
        // Allocate channel pointer array
        data_ = static_cast<Sample**>(
            allocator.allocate(channels_ * sizeof(Sample*), config::kAudioBlockAlignment)
        );
        
        // Allocate each channel
        for (ChannelCount c = 0; c < channels_; ++c) {
            data_[c] = static_cast<Sample*>(
                allocator.allocate(frames_ * sizeof(Sample), config::kAudioBlockAlignment)
            );
        }
        
        ownsMemory_ = true;
        clear();
    }

    void deallocate() {
        if (!ownsMemory_ || data_ == nullptr) return;
        
        auto& allocator = memory::getAudioAllocator();
        
        for (ChannelCount c = 0; c < channels_; ++c) {
            if (data_[c]) {
                allocator.deallocate(data_[c]);
            }
        }
        allocator.deallocate(data_);
        
        data_ = nullptr;
        ownsMemory_ = false;
    }

    Sample** data_ = nullptr;
    ChannelCount channels_ = 0;
    FrameCount frames_ = 0;
    bool ownsMemory_ = false;
};

/**
 * @brief Reference to a portion of an AudioBuffer
 */
class AudioBufferView {
public:
    AudioBufferView() = default;
    
    /// Create view of entire buffer
    explicit AudioBufferView(AudioBuffer& buffer)
        : data_(buffer.getData())
        , channels_(buffer.getChannelCount())
        , frames_(buffer.getFrameCount())
        , offset_(0) {}

    /// Create view of buffer portion
    AudioBufferView(AudioBuffer& buffer, FrameCount offset, FrameCount frames)
        : data_(buffer.getData())
        , channels_(buffer.getChannelCount())
        , frames_(frames)
        , offset_(offset) {
        NOMAD_ASSERT(offset + frames <= buffer.getFrameCount());
    }

    [[nodiscard]] Sample* getChannel(ChannelCount channel) const {
        NOMAD_ASSERT(channel < channels_);
        return data_[channel] + offset_;
    }

    [[nodiscard]] Sample* operator[](ChannelCount channel) const {
        return getChannel(channel);
    }

    [[nodiscard]] ChannelCount getChannelCount() const { return channels_; }
    [[nodiscard]] FrameCount getFrameCount() const { return frames_; }

private:
    Sample** data_ = nullptr;
    ChannelCount channels_ = 0;
    FrameCount frames_ = 0;
    FrameCount offset_ = 0;
};

} // namespace nomad::dsp
