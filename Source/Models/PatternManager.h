#pragma once

#include <JuceHeader.h>
#include "Pattern.h"
#include <map>
#include <memory>

/**
 * PatternManager handles multiple patterns and provides pattern management operations.
 * It maintains a collection of patterns with unique IDs and supports copy/paste operations.
 */
class PatternManager
{
public:
    using PatternID = int;
    
    PatternManager();
    
    // Pattern creation and deletion
    PatternID createPattern(const juce::String& name = "New Pattern");
    PatternID createPattern(const juce::String& name, int lengthInSteps, int stepsPerBeat);
    bool deletePattern(PatternID id);
    
    // Pattern access
    Pattern* getPattern(PatternID id);
    const Pattern* getPattern(PatternID id) const;
    bool hasPattern(PatternID id) const;
    
    // Pattern operations
    PatternID copyPattern(PatternID sourceId);
    PatternID copyPattern(PatternID sourceId, const juce::String& newName);
    bool pastePattern(PatternID targetId, PatternID sourceId);
    
    // Pattern list management
    std::vector<PatternID> getAllPatternIDs() const;
    int getPatternCount() const;
    
    // Clipboard operations
    void copyToClipboard(PatternID id);
    PatternID pasteFromClipboard();
    PatternID pasteFromClipboard(const juce::String& name);
    bool hasClipboard() const;
    
    // Clear all patterns
    void clear();
    
private:
    std::map<PatternID, std::unique_ptr<Pattern>> patterns;
    PatternID nextPatternID;
    std::unique_ptr<Pattern> clipboard;
    
    juce::CriticalSection lock;
    
    PatternID generatePatternID();
};
