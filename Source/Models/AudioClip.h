#pragma once

#include <JuceHeader.h>

/**
 * Represents an audio clip in the playlist
 * Contains file reference, position, and playback state
 */
class AudioClip
{
public:
    juce::File audioFile;
    int trackIndex = 0;
    double startTime = 0.0;  // In beats or seconds
    double duration = 0.0;    // In seconds
    juce::Colour color;
    juce::String name;
    
    // Audio data
    juce::AudioSampleBuffer audioData;
    double sampleRate = 44100.0;
    
    // Waveform cache for fast rendering
    juce::Image waveformCache;
    bool waveformCacheValid = false;
    
    AudioClip();
    AudioClip(const juce::File& file, int track, double start);
    
    bool loadAudioData();
    double getEndTime() const;
    juce::Rectangle<int> getBounds(int trackHeight, int pixelsPerBeat, int xOffset, int yOffset) const;
    
    // Waveform caching for performance
    void generateWaveformCache(int width, int height);
    const juce::Image& getWaveformCache() const { return waveformCache; }
    bool hasValidWaveformCache() const { return waveformCacheValid; }
};
