#pragma once

#include <JuceHeader.h>
#include "EffectsProcessor.h"

class MixerChannel : public juce::AudioProcessorParameter::Listener
{
public:
    enum class ChannelType
    {
        Audio,      // For audio tracks
        Instrument, // For virtual instruments
        Return,     // For effects returns
        Master,     // Master output
        Group       // For grouping channels
    };

    MixerChannel(const juce::String& name, ChannelType type, double sampleRate = 44100.0, int bufferSize = 512);
    ~MixerChannel() override;

    // Audio processing
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    
    // Audio source management
    void setAudioSource(juce::AudioSource* source);
    juce::AudioSource* getAudioSource() const;
    
    // Channel configuration
    void setGain(float newGain);
    float getGain() const;
    
    void setPan(float newPan); // -1.0 (full left) to 1.0 (full right)
    float getPan() const;
    
    void setSolo(bool shouldBeSolo);
    bool isSolo() const;
    
    void setMute(bool shouldBeMuted);
    bool isMuted() const;
    
    void setSoloMute(bool shouldBeSoloMuted);
    bool isSoloMuted() const;
    
    // Effects
    EffectsProcessor& getEffectsProcessor() { return effectsProcessor; }
    
    // Metering
    float getPeakLevel() const;
    float getRMSLevel() const;
    
    // Parameter handling (for automation)
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
    
    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);
    
private:
    const juce::String name;
    const ChannelType type;
    
    // Audio parameters
    std::atomic<float> gain { 0.8f };  // 0.0 to 1.0
    std::atomic<float> pan { 0.0f };    // -1.0 to 1.0
    std::atomic<bool> solo { false };
    std::atomic<bool> userMute { false };  // User-set mute state
    std::atomic<bool> soloMute { false };  // Mute due to solo on other channel
    
    // Effects processing
    EffectsProcessor effectsProcessor;
    
    // Metering
    std::atomic<float> peakLevel { 0.0f };
    std::atomic<float> rmsLevel { 0.0f };
    float peakHoldTime = 0.0f;
    static constexpr float peakHoldDuration = 1.5f; // seconds
    
    // Smoothing for parameter changes
    juce::SmoothedValue<float> gainSmoother;
    juce::SmoothedValue<float> panSmoother;
    
    // Sample rate tracking for metering decay
    double currentSampleRate = 44100.0;
    
    // Audio source
    juce::AudioSource* audioSource = nullptr;
    juce::AudioBuffer<float> sourceBuffer;
    
    // Metering helper
    void updateMetering(const juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};
