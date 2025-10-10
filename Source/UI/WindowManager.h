#pragma once

#include <JuceHeader.h>

class FloatingWindow; // forward declaration to avoid circular include

class WindowManager
{
public:
    static WindowManager& get()
    {
        static WindowManager instance;
        return instance;
    }

    void registerWindow(FloatingWindow* window)
    {
        if (window != nullptr)
            windows.addIfNotAlreadyThere(window);
    }

    void unregisterWindow(FloatingWindow* window)
    {
        windows.removeAllInstancesOf(window);
    }

    void bringToFront(FloatingWindow* window)
    {
        if (window != nullptr)
            window->toFront(true);
    }

    void repaintAll()
    {
        for (int i = 0; i < windows.size(); ++i)
        {
            if (auto* w = windows[i])
                w->repaint();
        }
    }

private:
    WindowManager() = default;
    juce::Array<FloatingWindow*> windows;
};


