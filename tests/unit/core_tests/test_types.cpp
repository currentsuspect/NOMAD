/**
 * @file test_types.cpp
 * @brief Unit tests for core type definitions
 * @author Nomad Team
 * @date 2025
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/base/types.hpp"
#include "core/base/config.hpp"

using namespace nomad;

TEST_CASE("Core types have correct sizes", "[core][types]") {
    SECTION("Fixed-width integers") {
        REQUIRE(sizeof(i8)  == 1);
        REQUIRE(sizeof(i16) == 2);
        REQUIRE(sizeof(i32) == 4);
        REQUIRE(sizeof(i64) == 8);
        
        REQUIRE(sizeof(u8)  == 1);
        REQUIRE(sizeof(u16) == 2);
        REQUIRE(sizeof(u32) == 4);
        REQUIRE(sizeof(u64) == 8);
    }
    
    SECTION("Floating point types") {
        REQUIRE(sizeof(f32) == 4);
        REQUIRE(sizeof(f64) == 8);
    }
    
    SECTION("Audio types") {
        REQUIRE(sizeof(Sample) == 4);
        REQUIRE(sizeof(SamplePrecise) == 8);
        REQUIRE(sizeof(SampleRate) == 4);
        REQUIRE(sizeof(FrameCount) == 8);
        REQUIRE(sizeof(BufferSize) == 4);
    }
    
    SECTION("MIDI types") {
        REQUIRE(sizeof(MidiNote) == 1);
        REQUIRE(sizeof(MidiVelocity) == 1);
        REQUIRE(sizeof(MidiChannel) == 1);
    }
}

TEST_CASE("Audio constants are valid", "[core][types]") {
    using namespace constants;
    
    SECTION("Sample rates") {
        REQUIRE(kSampleRate44100 == 44100);
        REQUIRE(kSampleRate48000 == 48000);
        REQUIRE(kSampleRate96000 == 96000);
        REQUIRE(kSampleRate192000 == 192000);
    }
    
    SECTION("Buffer sizes") {
        REQUIRE(kBufferSize64 == 64);
        REQUIRE(kBufferSize128 == 128);
        REQUIRE(kBufferSize256 == 256);
        REQUIRE(kBufferSize512 == 512);
        REQUIRE(kBufferSize1024 == 1024);
    }
    
    SECTION("Sample bounds") {
        REQUIRE(kSampleMin == -1.0f);
        REQUIRE(kSampleMax == 1.0f);
        REQUIRE(kSilence == 0.0f);
    }
    
    SECTION("MIDI constants") {
        REQUIRE(kMidiNoteMin == 0);
        REQUIRE(kMidiNoteMax == 127);
        REQUIRE(kMiddleC == 60);
        
        REQUIRE(kMidiVelocityMin == 0);
        REQUIRE(kMidiVelocityMax == 127);
        
        REQUIRE(kMidiChannelMin == 0);
        REQUIRE(kMidiChannelMax == 15);
    }
    
    SECTION("Tempo constants") {
        REQUIRE(kBPMMin == 20.0);
        REQUIRE(kBPMMax == 999.0);
        REQUIRE(kBPMDefault == 120.0);
    }
}

TEST_CASE("Type traits work correctly", "[core][types]") {
    SECTION("is_sample_type") {
        REQUIRE(is_sample_type_v<Sample>);
        REQUIRE(is_sample_type_v<SamplePrecise>);
        REQUIRE_FALSE(is_sample_type_v<int>);
        REQUIRE_FALSE(is_sample_type_v<i32>);
    }
    
    SECTION("is_fixed_integer") {
        REQUIRE(is_fixed_integer_v<i8>);
        REQUIRE(is_fixed_integer_v<i16>);
        REQUIRE(is_fixed_integer_v<i32>);
        REQUIRE(is_fixed_integer_v<i64>);
        REQUIRE(is_fixed_integer_v<u8>);
        REQUIRE(is_fixed_integer_v<u16>);
        REQUIRE(is_fixed_integer_v<u32>);
        REQUIRE(is_fixed_integer_v<u64>);
        REQUIRE_FALSE(is_fixed_integer_v<float>);
        REQUIRE_FALSE(is_fixed_integer_v<double>);
    }
}

TEST_CASE("Build configuration is correct", "[core][config]") {
    SECTION("Version info") {
        REQUIRE(config::kVersionMajor == 0);
        REQUIRE(config::kVersionMinor == 1);
        REQUIRE(config::kVersionPatch == 0);
        REQUIRE(std::string(config::kVersionString) == "0.1.0-alpha");
    }
    
    SECTION("Audio limits") {
        REQUIRE(config::kMaxAudioChannels >= 2);
        REQUIRE(config::kMaxTracks >= 100);
        REQUIRE(config::kMaxPluginsPerTrack >= 8);
    }
    
    SECTION("Memory config") {
        REQUIRE(config::kDefaultRTPoolSize > 0);
        REQUIRE(config::kAudioBlockAlignment >= 16);
    }
}
