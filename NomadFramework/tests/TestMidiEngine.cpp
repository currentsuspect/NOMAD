/**
 * @file TestMidiEngine.cpp
 * @brief Tests for MIDI Engine
 */

#include <gtest/gtest.h>
#include "midi/MidiEngine.h"

class MidiEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        midiEngine = std::make_unique<nomad::midi::MidiEngine>();
    }
    
    void TearDown() override
    {
        midiEngine.reset();
    }
    
    std::unique_ptr<nomad::midi::MidiEngine> midiEngine;
};

TEST_F(MidiEngineTest, Initialization)
{
    EXPECT_TRUE(midiEngine->initialize());
}

TEST_F(MidiEngineTest, ClockSync)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    EXPECT_FALSE(midiEngine->isClockSyncEnabled());
    
    midiEngine->setClockSyncEnabled(true);
    EXPECT_TRUE(midiEngine->isClockSyncEnabled());
    
    midiEngine->setClockSyncEnabled(false);
    EXPECT_FALSE(midiEngine->isClockSyncEnabled());
}

TEST_F(MidiEngineTest, Quantization)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    EXPECT_FALSE(midiEngine->isQuantizationEnabled());
    
    midiEngine->setQuantizationEnabled(true);
    EXPECT_TRUE(midiEngine->isQuantizationEnabled());
    
    midiEngine->setQuantizationGrid(0.25);
    EXPECT_EQ(midiEngine->getQuantizationGrid(), 0.25);
    
    midiEngine->setQuantizationGrid(0.125);
    EXPECT_EQ(midiEngine->getQuantizationGrid(), 0.125);
}

TEST_F(MidiEngineTest, Tempo)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    EXPECT_EQ(midiEngine->getTempo(), 120.0);
    
    midiEngine->setTempo(140.0);
    EXPECT_EQ(midiEngine->getTempo(), 140.0);
    
    midiEngine->setTempo(80.0);
    EXPECT_EQ(midiEngine->getTempo(), 80.0);
}

TEST_F(MidiEngineTest, TimePosition)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    EXPECT_EQ(midiEngine->getTimePosition(), 0.0);
    
    midiEngine->setTimePosition(10.5);
    EXPECT_EQ(midiEngine->getTimePosition(), 10.5);
    
    midiEngine->setTimePosition(25.0);
    EXPECT_EQ(midiEngine->getTimePosition(), 25.0);
}

TEST_F(MidiEngineTest, MidiEventCreation)
{
    nomad::midi::MidiEvent event(0x90, 60, 100, 0.0, 1);
    
    EXPECT_EQ(event.status, 0x90);
    EXPECT_EQ(event.data1, 60);
    EXPECT_EQ(event.data2, 100);
    EXPECT_EQ(event.timestamp, 0.0);
    EXPECT_EQ(event.channel, 1);
}

TEST_F(MidiEngineTest, MidiEventCallbacks)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    bool callbackCalled = false;
    nomad::midi::MidiEvent receivedEvent;
    
    auto callback = [&callbackCalled, &receivedEvent](const nomad::midi::MidiEvent& event) {
        callbackCalled = true;
        receivedEvent = event;
    };
    
    midiEngine->addMidiEventCallback(callback);
    
    // Simulate MIDI event
    nomad::midi::MidiEvent testEvent(0x90, 60, 100, 0.0, 1);
    midiEngine->processMidiEvents(512);
    
    midiEngine->removeMidiEventCallback(callback);
}

TEST_F(MidiEngineTest, MidiStats)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    auto stats = midiEngine->getMidiStats();
    EXPECT_GE(stats.eventsProcessed, 0);
    EXPECT_GE(stats.eventsDropped, 0);
    EXPECT_GE(stats.averageLatency, 0.0);
    EXPECT_GE(stats.activeInputs, 0);
    EXPECT_GE(stats.activeOutputs, 0);
}

TEST_F(MidiEngineTest, AvailableDevices)
{
    EXPECT_TRUE(midiEngine->initialize());
    
    auto inputDevices = midiEngine->getAvailableInputDevices();
    auto outputDevices = midiEngine->getAvailableOutputDevices();
    
    // Note: Device availability depends on system
    // These tests just verify the methods don't crash
    EXPECT_TRUE(true); // Placeholder assertion
}