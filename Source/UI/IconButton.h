#pragma once

#include <JuceHeader.h>

/**
 * Custom button that displays an SVG icon
 */
class IconButton : public juce::Button
{
public:
    IconButton(const juce::String& buttonName);
    ~IconButton() override = default;
    
    void loadSVG(const juce::File& svgFile);
    void loadSVGFromString(const juce::String& svgContent);
    
    void setIconColour(juce::Colour colour);
    void setIconColourActive(juce::Colour colour);
    
protected:
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
private:
    std::unique_ptr<juce::Drawable> iconDrawable;
    std::unique_ptr<juce::Drawable> iconDrawableActive;
    juce::Colour iconColour{juce::Colours::white};
    juce::Colour iconColourActive{juce::Colours::white};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconButton)
};
