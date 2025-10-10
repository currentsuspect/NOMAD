#pragma once

#include <JuceHeader.h>
#include "TransportController.h"
#include "SequencerEngine.h"
#include "../Models/PatternManager.h"
#include "Mixer.h"

/**
 * Core audio engine that manages audio I/O and processing.
 * Inherits from AudioIODeviceCallback for real-time audio processing.
 */
class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    enum class PlaybackMode
    {
        Pattern,  // Pattern mode - plays sequencer patterns in loop
        Song      // Song mode - plays playlist/arrangement
    };
    
    AudioEngine();
    ~AudioEngine() override;
    
    // AudioIODeviceCallback interface
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    // Audio device management
    bool initialize();
    void shutdown();
    
    // Getters for audio info
    double getSampleRate() const;
    int getBlockSize() const;
    juce::String getCurrentAudioDeviceName() const;
    
    // Access to device manager for UI
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    
    // Access to transport controller
    TransportController& getTransportController() { return transportController; }
    
    // Access to pattern manager
    PatternManager& getPatternManager() { return patternManager; }
    
    // Access to sequencer engine
    SequencerEngine& getSequencerEngine() { return sequencerEngine; }
    
    // Access to mixer
    Mixer* getMixer() { return mixer.get(); }
    
    // Playback mode
    void setPlaybackMode(PlaybackMode mode);
    PlaybackMode getPlaybackMode() const;
    
    // Clip playback
    void setAudioClips(const std::vector<class AudioClip>* clips);
    void updateLoopBehavior(); // Update loop based on clips
    
private:
    void renderAudioClips(float* const* outputChannelData, int numOutputChannels, int numSamples);
    
private:
    void renderMidiFromSequencer(juce::MidiBuffer& outputMidiBuffer, int numSamples);
    
private:
    juce::AudioDeviceManager deviceManager;
    TransportController transportController;
    PatternManager patternManager;
    SequencerEngine sequencerEngine;
    
    double currentSampleRate = 0.0;
    int currentBlockSize = 0;
    
    std::atomic<PlaybackMode> playbackMode{PlaybackMode::Pattern};
    
    // Audio clips (non-owning pointer, owned by PlaylistComponent)
    const std::vector<class AudioClip>* audioClips = nullptr;
    juce::CriticalSection clipsLock;
    
    // Audio mixer
    std::unique_ptr<Mixer> mixer;
    
    // MIDI buffer for sequencer output
    juce::MidiBuffer midiBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
