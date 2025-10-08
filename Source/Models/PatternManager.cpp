#include "PatternManager.h"

PatternManager::PatternManager()
    : nextPatternID(1)
{
}

PatternManager::PatternID PatternManager::createPattern(const juce::String& name)
{
    return createPattern(name, 16, 4);
}

PatternManager::PatternID PatternManager::createPattern(const juce::String& name, 
                                                        int lengthInSteps, 
                                                        int stepsPerBeat)
{
    const juce::ScopedLock sl(lock);
    
    PatternID id = generatePatternID();
    patterns[id] = std::make_unique<Pattern>(name, lengthInSteps, stepsPerBeat);
    
    return id;
}

bool PatternManager::deletePattern(PatternID id)
{
    const juce::ScopedLock sl(lock);
    
    auto it = patterns.find(id);
    if (it != patterns.end())
    {
        patterns.erase(it);
        return true;
    }
    
    return false;
}

Pattern* PatternManager::getPattern(PatternID id)
{
    const juce::ScopedLock sl(lock);
    
    auto it = patterns.find(id);
    if (it != patterns.end())
    {
        return it->second.get();
    }
    
    return nullptr;
}

const Pattern* PatternManager::getPattern(PatternID id) const
{
    const juce::ScopedLock sl(lock);
    
    auto it = patterns.find(id);
    if (it != patterns.end())
    {
        return it->second.get();
    }
    
    return nullptr;
}

bool PatternManager::hasPattern(PatternID id) const
{
    const juce::ScopedLock sl(lock);
    return patterns.find(id) != patterns.end();
}

PatternManager::PatternID PatternManager::copyPattern(PatternID sourceId)
{
    const juce::ScopedLock sl(lock);
    
    auto it = patterns.find(sourceId);
    if (it == patterns.end())
        return -1;
    
    PatternID newId = generatePatternID();
    patterns[newId] = it->second->clone();
    
    return newId;
}

PatternManager::PatternID PatternManager::copyPattern(PatternID sourceId, 
                                                      const juce::String& newName)
{
    const juce::ScopedLock sl(lock);
    
    auto it = patterns.find(sourceId);
    if (it == patterns.end())
        return -1;
    
    PatternID newId = generatePatternID();
    patterns[newId] = it->second->clone();
    patterns[newId]->setName(newName);
    
    return newId;
}

bool PatternManager::pastePattern(PatternID targetId, PatternID sourceId)
{
    const juce::ScopedLock sl(lock);
    
    auto sourceIt = patterns.find(sourceId);
    auto targetIt = patterns.find(targetId);
    
    if (sourceIt == patterns.end() || targetIt == patterns.end())
        return false;
    
    targetIt->second->copyFrom(*sourceIt->second);
    
    return true;
}

std::vector<PatternManager::PatternID> PatternManager::getAllPatternIDs() const
{
    const juce::ScopedLock sl(lock);
    
    std::vector<PatternID> ids;
    ids.reserve(patterns.size());
    
    for (const auto& pair : patterns)
    {
        ids.push_back(pair.first);
    }
    
    return ids;
}

int PatternManager::getPatternCount() const
{
    const juce::ScopedLock sl(lock);
    return static_cast<int>(patterns.size());
}

void PatternManager::copyToClipboard(PatternID id)
{
    const juce::ScopedLock sl(lock);
    
    auto it = patterns.find(id);
    if (it != patterns.end())
    {
        clipboard = it->second->clone();
    }
}

PatternManager::PatternID PatternManager::pasteFromClipboard()
{
    const juce::ScopedLock sl(lock);
    
    if (!clipboard)
        return -1;
    
    PatternID newId = generatePatternID();
    patterns[newId] = clipboard->clone();
    
    return newId;
}

PatternManager::PatternID PatternManager::pasteFromClipboard(const juce::String& name)
{
    const juce::ScopedLock sl(lock);
    
    if (!clipboard)
        return -1;
    
    PatternID newId = generatePatternID();
    patterns[newId] = clipboard->clone();
    patterns[newId]->setName(name);
    
    return newId;
}

bool PatternManager::hasClipboard() const
{
    const juce::ScopedLock sl(lock);
    return clipboard != nullptr;
}

void PatternManager::clear()
{
    const juce::ScopedLock sl(lock);
    patterns.clear();
    clipboard.reset();
}

PatternManager::PatternID PatternManager::generatePatternID()
{
    return nextPatternID++;
}
