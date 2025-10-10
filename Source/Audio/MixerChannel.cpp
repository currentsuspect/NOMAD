#include "MixerChannel.h"

MixerChannel::MixerChannel(const juce::String& name, ChannelType type, double sampleRate, int bufferSize)
    : name(name), type(type)
    , gainSmoother(0.8f)
    , panSmoother(0.0f)
{
    prepareToPlay(sampleRate, bufferSize);
}

MixerChannel::~MixerChannel()
{
}

void MixerChannel::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    gainSmoother.reset(sampleRate, 0.05); // 50ms smoothing
    panSmoother.reset(sampleRate, 0.05);  // 50ms smoothing
    
    effectsProcessor.prepareToPlay(samplesPerBlock, sampleRate);
    
    // Prepare audio source if present
    if (audioSource != nullptr)
    {
        audioSource->prepareToPlay(samplesPerBlock, sampleRate);
    }
    
    // Allocate source buffer for rendering audio source
    sourceBuffer.setSize(2, samplesPerBlock);
}

void MixerChannel::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Check both user mute and solo-induced mute
    if (userMute.load() || soloMute.load())
    {
        buffer.clear();
        return;
    }
    
    // Render audio from source if present
    if (audioSource != nullptr)
    {
        // Ensure source buffer is the right size
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (sourceBuffer.getNumSamples() != numSamples || sourceBuffer.getNumChannels() != numChannels)
        {
            sourceBuffer.setSize(numChannels, numSamples, false, false, true);
        }
        
        sourceBuffer.clear();
        
        // Get audio from the source
        juce::AudioSourceChannelInfo info;
        info.buffer = &sourceBuffer;
        info.startSample = 0;
        info.numSamples = numSamples;
        
        audioSource->getNextAudioBlock(info);
        
        // Copy source audio to the buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.copyFrom(ch, 0, sourceBuffer, ch, 0, numSamples);
        }
    }
    
    // Process effects
    effectsProcessor.processBlock(buffer, midiMessages);
    
    // Apply gain with smoothing
    const auto targetGain = gain.load();
    const auto targetPan = pan.load();
    
    if (gainSmoother.getTargetValue() != targetGain)
        gainSmoother.setTargetValue(targetGain);
        
    if (panSmoother.getTargetValue() != targetPan)
        panSmoother.setTargetValue(targetPan);
    
    // Apply gain and pan
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    if (numChannels == 1)
    {
        // Mono buffer - just apply gain without panning
        float* channelData = buffer.getWritePointer(0);
        
        for (int i = 0; i < numSamples; ++i)
        {
            const float gainValue = gainSmoother.getNextValue();
            // Advance pan smoother to keep it in sync, but don't use the value
            panSmoother.getNextValue();
            
            channelData[i] *= gainValue;
        }
    }
    else if (numChannels >= 2)
    {
        // Stereo buffer - apply proper pan law
        float* leftChannel = buffer.getWritePointer(0);
        float* rightChannel = buffer.getWritePointer(1);
        
        for (int i = 0; i < numSamples; ++i)
        {
            const float gainValue = gainSmoother.getNextValue();
            const float panValue = panSmoother.getNextValue();
            
            // Linear pan law: pan -1.0 = full left, 0.0 = center, 1.0 = full right
            const float leftGain = (panValue <= 0.0f) ? 1.0f : (1.0f - panValue);
            const float rightGain = (panValue >= 0.0f) ? 1.0f : (1.0f + panValue);
            
            leftChannel[i] *= gainValue * leftGain;
            rightChannel[i] *= gainValue * rightGain;
        }
        
        // For additional channels beyond stereo, just apply gain
        for (int ch = 2; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                channelData[i] *= targetGain;
            }
        }
    }
    
    // Update metering
    updateMetering(buffer);
}

void MixerChannel::setGain(float newGain)
{
    gain.store(juce::jlimit(0.0f, 1.0f, newGain));
}

float MixerChannel::getGain() const
{
    return gain.load();
}

void MixerChannel::setPan(float newPan)
{
    pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
}

float MixerChannel::getPan() const
{
    return pan.load();
}

void MixerChannel::setSolo(bool shouldBeSolo)
{
    solo.store(shouldBeSolo);
}

bool MixerChannel::isSolo() const
{
    return solo.load();
}

void MixerChannel::setMute(bool shouldBeMuted)
{
    userMute.store(shouldBeMuted);
}

bool MixerChannel::isMuted() const
{
    return userMute.load();
}

void MixerChannel::setSoloMute(bool shouldBeSoloMuted)
{
    soloMute.store(shouldBeSoloMuted);
}

bool MixerChannel::isSoloMuted() const
{
    return soloMute.load();
}

float MixerChannel::getPeakLevel() const
{
    return juce::Decibels::gainToDecibels(peakLevel.load());
}

float MixerChannel::getRMSLevel() const
{
    return juce::Decibels::gainToDecibels(rmsLevel.load());
}

void MixerChannel::parameterValueChanged(int parameterIndex, float newValue)
{
    // Handle automation here
    juce::ignoreUnused(parameterIndex, newValue);
}

void MixerChannel::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
    juce::ignoreUnused(parameterIndex, gestureIsStarting);
}

juce::ValueTree MixerChannel::getState() const
{
    juce::ValueTree state("MIXERCHANNEL");
    state.setProperty("name", name, nullptr);
    state.setProperty("gain", gain.load(), nullptr);
    state.setProperty("pan", pan.load(), nullptr);
    state.setProperty("solo", solo.load(), nullptr);
    state.setProperty("mute", userMute.load(), nullptr);  // Only save user mute, not solo mute
    
    // Save effects state
    // TODO: Implement effects state saving
    
    return state;
}

void MixerChannel::setState(const juce::ValueTree& state)
{
    if (state.hasType("MIXERCHANNEL"))
    {
        gain = state.getProperty("gain", 0.8f);
        pan = state.getProperty("pan", 0.0f);
        solo = state.getProperty("solo", false);
        userMute = state.getProperty("mute", false);  // Load user mute only
        soloMute = false;  // Reset solo mute on load
        
        // Load effects state
        // TODO: Implement effects state loading
    }
}

void MixerChannel::setAudioSource(juce::AudioSource* source)
{
    audioSource = source;
    
    // If we have a valid source and we're already prepared, prepare the source
    if (audioSource != nullptr && currentSampleRate > 0.0)
    {
        audioSource->prepareToPlay(sourceBuffer.getNumSamples(), currentSampleRate);
    }
}

juce::AudioSource* MixerChannel::getAudioSource() const
{
    return audioSource;
}

void MixerChannel::updateMetering(const juce::AudioBuffer<float>& buffer)
{
    // Simple peak and RMS metering
    float peak = 0.0f;
    float sumOfSquares = 0.0f;
    int numSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float sample = std::abs(channelData[i]);
            peak = std::max(peak, sample);
            sumOfSquares += sample * sample;
            numSamples++;
        }
    }
    
    // Update peak level with hold
    const float currentPeak = peakLevel.load();
    if (peak > currentPeak || peakHoldTime <= 0.0f)
    {
        peakLevel.store(peak);
        peakHoldTime = peakHoldDuration;
    }
    else
    {
        // Decay the peak over time
        peakHoldTime -= buffer.getNumSamples() / static_cast<float>(currentSampleRate);
        peakLevel.store(std::max(peak, currentPeak * 0.999f));
    }
    
    // Update RMS level
    if (numSamples > 0)
    {
        const float rms = std::sqrt(sumOfSquares / numSamples);
        rmsLevel.store(rms);
    }
}
