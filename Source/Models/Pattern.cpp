#include "Pattern.h"

Pattern::Pattern()
    : lengthInSteps(16)
    , stepsPerBeat(4)
    , name("New Pattern")
{
}

Pattern::Pattern(const juce::String& patternName, int lengthInSteps, int stepsPerBeat)
    : lengthInSteps(lengthInSteps)
    , stepsPerBeat(stepsPerBeat)
    , name(patternName)
{
}

void Pattern::addNote(const Note& note)
{
    const juce::ScopedLock sl(lock);
    
    // Validate note parameters
    if (note.step < 0 || note.step >= lengthInSteps)
        return;
    
    if (note.pitch < 0 || note.pitch > 127)
        return;
    
    if (note.velocity < 0.0f || note.velocity > 1.0f)
        return;
    
    // Check if a note already exists at this position
    auto it = std::find_if(notes.begin(), notes.end(),
        [&note](const Note& n) {
            return n.step == note.step && n.track == note.track && n.pitch == note.pitch;
        });
    
    if (it != notes.end())
    {
        // Update existing note
        *it = note;
    }
    else
    {
        // Add new note
        notes.push_back(note);
    }
}

void Pattern::removeNote(int step, int track)
{
    const juce::ScopedLock sl(lock);
    
    notes.erase(
        std::remove_if(notes.begin(), notes.end(),
            [step, track](const Note& n) {
                return n.step == step && n.track == track;
            }),
        notes.end()
    );
}

void Pattern::removeNote(int step, int track, int pitch)
{
    const juce::ScopedLock sl(lock);
    
    notes.erase(
        std::remove_if(notes.begin(), notes.end(),
            [step, track, pitch](const Note& n) {
                return n.step == step && n.track == track && n.pitch == pitch;
            }),
        notes.end()
    );
}

std::vector<Pattern::Note> Pattern::getNotesInRange(int startStep, int endStep) const
{
    const juce::ScopedLock sl(lock);
    
    std::vector<Note> result;
    
    for (const auto& note : notes)
    {
        if (note.step >= startStep && note.step < endStep)
        {
            result.push_back(note);
        }
    }
    
    return result;
}

std::vector<Pattern::Note> Pattern::getAllNotes() const
{
    const juce::ScopedLock sl(lock);
    return notes;
}

void Pattern::clearAllNotes()
{
    const juce::ScopedLock sl(lock);
    notes.clear();
}

void Pattern::setLength(int steps)
{
    const juce::ScopedLock sl(lock);
    
    if (steps > 0)
    {
        lengthInSteps = steps;
        
        // Remove notes that are beyond the new length
        notes.erase(
            std::remove_if(notes.begin(), notes.end(),
                [steps](const Note& n) {
                    return n.step >= steps;
                }),
            notes.end()
        );
    }
}

int Pattern::getLength() const
{
    return lengthInSteps;
}

void Pattern::setStepsPerBeat(int steps)
{
    if (steps > 0)
    {
        stepsPerBeat = steps;
    }
}

int Pattern::getStepsPerBeat() const
{
    return stepsPerBeat;
}

void Pattern::setName(const juce::String& patternName)
{
    name = patternName;
}

juce::String Pattern::getName() const
{
    return name;
}

std::unique_ptr<Pattern> Pattern::clone() const
{
    const juce::ScopedLock sl(lock);
    
    auto cloned = std::make_unique<Pattern>(name + " (Copy)", lengthInSteps, stepsPerBeat);
    
    // Copy notes without holding the lock on the new pattern
    for (const auto& note : notes)
    {
        cloned->notes.push_back(note);
    }
    
    return cloned;
}

void Pattern::copyFrom(const Pattern& other)
{
    const juce::ScopedLock sl(lock);
    
    lengthInSteps = other.lengthInSteps;
    stepsPerBeat = other.stepsPerBeat;
    name = other.name;
    
    // Copy notes from other pattern
    notes.clear();
    auto otherNotes = other.getAllNotes();
    for (const auto& note : otherNotes)
    {
        notes.push_back(note);
    }
}
