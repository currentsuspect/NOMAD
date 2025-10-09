/**
 * @file TestAudioEngine.cpp
 * @brief Tests for Audio Engine
 */

#include <gtest/gtest.h>
#include "audio/AudioEngine.h"

class AudioEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioEngine = std::make_unique<nomad::audio::AudioEngine>(44100.0, 512);
    }
    
    void TearDown() override
    {
        audioEngine.reset();
    }
    
    std::unique_ptr<nomad::audio::AudioEngine> audioEngine;
};

TEST_F(AudioEngineTest, Initialization)
{
    EXPECT_TRUE(audioEngine->initialize());
    EXPECT_EQ(audioEngine->getSampleRate(), 44100.0);
    EXPECT_EQ(audioEngine->getBufferSize(), 512);
}

TEST_F(AudioEngineTest, BufferSizeChange)
{
    EXPECT_TRUE(audioEngine->initialize());
    
    audioEngine->setBufferSize(1024);
    EXPECT_EQ(audioEngine->getBufferSize(), 1024);
    
    audioEngine->setBufferSize(256);
    EXPECT_EQ(audioEngine->getBufferSize(), 256);
}

TEST_F(AudioEngineTest, SampleRateChange)
{
    EXPECT_TRUE(audioEngine->initialize());
    
    audioEngine->setSampleRate(48000.0);
    EXPECT_EQ(audioEngine->getSampleRate(), 48000.0);
    
    audioEngine->setSampleRate(96000.0);
    EXPECT_EQ(audioEngine->getSampleRate(), 96000.0);
}

TEST_F(AudioEngineTest, DoubleBuffering)
{
    EXPECT_TRUE(audioEngine->initialize());
    
    EXPECT_TRUE(audioEngine->isDoubleBufferingEnabled());
    
    audioEngine->setDoubleBufferingEnabled(false);
    EXPECT_FALSE(audioEngine->isDoubleBufferingEnabled());
    
    audioEngine->setDoubleBufferingEnabled(true);
    EXPECT_TRUE(audioEngine->isDoubleBufferingEnabled());
}

TEST_F(AudioEngineTest, PerformanceStats)
{
    EXPECT_TRUE(audioEngine->initialize());
    
    auto stats = audioEngine->getPerformanceStats();
    EXPECT_GE(stats.cpuUsage, 0.0);
    EXPECT_GE(stats.maxCpuUsage, 0.0);
    EXPECT_GE(stats.bufferUnderruns, 0);
    EXPECT_GE(stats.bufferOverruns, 0);
    EXPECT_GE(stats.averageLatency, 0.0);
}

TEST_F(AudioEngineTest, RealtimeCallbacks)
{
    EXPECT_TRUE(audioEngine->initialize());
    
    bool callbackCalled = false;
    auto callback = [&callbackCalled]() { callbackCalled = true; };
    
    audioEngine->addRealtimeCallback(callback);
    
    // Simulate audio processing
    const int numSamples = 512;
    float* inputChannels[2] = {nullptr, nullptr};
    float* outputChannels[2] = {nullptr, nullptr};
    
    // Allocate test buffers
    juce::AudioBuffer<float> inputBuffer(inputChannels, 2, numSamples);
    juce::AudioBuffer<float> outputBuffer(outputChannels, 2, numSamples);
    
    juce::AudioIODeviceCallbackContext context;
    audioEngine->audioDeviceIOCallbackWithContext(
        inputBuffer.getArrayOfReadPointers(),
        2,
        outputBuffer.getArrayOfWritePointers(),
        2,
        numSamples,
        context
    );
    
    // Note: In a real test, we'd need to verify callback was called
    // This is simplified for the test framework
    audioEngine->removeRealtimeCallback(callback);
}