#include "AudioEngine.h"
#include "../Models/AudioClip.h"

AudioEngine::AudioEngine()
    : sequencerEngine(patternManager, transportController)
{
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

bool AudioEngine::initialize()
{
    // Initialize the audio device manager with default number of channels
    const int numInputChannels = 2;
    const int numOutputChannels = 2;
    
    juce::String audioError = deviceManager.initialise(
        numInputChannels,
        numOutputChannels,
        nullptr,  // Use default device setup
        true,     // Select default device on failure
        juce::String(),  // Preferred default device name
        nullptr   // Preferred setup options
    );
    
    if (audioError.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Audio Device Error",
            "Failed to initialize audio device:\n" + audioError
        );
        return false;
    }
    
    // Add this as the audio callback
    deviceManager.addAudioCallback(this);
    
    return true;
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        currentSampleRate = device->getCurrentSampleRate();
        currentBlockSize = device->getCurrentBufferSizeSamples();
        
        juce::Logger::writeToLog("Audio device started:");
        juce::Logger::writeToLog("  Sample Rate: " + juce::String(currentSampleRate) + " Hz");
        juce::Logger::writeToLog("  Buffer Size: " + juce::String(currentBlockSize) + " samples");
        juce::Logger::writeToLog("  Device: " + device->getName());
    }
}

void AudioEngine::audioDeviceStopped()
{
    juce::Logger::writeToLog("Audio device stopped");
    currentSampleRate = 0.0;
    currentBlockSize = 0;
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                  int numInputChannels,
                                                  float* const* outputChannelData,
                                                  int numOutputChannels,
                                                  int numSamples,
                                                  const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(inputChannelData, numInputChannels, context);
    
    // CRITICAL: Prevent denormals for massive CPU savings
    juce::ScopedNoDenormals noDenormals;
    
    // Clear output buffers first (vectorized operation)
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
        {
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
        }
    }
    
    // Process based on playback mode
    if (transportController.isPlaying())
    {
        PlaybackMode mode = playbackMode.load();
        
        if (mode == PlaybackMode::Pattern)
        {
            // Pattern mode: play sequencer patterns
            renderMidiFromSequencer(midiBuffer, numSamples);
        }
        else // Song mode
        {
            // Song mode: play playlist clips
            renderAudioClips(outputChannelData, numOutputChannels, numSamples);
        }
        
        transportController.advancePosition(numSamples, currentSampleRate);
    }
    else if (transportController.isRecording())
    {
        transportController.advancePosition(numSamples, currentSampleRate);
    }
}

double AudioEngine::getSampleRate() const
{
    return currentSampleRate;
}

int AudioEngine::getBlockSize() const
{
    return currentBlockSize;
}

juce::String AudioEngine::getCurrentAudioDeviceName() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getName();
    
    return "No device";
}

void AudioEngine::setAudioClips(const std::vector<AudioClip>* clips)
{
    const juce::ScopedLock lock(clipsLock);
    audioClips = clips;
}

void AudioEngine::renderAudioClips(float* const* outputChannelData, int numOutputChannels, int numSamples)
{
    const juce::ScopedLock lock(clipsLock);
    
    if (audioClips == nullptr || currentSampleRate <= 0.0)
        return;
    
    // Get current playback position in samples
    int64_t playbackPositionSamples = transportController.getPositionInSamples();
    double playbackPositionBeats = transportController.getPosition();
    
    // Iterate through all clips and mix them
    for (const auto& clip : *audioClips)
    {
        // Skip if clip has no audio data
        if (clip.audioData.getNumSamples() == 0)
            continue;
        
        // Calculate clip start and end in beats
        double clipStartBeats = clip.startTime;
        double clipEndBeats = clip.startTime + clip.duration;
        
        // Skip if playback position is outside clip range
        if (playbackPositionBeats >= clipEndBeats || playbackPositionBeats + (numSamples / currentSampleRate) * (transportController.getTempo() / 60.0) < clipStartBeats)
            continue;
        
        // Calculate sample offset within the clip
        double offsetBeats = playbackPositionBeats - clipStartBeats;
        double offsetSeconds = offsetBeats * 60.0 / transportController.getTempo();
        int64_t clipSampleOffset = (int64_t)(offsetSeconds * clip.sampleRate);
        
        // Clamp to valid range
        if (clipSampleOffset < 0)
            clipSampleOffset = 0;
        
        if (clipSampleOffset >= clip.audioData.getNumSamples())
            continue;
        
        // Calculate how many samples to read
        int samplesToRead = juce::jmin(numSamples, (int)(clip.audioData.getNumSamples() - clipSampleOffset));
        
        // Mix the clip audio into output
        for (int channel = 0; channel < numOutputChannels && channel < clip.audioData.getNumChannels(); ++channel)
        {
            if (outputChannelData[channel] != nullptr)
            {
                const float* clipData = clip.audioData.getReadPointer(channel, (int)clipSampleOffset);
                juce::FloatVectorOperations::add(outputChannelData[channel], clipData, samplesToRead);
            }
        }
    }
}

void AudioEngine::setPlaybackMode(PlaybackMode mode)
{
    playbackMode.store(mode);
}

AudioEngine::PlaybackMode AudioEngine::getPlaybackMode() const
{
    return playbackMode.load();
}

void AudioEngine::renderMidiFromSequencer(juce::MidiBuffer& outputMidiBuffer, int numSamples)
{
    if (currentSampleRate <= 0.0)
        return;
    
    // Get current playback position
    double startTimeBeats = transportController.getPosition();
    
    // Calculate end time for this block
    double blockDurationSeconds = numSamples / currentSampleRate;
    double blockDurationBeats = transportController.secondsToBeats(blockDurationSeconds);
    double endTimeBeats = startTimeBeats + blockDurationBeats;
    
    // Process the sequencer to generate MIDI events
    sequencerEngine.processBlock(outputMidiBuffer, startTimeBeats, endTimeBeats, currentSampleRate, numSamples);
    
    // TODO: In the future, route MIDI to synthesizer plugins
    // For now, MIDI events are generated but not yet connected to sound output
}
