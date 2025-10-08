#include "AudioClip.h"

AudioClip::AudioClip() 
    : color(juce::Colours::purple)
{
}

AudioClip::AudioClip(const juce::File& file, int track, double start)
    : audioFile(file),
      trackIndex(track),
      startTime(start),
      color(juce::Colour(0xffa855f7)),
      name(file.getFileNameWithoutExtension())
{
}

bool AudioClip::loadAudioData()
{
    if (!audioFile.existsAsFile())
    {
        juce::Logger::writeToLog("Audio file does not exist: " + audioFile.getFullPathName());
        return false;
    }
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    
    if (reader == nullptr)
    {
        juce::Logger::writeToLog("Failed to create reader for: " + audioFile.getFullPathName());
        return false;
    }
    
    sampleRate = reader->sampleRate;
    double durationInSeconds = reader->lengthInSamples / sampleRate;
    
    // Convert duration from seconds to beats (assuming 120 BPM)
    double tempo = 120.0;
    duration = durationInSeconds * (tempo / 60.0);
    
    juce::Logger::writeToLog("Loading audio: " + name + 
                            " (" + juce::String(durationInSeconds, 2) + "s = " + 
                            juce::String(duration, 2) + " beats @ " + juce::String(tempo) + " BPM, " +
                            juce::String(reader->numChannels) + " channels)");
    
    // Load the entire file into memory
    audioData.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&audioData, 0, (int)reader->lengthInSamples, 0, true, true);
    
    juce::Logger::writeToLog("Successfully loaded " + juce::String(audioData.getNumSamples()) + " samples");
    
    return true;
}

double AudioClip::getEndTime() const
{
    return startTime + duration;
}

juce::Rectangle<int> AudioClip::getBounds(int trackHeight, int pixelsPerBeat, int xOffset, int yOffset) const
{
    int x = (int)(startTime * pixelsPerBeat) - xOffset;
    int y = trackIndex * trackHeight + yOffset;
    int width = (int)(duration * pixelsPerBeat);
    int height = trackHeight;
    
    return juce::Rectangle<int>(x, y, width, height);
}

void AudioClip::generateWaveformCache(int width, int height)
{
    if (audioData.getNumSamples() == 0 || width <= 0 || height <= 0)
        return;
    
    // Create cache image
    waveformCache = juce::Image(juce::Image::ARGB, width, height, true);
    juce::Graphics g(waveformCache);
    
    // Clear background
    g.fillAll(juce::Colours::transparentBlack);
    
    // Draw waveform
    const float* channelData = audioData.getReadPointer(0);
    int numSamples = audioData.getNumSamples();
    float samplesPerPixel = numSamples / (float)width;
    
    juce::Path waveformPath;
    float centerY = height * 0.5f;
    
    for (int x = 0; x < width; ++x)
    {
        int startSample = (int)(x * samplesPerPixel);
        int endSample = juce::jmin((int)((x + 1) * samplesPerPixel), numSamples);
        
        // Find min/max in this range
        float minVal = 0.0f, maxVal = 0.0f;
        for (int i = startSample; i < endSample; ++i)
        {
            float sample = channelData[i];
            minVal = juce::jmin(minVal, sample);
            maxVal = juce::jmax(maxVal, sample);
        }
        
        float topY = centerY - (maxVal * height * 0.4f);
        float bottomY = centerY - (minVal * height * 0.4f);
        
        // Draw vertical line for this pixel
        g.setColour(juce::Colour(0xffa855f7).withAlpha(0.6f));
        g.drawLine((float)x, topY, (float)x, bottomY, 1.0f);
    }
    
    waveformCacheValid = true;
}
