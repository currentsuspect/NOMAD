#pragma once

#include <JuceHeader.h>

/**
 * Main effects processor for NOMAD DAW.
 * Handles all audio effects processing in the signal chain.
 */
class EffectsProcessor
{
public:
    EffectsProcessor();
    ~EffectsProcessor() = default;

    // Process audio buffer with all active effects
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
    // Effect management
    void addEffect(std::unique_ptr<juce::AudioProcessor> effect);
    void removeEffect(int index);
    void moveEffect(int sourceIndex, int destIndex);
    
    // Global parameters
    void setSampleRate(double newSampleRate);
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void reset();
    
    // Preset management
    void savePreset(const juce::String& presetName);
    void loadPreset(const juce::String& presetName);
    
    // Bypass control
    void setBypassed(bool shouldBeBypassed);
    bool isBypassed() const;
    
private:
    juce::OwnedArray<juce::AudioProcessor> effectsChain;
    juce::CriticalSection effectLock;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool bypassed = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsProcessor)
};
