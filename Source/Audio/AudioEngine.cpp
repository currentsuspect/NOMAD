#include "AudioEngine.h"
#include "../Models/AudioClip.h"

AudioEngine::AudioEngine()
    : sequencerEngine(patternManager, transportController)
{
    // Create the mixer
    mixer = std::make_unique<Mixer>();
    
    // Add default channels (like FL Studio - multiple tracks + master)
    if (mixer)
    {
        // Add 8 audio tracks
        for (int i = 1; i <= 8; ++i)
        {
            mixer->addChannel("Track " + juce::String(i), MixerChannel::ChannelType::Audio);
        }
        
        // Add master channel
        mixer->addChannel("Master", MixerChannel::ChannelType::Master);
    }
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
    // Remove the mixer callback first
    if (mixer)
    {
        deviceManager.removeAudioCallback(mixer.get());
    }
    
    // Then remove our own callback
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
    
    // Reset the mixer
    mixer.reset();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                 int numInputChannels,
                                                 float* const* outputChannelData,
                                                 int numOutputChannels,
                                                 int numSamples,
                                                 const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context, inputChannelData, numInputChannels);
    
    // Clear output buffers
    for (int i = 0; i < numOutputChannels; ++i)
    {
        if (outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
    
    // Only process if transport is playing
    if (!transportController.isPlaying())
        return;
    
    // Render based on playback mode
    if (playbackMode.load() == PlaybackMode::Song)
    {
        // Song mode - render audio clips from playlist
        renderAudioClips(outputChannelData, numOutputChannels, numSamples);
    }
    else
    {
        // Pattern mode - render MIDI from sequencer
        midiBuffer.clear();
        renderMidiFromSequencer(midiBuffer, numSamples);
        
        // TODO: Route MIDI to synthesizers
    }
    
    // Update transport position
    transportController.advancePosition(numSamples, currentSampleRate);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        currentSampleRate = device->getCurrentSampleRate();
        currentBlockSize = device->getCurrentBufferSizeSamples();
        
        // Initialize the mixer with the correct sample rate and buffer size
        if (mixer)
        {
            deviceManager.addAudioCallback(mixer.get());
        }
    }
}

void AudioEngine::audioDeviceStopped()
{
    // Clean up when audio device stops
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
    updateLoopBehavior();
}

void AudioEngine::updateLoopBehavior()
{
    const juce::ScopedLock lock(clipsLock);
    
    // Check if we're in song mode
    if (playbackMode.load() != PlaybackMode::Song)
    {
        // In pattern mode, enable looping with default 4-bar loop
        transportController.setLoopEnabled(true);
        transportController.setLoopPoints(0.0, 16.0);
        return;
    }
    
    // In song mode, check if there are any clips
    if (audioClips == nullptr || audioClips->empty())
    {
        // No clips - enable looping with default 4-bar loop for empty playlist
        transportController.setLoopEnabled(true);
        transportController.setLoopPoints(0.0, 16.0);
        return;
    }
    
    // There are clips - disable looping and let playback continue
    transportController.setLoopEnabled(false);
    
    // Optionally, we could calculate the end point of the furthest clip
    // and stop playback there, but for now we just disable looping
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
    updateLoopBehavior(); // Update loop behavior when mode changes
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
