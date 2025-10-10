#pragma once

#include <JuceHeader.h>

enum class WindowType { Generic, Mixer, Playlist, Sequencer };

struct WindowStyle
{
    juce::Colour background;
    juce::Colour border;
    int borderRadius = 8;
    float shadowOpacity = 0.6f;
};

class ThemeManager
{
public:
    static ThemeManager& get()
    {
        static ThemeManager instance;
        return instance;
    }

    WindowStyle getWindowStyle(WindowType type) const
    {
        juce::ignoreUnused(type);
        WindowStyle s;
        s.background = juce::Colour(0xff0d0a15);
        s.border = juce::Colour(0xffa855f7);
        s.borderRadius = 8;
        s.shadowOpacity = 0.6f;
        return s;
    }

private:
    ThemeManager() = default;
};


