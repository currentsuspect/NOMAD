#include "Mixer.h"

Mixer::Mixer()
    : masterGainSmoother(0.8f)
{
    // Create master channel
    masterChannel = std::make_unique<MixerChannel>("Master", MixerChannel::ChannelType::Master);
}

Mixer::~Mixer()
{
    // Ensure all channels are cleared before destruction
    channels.clear();
}

void Mixer::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);
    
    // Clear internal buffer before summing channels
    internalBuffer.clear();
    
    // Process all channels and sum their outputs
    for (auto* channel : channels)
    {
        if (channel != nullptr)
        {
            // Create a separate buffer for this channel's output
            juce::AudioBuffer<float> channelBuffer(internalBuffer.getNumChannels(), numSamples);
            channelBuffer.clear();
            
            // TODO: In the future, each channel will get audio from its own source
            // For now, channels process silence (or could copy from input if needed)
            
            // Process the channel (applies effects, gain, pan, etc.)
            juce::MidiBuffer midiBuffer; // Empty MIDI buffer for now
            channel->processBlock(channelBuffer, midiBuffer);
            
            // Sum this channel's output to the mix bus
            for (int ch = 0; ch < internalBuffer.getNumChannels(); ++ch)
            {
                internalBuffer.addFrom(ch, 0, channelBuffer, ch, 0, numSamples);
            }
        }
    }
    
    // Apply master channel processing
    if (masterChannel != nullptr)
    {
        juce::MidiBuffer midiBuffer; // Empty MIDI buffer for master
        masterChannel->processBlock(internalBuffer, midiBuffer);
    }
    
    // Apply master gain with per-sample smoothing
    for (int ch = 0; ch < internalBuffer.getNumChannels(); ++ch)
    {
        float* channelData = internalBuffer.getWritePointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] *= masterGainSmoother.getNextValue();
        }
    }
    
    // Copy to output
    for (int ch = 0; ch < juce::jmin(numOutputChannels, internalBuffer.getNumChannels()); ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            juce::FloatVectorOperations::copy(outputChannelData[ch], 
                                            internalBuffer.getReadPointer(ch), 
                                            numSamples);
        }
    }
    
    // Clear any remaining output channels
    for (int ch = internalBuffer.getNumChannels(); ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        }
    }
}

void Mixer::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        sampleRate = device->getCurrentSampleRate();
        bufferSize = device->getCurrentBufferSizeSamples();
        
        // Initialize internal buffer with enough channels (stereo for now)
        internalBuffer.setSize(2, bufferSize);
        
        // Prepare all channels
        for (auto* channel : channels)
        {
            if (channel != nullptr)
            channel->prepareToPlay(sampleRate, bufferSize);
        }
        
        if (masterChannel != nullptr)
            masterChannel->prepareToPlay(sampleRate, bufferSize);
            
        // Reset master gain smoother
        masterGainSmoother.reset(sampleRate, 0.05); // 50ms smoothing
        masterGainSmoother.setCurrentAndTargetValue(0.8f);
    }
}

void Mixer::audioDeviceStopped()
{
    // Clean up resources if needed
}

MixerChannel* Mixer::addChannel(const juce::String& name, MixerChannel::ChannelType type)
{
    const juce::ScopedLock sl(lock);
    
    auto* newChannel = new MixerChannel(name, type, sampleRate, bufferSize);
    channels.add(newChannel);
    
    return newChannel;
}

void Mixer::removeChannel(int index)
{
    const juce::ScopedLock sl(lock);
    
    if (juce::isPositiveAndBelow(index, channels.size()))
    {
        channels.remove(index);
    }
}

MixerChannel* Mixer::getChannel(int index) const
{
    const juce::ScopedLock sl(lock);
    
    if (juce::isPositiveAndBelow(index, channels.size()))
        return channels[index];
        
    return nullptr;
}

int Mixer::getNumChannels() const
{
    return channels.size();
}

void Mixer::setMasterGain(float newGain)
{
    masterGainSmoother.setTargetValue(juce::jlimit(0.0f, 1.0f, newGain));
}

float Mixer::getMasterGain() const
{
    return masterGainSmoother.getTargetValue();
}

void Mixer::handleSoloStateChanged()
{
    bool anySoloed = false;
    
    // First, check if any channel is soloed
    for (auto* channel : channels)
    {
        if (channel != nullptr && channel->isSolo())
        {
            anySoloed = true;
            break;
        }
    }
    
    // Update solo mute state for all channels without affecting user mute
    for (auto* channel : channels)
    {
        if (channel != nullptr)
        {
            // Set solo mute only if another channel is soloed and this one isn't
            channel->setSoloMute(anySoloed && !channel->isSolo());
        }
    }
}

juce::ValueTree Mixer::getState() const
{
    juce::ValueTree state("MIXER");
    
    // Save master channel state
    if (masterChannel != nullptr)
    {
        state.addChild(masterChannel->getState(), -1, nullptr);
    }
    
    // Save all channels
    for (auto* channel : channels)
    {
        if (channel != nullptr)
        {
            state.addChild(channel->getState(), -1, nullptr);
        }
    }
    
    return state;
}

void Mixer::setState(const juce::ValueTree& state)
{
    if (state.hasType("MIXER"))
    {
        // Load master channel state (first child)
        if (state.getNumChildren() > 0 && masterChannel != nullptr)
        {
            masterChannel->setState(state.getChild(0));
        }
        
        // Load other channels
        for (int i = 1; i < state.getNumChildren(); ++i)
        {
            const auto& channelState = state.getChild(i);
            auto* channel = addChannel(channelState.getProperty("name", "Channel").toString(),
                                     MixerChannel::ChannelType::Audio);
            
            if (channel != nullptr)
            {
                channel->setState(channelState);
            }
        }
    }
}
