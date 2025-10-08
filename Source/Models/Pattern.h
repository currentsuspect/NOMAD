#pragma once

#include <JuceHeader.h>
#include <vector>
#include <algorithm>

/**
 * Pattern class represents a step-based sequence of notes.
 * Each pattern contains notes with step position, track, pitch, velocity, and duration.
 */
class Pattern
{
public:
    /**
     * Note structure representing a single note in the pattern
     */
    struct Note
    {
        int step;           // Step position in pattern
        int track;          // Track index
        int pitch;          // MIDI note number (0-127)
        float velocity;     // Velocity (0.0 to 1.0)
        int duration;       // Duration in steps
        
        Note() : step(0), track(0), pitch(60), velocity(1.0f), duration(1) {}
        
        Note(int s, int t, int p, float v, int d)
            : step(s), track(t), pitch(p), velocity(v), duration(d) {}
        
        bool operator==(const Note& other) const
        {
            return step == other.step && track == other.track && 
                   pitch == other.pitch && velocity == other.velocity && 
                   duration == other.duration;
        }
    };
    
    Pattern();
    Pattern(const juce::String& name, int lengthInSteps = 16, int stepsPerBeat = 4);
    
    // Note management
    void addNote(const Note& note);
    void removeNote(int step, int track);
    void removeNote(int step, int track, int pitch);
    std::vector<Note> getNotesInRange(int startStep, int endStep) const;
    std::vector<Note> getAllNotes() const;
    void clearAllNotes();
    
    // Pattern configuration
    void setLength(int steps);
    int getLength() const;
    
    void setStepsPerBeat(int steps);
    int getStepsPerBeat() const;
    
    void setName(const juce::String& name);
    juce::String getName() const;
    
    // Pattern operations
    std::unique_ptr<Pattern> clone() const;
    void copyFrom(const Pattern& other);
    
private:
    std::vector<Note> notes;
    int lengthInSteps;
    int stepsPerBeat;
    juce::String name;
    
    juce::CriticalSection lock;
};
