#pragma once

#include <JuceHeader.h>
#include "MixerChannel.h"

/**
 * Main mixer class that manages all audio channels, routing, and mixing.
 */
class Mixer : public juce::AudioIODeviceCallback
{
public:
    Mixer();
    ~Mixer() override;
    
    // AudioIODeviceCallback interface
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                        int numInputChannels,
                                        float* const* outputChannelData,
                                        int numOutputChannels,
                                        int numSamples,
                                        const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    // Channel management
    MixerChannel* addChannel(const juce::String& name, MixerChannel::ChannelType type);
    void removeChannel(int index);
    MixerChannel* getChannel(int index) const;
    int getNumChannels() const;
    
    // Master bus control
    void setMasterGain(float newGain);
    float getMasterGain() const;
    
    // Solo/mute management
    void handleSoloStateChanged();
    
    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);
    
    // Access to master channel
    MixerChannel* getMasterChannel() { return masterChannel.get(); }
    
    // Get the internal audio buffer (for metering, etc.)
    const juce::AudioBuffer<float>& getInternalBuffer() const { return internalBuffer; }
    
private:
    juce::OwnedArray<MixerChannel> channels;
    std::unique_ptr<MixerChannel> masterChannel;
    
    // Internal processing buffer
    juce::AudioBuffer<float> internalBuffer;
    
    // Sample rate and buffer size
    double sampleRate = 44100.0;
    int bufferSize = 512;
    
    // Smoothing for master fader
    juce::SmoothedValue<float> masterGainSmoother;
    
    // Mutex for thread safety
    juce::CriticalSection lock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mixer)
};
