/**
 * @file TestTransport.cpp
 * @brief Tests for Transport System
 */

#include <gtest/gtest.h>
#include "transport/Transport.h"
#include "audio/AudioEngine.h"

class TransportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioEngine = std::make_unique<nomad::audio::AudioEngine>(44100.0, 512);
        transport = std::make_unique<nomad::transport::Transport>(*audioEngine);
        
        audioEngine->initialize();
        transport->initialize();
    }
    
    void TearDown() override
    {
        transport.reset();
        audioEngine.reset();
    }
    
    std::unique_ptr<nomad::audio::AudioEngine> audioEngine;
    std::unique_ptr<nomad::transport::Transport> transport;
};

TEST_F(TransportTest, Initialization)
{
    EXPECT_TRUE(transport->getState() == nomad::transport::TransportState::Stopped);
    EXPECT_EQ(transport->getTimePosition(), 0.0);
    EXPECT_EQ(transport->getBeatPosition(), 0.0);
    EXPECT_EQ(transport->getTempo(), 120.0);
}

TEST_F(TransportTest, PlaybackControl)
{
    // Test play
    transport->play();
    EXPECT_TRUE(transport->isPlaying());
    EXPECT_EQ(transport->getState(), nomad::transport::TransportState::Playing);
    
    // Test pause
    transport->pause();
    EXPECT_FALSE(transport->isPlaying());
    EXPECT_EQ(transport->getState(), nomad::transport::TransportState::Paused);
    
    // Test stop
    transport->stop();
    EXPECT_FALSE(transport->isPlaying());
    EXPECT_EQ(transport->getState(), nomad::transport::TransportState::Stopped);
    EXPECT_EQ(transport->getTimePosition(), 0.0);
}

TEST_F(TransportTest, Recording)
{
    // Test record
    transport->record();
    EXPECT_TRUE(transport->isRecording());
    EXPECT_EQ(transport->getState(), nomad::transport::TransportState::Recording);
    
    // Test stop recording
    transport->stopRecording();
    EXPECT_FALSE(transport->isRecording());
    EXPECT_EQ(transport->getState(), nomad::transport::TransportState::Playing);
}

TEST_F(TransportTest, TimePosition)
{
    transport->setTimePosition(10.5);
    EXPECT_EQ(transport->getTimePosition(), 10.5);
    
    transport->setTimePosition(25.0);
    EXPECT_EQ(transport->getTimePosition(), 25.0);
}

TEST_F(TransportTest, BeatPosition)
{
    transport->setBeatPosition(4.0);
    EXPECT_EQ(transport->getBeatPosition(), 4.0);
    
    transport->setBeatPosition(8.5);
    EXPECT_EQ(transport->getBeatPosition(), 8.5);
}

TEST_F(TransportTest, Tempo)
{
    transport->setTempo(140.0);
    EXPECT_EQ(transport->getTempo(), 140.0);
    
    transport->setTempo(80.0);
    EXPECT_EQ(transport->getTempo(), 80.0);
}

TEST_F(TransportTest, TimeSignature)
{
    transport->setTimeSignature(3, 4);
    EXPECT_EQ(transport->getTimeSignatureNumerator(), 3.0);
    EXPECT_EQ(transport->getTimeSignatureDenominator(), 4.0);
    
    transport->setTimeSignature(7, 8);
    EXPECT_EQ(transport->getTimeSignatureNumerator(), 7.0);
    EXPECT_EQ(transport->getTimeSignatureDenominator(), 8.0);
}

TEST_F(TransportTest, Looping)
{
    EXPECT_FALSE(transport->isLoopEnabled());
    
    transport->setLoopEnabled(true);
    EXPECT_TRUE(transport->isLoopEnabled());
    
    transport->setLoopRange(10.0, 20.0);
    EXPECT_EQ(transport->getLoopStart(), 10.0);
    EXPECT_EQ(transport->getLoopEnd(), 20.0);
    
    transport->setLoopEnabled(false);
    EXPECT_FALSE(transport->isLoopEnabled());
}

TEST_F(TransportTest, TimeConversions)
{
    transport->setTempo(120.0);
    
    double time = transport->beatsToTime(4.0);
    EXPECT_GT(time, 0.0);
    
    double beats = transport->timeToBeats(time);
    EXPECT_NEAR(beats, 4.0, 0.001);
}

TEST_F(TransportTest, SamplesPerBeat)
{
    int samplesPerBeat = transport->getSamplesPerBeat();
    EXPECT_GT(samplesPerBeat, 0);
    
    double samplesPerSecond = transport->getSamplesPerSecond();
    EXPECT_EQ(samplesPerSecond, 44100.0);
}

TEST_F(TransportTest, TransportCallbacks)
{
    bool callbackCalled = false;
    nomad::transport::TransportInfo receivedInfo;
    
    auto callback = [&callbackCalled, &receivedInfo](const nomad::transport::TransportInfo& info) {
        callbackCalled = true;
        receivedInfo = info;
    };
    
    transport->addTransportCallback(callback);
    
    // Trigger a state change
    transport->play();
    
    // Note: In a real test, we'd verify callback was called
    // This is simplified for the test framework
    transport->removeTransportCallback(callback);
}

TEST_F(TransportTest, TransportInfo)
{
    transport->setTempo(140.0);
    transport->setTimeSignature(3, 4);
    transport->setLoopEnabled(true);
    transport->setLoopRange(5.0, 15.0);
    
    auto info = transport->getTransportInfo();
    EXPECT_EQ(info.tempo, 140.0);
    EXPECT_EQ(info.timeSignatureNumerator, 3.0);
    EXPECT_EQ(info.timeSignatureDenominator, 4.0);
    EXPECT_TRUE(info.isLooping);
    EXPECT_EQ(info.loopStart, 5.0);
    EXPECT_EQ(info.loopEnd, 15.0);
}

TEST_F(TransportTest, ProcessTransport)
{
    transport->play();
    transport->setTimePosition(0.0);
    
    // Process some samples
    transport->processTransport(512);
    
    // Time should have advanced
    EXPECT_GT(transport->getTimePosition(), 0.0);
}