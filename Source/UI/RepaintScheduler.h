#pragma once

#include <JuceHeader.h>

/**
 * Unified repaint scheduler that batches and optimizes repaint requests.
 * Combines dirty region tracking with smart repaint management.
 * Ensures single-pass redraws per frame for silky smooth performance.
 */
class RepaintScheduler
{
public:
    static RepaintScheduler& getInstance()
    {
        static RepaintScheduler instance;
        return instance;
    }
    
    // Request a repaint for a specific area of a component
    void requestRepaint(juce::Component* component, juce::Rectangle<int> area)
    {
        if (component == nullptr)
            return;
        
        const juce::ScopedLock lock(criticalSection);
        
        // Find or create entry for this component
        auto* entry = findOrCreateEntry(component);
        if (entry == nullptr)
            return;
        
        // Union with existing dirty region
        if (entry->dirtyRegion.isEmpty())
        {
            entry->dirtyRegion = area;
        }
        else
        {
            entry->dirtyRegion = entry->dirtyRegion.getUnion(area);
        }
        
        entry->needsRepaint = true;
    }
    
    // Request full component repaint
    void requestRepaint(juce::Component* component)
    {
        if (component == nullptr)
            return;
        
        requestRepaint(component, component->getLocalBounds());
    }
    
    // Flush all pending repaints (call once per frame)
    void flushRepaints()
    {
        const juce::ScopedLock lock(criticalSection);
        
        for (auto& entry : repaintQueue)
        {
            if (entry.needsRepaint && entry.component != nullptr)
            {
                // Trigger actual repaint
                if (!entry.dirtyRegion.isEmpty())
                {
                    entry.component->repaint(entry.dirtyRegion);
                }
                
                // Clear for next frame
                entry.needsRepaint = false;
                entry.dirtyRegion = juce::Rectangle<int>();
            }
        }
        
        // Clean up dead components
        repaintQueue.removeIf([](const RepaintEntry& entry) {
            return entry.component == nullptr;
        });
    }
    
    // Check if a component has pending repaints
    bool hasPendingRepaints(juce::Component* component) const
    {
        const juce::ScopedLock lock(criticalSection);
        
        for (const auto& entry : repaintQueue)
        {
            if (entry.component == component && entry.needsRepaint)
                return true;
        }
        
        return false;
    }
    
    // Clear all pending repaints
    void clear()
    {
        const juce::ScopedLock lock(criticalSection);
        repaintQueue.clear();
    }
    
private:
    RepaintScheduler() = default;
    ~RepaintScheduler() = default;
    
    // Prevent copying
    RepaintScheduler(const RepaintScheduler&) = delete;
    RepaintScheduler& operator=(const RepaintScheduler&) = delete;
    
    struct RepaintEntry
    {
        juce::Component::SafePointer<juce::Component> component;
        juce::Rectangle<int> dirtyRegion;
        bool needsRepaint = false;
    };
    
    RepaintEntry* findOrCreateEntry(juce::Component* component)
    {
        // Find existing entry
        for (auto& entry : repaintQueue)
        {
            if (entry.component == component)
                return &entry;
        }
        
        // Create new entry
        RepaintEntry newEntry;
        newEntry.component = component;
        repaintQueue.add(newEntry);
        return &repaintQueue.getReference(repaintQueue.size() - 1);
    }
    
    juce::Array<RepaintEntry> repaintQueue;
    mutable juce::CriticalSection criticalSection;
};
