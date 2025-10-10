#include "EffectsProcessor.h"

EffectsProcessor::EffectsProcessor()
{
    // Initialize with default effects chain if needed
}

void EffectsProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (bypassed)
        return;
        
    const juce::ScopedLock sl(effectLock);
    
    // Process each effect in the chain in-place
    for (auto* effect : effectsChain)
    {
        if (effect != nullptr)
        {
            // Process the effect directly on the buffer (in-place)
            effect->processBlock(buffer, midiMessages);
        }
    }
}

void EffectsProcessor::addEffect(std::unique_ptr<juce::AudioProcessor> effect)
{
    const juce::ScopedLock sl(effectLock);
    
    if (effect != nullptr)
    {
        effect->setRateAndBufferSizeDetails(currentSampleRate, currentBlockSize);
        effect->prepareToPlay(currentSampleRate, currentBlockSize);
        effectsChain.add(effect.release());
    }
}

void EffectsProcessor::removeEffect(int index)
{
    const juce::ScopedLock sl(effectLock);
    
    if (juce::isPositiveAndBelow(index, effectsChain.size()))
    {
        effectsChain.remove(index);
    }
}

void EffectsProcessor::moveEffect(int sourceIndex, int destIndex)
{
    const juce::ScopedLock sl(effectLock);
    
    if (juce::isPositiveAndBelow(sourceIndex, effectsChain.size()) &&
        juce::isPositiveAndBelow(destIndex, effectsChain.size() + 1))
    {
        effectsChain.move(sourceIndex, destIndex);
    }
}

void EffectsProcessor::setSampleRate(double newSampleRate)
{
    if (newSampleRate > 0.0 && newSampleRate != currentSampleRate)
    {
        currentSampleRate = newSampleRate;
        
        const juce::ScopedLock sl(effectLock);
        
        for (auto* effect : effectsChain)
        {
            if (effect != nullptr)
            effect->setRateAndBufferSizeDetails(currentSampleRate, currentBlockSize);
        }
    }
}

void EffectsProcessor::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentBlockSize = samplesPerBlockExpected;
    setSampleRate(sampleRate);
    
    const juce::ScopedLock sl(effectLock);
    
    for (auto* effect : effectsChain)
    {
        if (effect != nullptr)
            effect->prepareToPlay(sampleRate, samplesPerBlockExpected);
    }
}

void EffectsProcessor::reset()
{
    const juce::ScopedLock sl(effectLock);
    
    for (auto* effect : effectsChain)
    {
        if (effect != nullptr)
            effect->reset();
    }
}

void EffectsProcessor::savePreset(const juce::String& presetName)
{
    // Implementation for saving presets
    juce::ignoreUnused(presetName);
    // TODO: Implement preset saving logic
}

void EffectsProcessor::loadPreset(const juce::String& presetName)
{
    // Implementation for loading presets
    juce::ignoreUnused(presetName);
    // TODO: Implement preset loading logic
}

void EffectsProcessor::setBypassed(bool shouldBeBypassed)
{
    bypassed = shouldBeBypassed;
}

bool EffectsProcessor::isBypassed() const
{
    return bypassed;
}
