/**
 * @file types.hpp
 * @brief Core type definitions for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides fundamental type definitions used throughout the codebase.
 * All modules depend on these definitions for consistent data representation.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace nomad {

//=============================================================================
// Fixed-width integer types
//=============================================================================

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

//=============================================================================
// Floating point types
//=============================================================================

using f32 = float;
using f64 = double;

static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

//=============================================================================
// Size types
//=============================================================================

using usize = std::size_t;
using isize = std::ptrdiff_t;

//=============================================================================
// Audio-specific types
//=============================================================================

/// Sample rate in Hz (e.g., 44100, 48000, 96000)
using SampleRate = u32;

/// Number of audio frames (samples per channel)
using FrameCount = u64;

/// Buffer size in frames
using BufferSize = u32;

/// Channel count
using ChannelCount = u32;

/// Audio sample value (normalized -1.0 to 1.0)
using Sample = f32;

/// High-precision sample for internal processing
using SamplePrecise = f64;

/// Time in samples (for sample-accurate timing)
using SampleTime = i64;

/// Time in seconds (for display and user interaction)
using Seconds = f64;

/// Time in milliseconds
using Milliseconds = f64;

/// Beats per minute
using BPM = f64;

/// Musical beat position (for sequencing)
using BeatPosition = f64;

//=============================================================================
// MIDI types
//=============================================================================

/// MIDI note number (0-127)
using MidiNote = u8;

/// MIDI velocity (0-127)
using MidiVelocity = u8;

/// MIDI channel (0-15)
using MidiChannel = u8;

/// MIDI control change number (0-127)
using MidiCC = u8;

/// MIDI program number (0-127)
using MidiProgram = u8;

//=============================================================================
// UI types
//=============================================================================

/// Pixel coordinate (screen space)
using Pixel = i32;

/// Normalized coordinate (0.0 to 1.0)
using Normalized = f32;

/// Color component (0-255)
using ColorComponent = u8;

/// RGBA color packed into 32 bits
using PackedColor = u32;

//=============================================================================
// Utility type traits
//=============================================================================

/// Type trait to check if a type is an audio sample type
template<typename T>
struct is_sample_type : std::disjunction<
    std::is_same<T, Sample>,
    std::is_same<T, SamplePrecise>
> {};

template<typename T>
inline constexpr bool is_sample_type_v = is_sample_type<T>::value;

/// Type trait to check if a type is a fixed-width integer
template<typename T>
struct is_fixed_integer : std::disjunction<
    std::is_same<T, i8>, std::is_same<T, i16>, std::is_same<T, i32>, std::is_same<T, i64>,
    std::is_same<T, u8>, std::is_same<T, u16>, std::is_same<T, u32>, std::is_same<T, u64>
> {};

template<typename T>
inline constexpr bool is_fixed_integer_v = is_fixed_integer<T>::value;

//=============================================================================
// Constants
//=============================================================================

namespace constants {

/// Standard sample rates
inline constexpr SampleRate kSampleRate44100  = 44100;
inline constexpr SampleRate kSampleRate48000  = 48000;
inline constexpr SampleRate kSampleRate88200  = 88200;
inline constexpr SampleRate kSampleRate96000  = 96000;
inline constexpr SampleRate kSampleRate176400 = 176400;
inline constexpr SampleRate kSampleRate192000 = 192000;

/// Common buffer sizes
inline constexpr BufferSize kBufferSize32   = 32;
inline constexpr BufferSize kBufferSize64   = 64;
inline constexpr BufferSize kBufferSize128  = 128;
inline constexpr BufferSize kBufferSize256  = 256;
inline constexpr BufferSize kBufferSize512  = 512;
inline constexpr BufferSize kBufferSize1024 = 1024;
inline constexpr BufferSize kBufferSize2048 = 2048;

/// Audio constants
inline constexpr Sample kSampleMin = -1.0f;
inline constexpr Sample kSampleMax =  1.0f;
inline constexpr Sample kSilence   =  0.0f;

/// MIDI constants
inline constexpr MidiNote kMidiNoteMin = 0;
inline constexpr MidiNote kMidiNoteMax = 127;
inline constexpr MidiNote kMiddleC     = 60;

inline constexpr MidiVelocity kMidiVelocityMin = 0;
inline constexpr MidiVelocity kMidiVelocityMax = 127;

inline constexpr MidiChannel kMidiChannelMin = 0;
inline constexpr MidiChannel kMidiChannelMax = 15;

/// Tempo constants
inline constexpr BPM kBPMMin     = 20.0;
inline constexpr BPM kBPMMax     = 999.0;
inline constexpr BPM kBPMDefault = 120.0;

} // namespace constants

} // namespace nomad
