#include "SequencerEngine.h"

SequencerEngine::SequencerEngine(PatternManager& patternManager, TransportController& transport)
    : patternManager(patternManager)
    , transportController(transport)
{
}

void SequencerEngine::processBlock(juce::MidiBuffer& midiMessages,
                                   double startTime,
                                   double endTime,
                                   double sampleRate,
                                   int blockSize)
{
    // Clear any existing MIDI messages
    midiMessages.clear();
    
    // Get the active pattern
    PatternID currentPatternID = activePatternID.load();
    if (currentPatternID < 0)
        return;
    
    const Pattern* pattern = patternManager.getPattern(currentPatternID);
    if (pattern == nullptr)
        return;
    
    // Process note-offs for currently active notes
    processNoteOffs(midiMessages, startTime, endTime, sampleRate, blockSize);
    
    // Generate MIDI events for the pattern
    generateMidiForPattern(midiMessages, pattern, startTime, endTime, sampleRate, blockSize);
}

void SequencerEngine::setActivePattern(PatternID id)
{
    // Stop all currently playing notes when switching patterns
    if (activePatternID.load() != id)
    {
        reset();
    }
    
    activePatternID.store(id);
}

SequencerEngine::PatternID SequencerEngine::getActivePattern() const
{
    return activePatternID.load();
}

void SequencerEngine::setLoopEnabled(bool enabled)
{
    loopEnabled.store(enabled);
}

bool SequencerEngine::isLoopEnabled() const
{
    return loopEnabled.load();
}

void SequencerEngine::reset()
{
    const juce::ScopedLock lock(activeNotesLock);
    activeNotes.clear();
}

void SequencerEngine::generateMidiForPattern(juce::MidiBuffer& midiMessages,
                                            const Pattern* pattern,
                                            double startTime,
                                            double endTime,
                                            double sampleRate,
                                            int blockSize)
{
    if (pattern == nullptr)
        return;
    
    const int patternLength = pattern->getLength();
    const int stepsPerBeat = pattern->getStepsPerBeat();
    
    if (patternLength == 0 || stepsPerBeat == 0)
        return;
    
    // Calculate pattern length in beats
    const double patternLengthBeats = static_cast<double>(patternLength) / stepsPerBeat;
    
    // Safety check: pattern must have positive length
    if (patternLengthBeats <= 0.0)
        return;
    
    // Handle looping
    double processStartTime = startTime;
    double processEndTime = endTime;
    
    if (loopEnabled.load())
    {
        // Wrap times to pattern length (safe with positive patternLengthBeats)
        if (startTime >= patternLengthBeats)
            processStartTime = std::fmod(startTime, patternLengthBeats);
        else
            processStartTime = startTime;
            
        if (endTime >= patternLengthBeats)
            processEndTime = std::fmod(endTime, patternLengthBeats);
        else
            processEndTime = endTime;
        
        // Handle wrap-around case (avoid infinite recursion)
        if (processEndTime < processStartTime && (endTime - startTime) < patternLengthBeats)
        {
            // Process from start to end of pattern
            auto notes1 = pattern->getNotesInRange(
                static_cast<int>(std::floor(processStartTime * stepsPerBeat)),
                patternLength
            );
            for (const auto& note : notes1)
            {
                const double noteStartBeats = static_cast<double>(note.step) / stepsPerBeat;
                if (noteStartBeats >= processStartTime)
                {
                    const int sampleOffset = beatsToSampleOffset(noteStartBeats, startTime, sampleRate, blockSize);
                    if (sampleOffset >= 0 && sampleOffset < blockSize)
                    {
                        const int midiVelocity = juce::jlimit(1, 127, static_cast<int>(note.velocity * 127.0f));
                        const int midiChannel = juce::jlimit(1, 16, note.track + 1);
                        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(midiChannel, note.pitch, static_cast<juce::uint8>(midiVelocity));
                        midiMessages.addEvent(noteOn, sampleOffset);
                    }
                }
            }
            
            // Process from beginning to wrapped end
            auto notes2 = pattern->getNotesInRange(0, static_cast<int>(std::ceil(processEndTime * stepsPerBeat)));
            for (const auto& note : notes2)
            {
                const double noteStartBeats = static_cast<double>(note.step) / stepsPerBeat;
                if (noteStartBeats < processEndTime)
                {
                    const int sampleOffset = beatsToSampleOffset(noteStartBeats + patternLengthBeats, startTime, sampleRate, blockSize);
                    if (sampleOffset >= 0 && sampleOffset < blockSize)
                    {
                        const int midiVelocity = juce::jlimit(1, 127, static_cast<int>(note.velocity * 127.0f));
                        const int midiChannel = juce::jlimit(1, 16, note.track + 1);
                        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(midiChannel, note.pitch, static_cast<juce::uint8>(midiVelocity));
                        midiMessages.addEvent(noteOn, sampleOffset);
                    }
                }
            }
            return;
        }
    }
    else
    {
        // Non-looping: stop if we've passed the pattern end
        if (startTime >= patternLengthBeats)
            return;
        
        processEndTime = std::min(endTime, patternLengthBeats);
    }
    
    // Convert beat range to step range
    const int startStep = static_cast<int>(std::floor(processStartTime * stepsPerBeat));
    const int endStep = static_cast<int>(std::ceil(processEndTime * stepsPerBeat));
    
    // Get notes in this range
    auto notes = pattern->getNotesInRange(startStep, endStep + 1);
    
    // Generate MIDI events for each note
    for (const auto& note : notes)
    {
        // Calculate note start time in beats
        const double noteStartBeats = static_cast<double>(note.step) / stepsPerBeat;
        
        // Skip if note starts before our time range
        if (noteStartBeats < processStartTime)
            continue;
        
        // Skip if note starts after our time range
        if (noteStartBeats >= processEndTime)
            continue;
        
        // Calculate sample offset for note-on
        const int sampleOffset = beatsToSampleOffset(noteStartBeats, startTime, sampleRate, blockSize);
        
        // Clamp to valid range
        if (sampleOffset < 0 || sampleOffset >= blockSize)
            continue;
        
        // Convert velocity (0.0-1.0) to MIDI velocity (0-127)
        const int midiVelocity = juce::jlimit(1, 127, static_cast<int>(note.velocity * 127.0f));
        
        // Use track as MIDI channel (clamped to 1-16)
        const int midiChannel = juce::jlimit(1, 16, note.track + 1);
        
        // Create note-on message
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(midiChannel, note.pitch, static_cast<juce::uint8>(midiVelocity));
        midiMessages.addEvent(noteOn, sampleOffset);
        
        // Calculate note end time
        const double noteDurationBeats = static_cast<double>(note.duration) / stepsPerBeat;
        const double noteEndBeats = noteStartBeats + noteDurationBeats;
        
        // Check if note ends within this block
        if (noteEndBeats <= processEndTime)
        {
            // Schedule note-off within this block
            const int noteOffSampleOffset = beatsToSampleOffset(noteEndBeats, startTime, sampleRate, blockSize);
            
            if (noteOffSampleOffset >= 0 && noteOffSampleOffset < blockSize)
            {
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(midiChannel, note.pitch);
                midiMessages.addEvent(noteOff, noteOffSampleOffset);
            }
        }
        else
        {
            // Note extends beyond this block, track it for later note-off
            const juce::ScopedLock lock(activeNotesLock);
            
            ActiveNote activeNote;
            activeNote.pitch = note.pitch;
            activeNote.channel = midiChannel;
            activeNote.track = note.track;
            activeNote.endTimeBeats = noteEndBeats;
            
            activeNotes.push_back(activeNote);
        }
    }
}

int SequencerEngine::beatsToSampleOffset(double beats, double blockStartBeats, double sampleRate, int blockSize) const
{
    // Get tempo from transport
    const double tempo = transportController.getTempo();
    
    // Calculate time difference in beats
    const double beatDiff = beats - blockStartBeats;
    
    // Convert beats to seconds
    const double secondsDiff = beatDiff * 60.0 / tempo;
    
    // Convert seconds to samples
    const double samplesDiff = secondsDiff * sampleRate;
    
    // Return as integer sample offset
    return static_cast<int>(std::round(samplesDiff));
}

void SequencerEngine::processNoteOffs(juce::MidiBuffer& midiMessages,
                                     double startTime,
                                     double endTime,
                                     double sampleRate,
                                     int blockSize)
{
    juce::ignoreUnused(blockSize);
    
    const juce::ScopedLock lock(activeNotesLock);
    
    // Process active notes and send note-offs for those that end in this block
    for (auto it = activeNotes.begin(); it != activeNotes.end(); )
    {
        const ActiveNote& note = *it;
        
        // Check if note should end in this block
        if (note.endTimeBeats >= startTime && note.endTimeBeats < endTime)
        {
            // Calculate sample offset for note-off
            const int sampleOffset = beatsToSampleOffset(note.endTimeBeats, startTime, sampleRate, blockSize);
            
            if (sampleOffset >= 0 && sampleOffset < blockSize)
            {
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(note.channel, note.pitch);
                midiMessages.addEvent(noteOff, sampleOffset);
            }
            
            // Remove from active notes
            it = activeNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
