#pragma once

#include <JuceHeader.h>
#include "../Models/Pattern.h"
#include "../Models/PatternManager.h"
#include "TransportController.h"
#include <map>
#include <memory>

/**
 * SequencerEngine processes patterns and generates MIDI events for playback.
 * It converts pattern steps to MIDI note on/off events with sample-accurate timing.
 */
class SequencerEngine
{
public:
    using PatternID = PatternManager::PatternID;
    
    SequencerEngine(PatternManager& patternManager, TransportController& transport);
    ~SequencerEngine() = default;
    
    /**
     * Process a block of audio and generate MIDI events.
     * Called from the audio thread.
     * 
     * @param midiMessages Buffer to fill with MIDI events
     * @param startTime Start time in beats
     * @param endTime End time in beats
     * @param sampleRate Current sample rate
     * @param blockSize Number of samples in this block
     */
    void processBlock(juce::MidiBuffer& midiMessages,
                     double startTime,
                     double endTime,
                     double sampleRate,
                     int blockSize);
    
    /**
     * Set the active pattern to be played by the sequencer.
     * Thread-safe.
     */
    void setActivePattern(PatternID id);
    
    /**
     * Get the currently active pattern ID.
     */
    PatternID getActivePattern() const;
    
    /**
     * Enable or disable loop mode for the active pattern.
     * When enabled, the pattern will loop continuously during playback.
     */
    void setLoopEnabled(bool enabled);
    
    /**
     * Check if loop mode is enabled.
     */
    bool isLoopEnabled() const;
    
    /**
     * Reset the sequencer state (stop all notes, reset position).
     */
    void reset();
    
private:
    PatternManager& patternManager;
    TransportController& transportController;
    
    std::atomic<PatternID> activePatternID { -1 };
    std::atomic<bool> loopEnabled { true };
    
    // Track which notes are currently playing to send note-offs
    struct ActiveNote
    {
        int pitch;
        int channel;
        int track;
        double endTimeBeats;
    };
    
    std::vector<ActiveNote> activeNotes;
    juce::CriticalSection activeNotesLock;
    
    /**
     * Generate MIDI events for a pattern within the given time range.
     */
    void generateMidiForPattern(juce::MidiBuffer& midiMessages,
                               const Pattern* pattern,
                               double startTime,
                               double endTime,
                               double sampleRate,
                               int blockSize);
    
    /**
     * Convert a time in beats to a sample offset within the current block.
     */
    int beatsToSampleOffset(double beats, double blockStartBeats, double sampleRate, int blockSize) const;
    
    /**
     * Send note-off messages for any notes that should end in this block.
     */
    void processNoteOffs(juce::MidiBuffer& midiMessages,
                        double startTime,
                        double endTime,
                        double sampleRate,
                        int blockSize);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerEngine)
};
