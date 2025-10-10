# Mixer Fixes Design Document

## Overview

This design addresses critical architectural and implementation issues in the NOMAD DAW mixer system. The fixes focus on proper audio routing, accurate metering, correct solo/mute behavior, and efficient processing.

## Architecture

### Current Issues

1. **Broken Audio Flow**: Mixer copies input to internal buffer, then each channel copies the entire internal buffer, processes it, and adds it back. This means all channels process the same input.

2. **No Source Architecture**: Channels have no concept of audio sources - they just process whatever buffer is passed to them.

3. **Inefficient Processing**: Multiple unnecessary buffer copies in both mixer and effects processor.

### Proposed Architecture

```
Audio Sources (Tracks/Instruments)
        ↓
    MixerChannel (with AudioSource)
        ↓
    Effects Chain (in-place processing)
        ↓
    Gain/Pan Processing
        ↓
    Mixer (sums all channels)
        ↓
    Master Channel
        ↓
    Audio Output
```

## Components and Interfaces

### 1. MixerChannel Enhancements

**Add Audio Source Support:**
```cpp
class MixerChannel {
    // Add audio source that feeds this channel
    void setAudioSource(juce::AudioSource* source);
    
    // Process with internal source
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
private:
    juce::AudioSource* audioSource = nullptr;
    juce::AudioBuffer<float> sourceBuffer; // Buffer for source audio
};
```

**Fix Missing updateMetering Method:**
```cpp
private:
    void updateMetering(const juce::AudioBuffer<float>& buffer);
```

**Fix Solo/Mute Logic:**
- Add `userMute` flag separate from solo-induced mute
- Only check `userMute` in processBlock
- handleSoloStateChanged affects playback without changing user state

### 2. Mixer Audio Routing Fix

**Current (Broken):**
```cpp
// All channels process the same input buffer
for (auto* channel : channels) {
    channelBuffer.copyFrom(internalBuffer); // WRONG!
    channel->processBlock(channelBuffer);
    internalBuffer.addFrom(channelBuffer);
}
```

**Fixed:**
```cpp
// Each channel processes its own source
internalBuffer.clear();
for (auto* channel : channels) {
    juce::AudioBuffer<float> channelBuffer(2, numSamples);
    channelBuffer.clear();
    
    // Channel gets audio from its source
    channel->processBlock(channelBuffer, midiBuffer);
    
    // Sum to mix bus
    for (int ch = 0; ch < 2; ++ch)
        internalBuffer.addFrom(ch, 0, channelBuffer, ch, 0, numSamples);
}
```

### 3. Effects Processor Optimization

**Current (Inefficient):**
```cpp
// Creates temp buffer and copies unnecessarily
juce::AudioBuffer<float> tempBuffer(...);
tempBuffer.copyFrom(buffer);
effect->processBlock(tempBuffer, midi);
buffer.copyFrom(tempBuffer);
```

**Fixed:**
```cpp
// Process in-place
effect->processBlock(buffer, midiMessages);
```

### 4. Master Gain Smoothing Fix

**Current (Bug):**
```cpp
const float masterGain = masterGainSmoother.getNextValue(); // Gets ONE value
for (int ch = 0; ch < numChannels; ++ch)
    buffer.applyGain(ch, 0, numSamples, masterGain); // Applies to ALL samples
```

**Fixed:**
```cpp
// Apply smoothing per-sample
for (int ch = 0; ch < numChannels; ++ch) {
    float* channelData = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i)
        channelData[i] *= masterGainSmoother.getNextValue();
}
```

### 5. Metering Implementation

**Add the missing method:**
```cpp
void MixerChannel::updateMetering(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    float sumOfSquares = 0.0f;
    int totalSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const float sample = std::abs(data[i]);
            peak = std::max(peak, sample);
            sumOfSquares += sample * sample;
            totalSamples++;
        }
    }
    
    // Update peak with hold
    const float currentPeak = peakLevel.load();
    if (peak > currentPeak || peakHoldTime <= 0.0f) {
        peakLevel.store(peak);
        peakHoldTime = peakHoldDuration;
    } else {
        peakHoldTime -= buffer.getNumSamples() / sampleRate;
        peakLevel.store(currentPeak * 0.999f);
    }
    
    // Update RMS
    if (totalSamples > 0) {
        rmsLevel.store(std::sqrt(sumOfSquares / totalSamples));
    }
}
```

**Add sample rate tracking:**
```cpp
class MixerChannel {
private:
    double sampleRate = 44100.0; // Track for metering decay
};
```

### 6. Pan Processing Fix

**Current Issue:** Mono panning tries to write to channel 1 when buffer might be mono.

**Fixed:**
```cpp
if (numChannels == 1) {
    // Mono source - just apply gain, no panning
    float* data = buffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
        data[i] *= gainSmoother.getNextValue();
} else if (numChannels >= 2) {
    // Stereo - apply pan
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    
    for (int i = 0; i < numSamples; ++i) {
        const float gain = gainSmoother.getNextValue();
        const float pan = panSmoother.getNextValue();
        
        const float leftGain = (pan <= 0) ? 1.0f : (1.0f - pan);
        const float rightGain = (pan >= 0) ? 1.0f : (1.0f + pan);
        
        left[i] *= gain * leftGain;
        right[i] *= gain * rightGain;
    }
}
```

### 7. Solo/Mute State Management

**Add separate state tracking:**
```cpp
class MixerChannel {
private:
    std::atomic<bool> solo { false };      // User solo button
    std::atomic<bool> userMute { false };  // User mute button
    std::atomic<bool> soloMute { false };  // Muted due to solo on other channel
};
```

**Update logic:**
```cpp
void MixerChannel::processBlock(...) {
    // Check both user mute and solo-induced mute
    if (userMute.load() || soloMute.load()) {
        buffer.clear();
        return;
    }
    // ... rest of processing
}

void Mixer::handleSoloStateChanged() {
    bool anySoloed = false;
    for (auto* ch : channels)
        if (ch->isSolo()) { anySoloed = true; break; }
    
    for (auto* ch : channels)
        ch->setSoloMute(anySoloed && !ch->isSolo());
}
```

## Data Models

### MixerChannel State
```cpp
struct MixerChannelState {
    String name;
    float gain;           // 0.0 to 1.0
    float pan;            // -1.0 to 1.0
    bool solo;            // User solo state
    bool userMute;        // User mute state
    bool soloMute;        // Solo-induced mute (not saved)
    ChannelType type;
    // Effects state...
};
```

## Error Handling

1. **Null Pointer Checks**: Always check channel pointers before dereferencing
2. **Buffer Size Validation**: Ensure buffers are properly sized before processing
3. **Thread Safety**: Use CriticalSection for all channel array access
4. **Sample Rate Validation**: Check for positive, non-subnormal values

## Testing Strategy

### Unit Tests (Optional)
- Test solo/mute state combinations
- Test gain/pan calculations
- Test metering accuracy
- Test buffer routing

### Integration Tests
- Test full mixer with multiple channels
- Test audio source routing
- Test effects chain processing
- Test state save/load

### Manual Testing
- Use MixerTest app to verify:
  - Meters update correctly
  - Solo/mute buttons work as expected
  - Gain/pan controls affect audio
  - No clicks or pops during parameter changes
  - CPU usage is reasonable

## Implementation Notes

1. **Backward Compatibility**: State loading should handle old format without soloMute
2. **Performance**: Minimize allocations in audio thread
3. **JUCE Best Practices**: Use JUCE atomic types and thread primitives
4. **Code Style**: Match existing NOMAD DAW conventions
