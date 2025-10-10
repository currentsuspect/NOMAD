#pragma once

#include <JuceHeader.h>

/**
 * Global drag state manager for UI performance optimization.
 * When dragging windows or components, enters "lightweight mode" that:
 * - Disables shadows and blur effects
 * - Reduces visual complexity
 * - Ensures smooth 60 FPS dragging like FL Studio
 */
class DragStateManager
{
public:
    static DragStateManager& getInstance()
    {
        static DragStateManager instance;
        return instance;
    }
    
    // Enter lightweight rendering mode (during drag operations)
    void enterLightweightMode()
    {
        if (!isLightweightMode)
        {
            isLightweightMode = true;
            notifyListeners();
        }
    }
    
    // Exit lightweight mode (drag finished)
    void exitLightweightMode()
    {
        if (isLightweightMode)
        {
            isLightweightMode = false;
            notifyListeners();
        }
    }
    
    // Check if currently in lightweight mode
    bool isLightweight() const
    {
        return isLightweightMode;
    }
    
    // Listener interface for components that need to respond to drag state changes
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void dragStateChanged(bool isLightweight) = 0;
    };
    
    // Add a listener
    void addListener(Listener* listener)
    {
        listeners.add(listener);
    }
    
    // Remove a listener
    void removeListener(Listener* listener)
    {
        listeners.remove(listener);
    }
    
private:
    DragStateManager() = default;
    ~DragStateManager() = default;
    
    // Prevent copying
    DragStateManager(const DragStateManager&) = delete;
    DragStateManager& operator=(const DragStateManager&) = delete;
    
    void notifyListeners()
    {
        listeners.call([this](Listener& l) { l.dragStateChanged(isLightweightMode); });
    }
    
    bool isLightweightMode = false;
    juce::ListenerList<Listener> listeners;
};
