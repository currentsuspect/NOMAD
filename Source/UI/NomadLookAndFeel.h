#pragma once

#include <JuceHeader.h>

/**
 * Custom LookAndFeel for NOMAD DAW
 * Implements modern, rounded UI elements with smooth animations
 */
class NomadLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NomadLookAndFeel();
    
    // Button drawing
    void drawButtonBackground(juce::Graphics& g,
                            juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
    
    void drawButtonText(juce::Graphics& g,
                       juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted,
                       bool shouldDrawButtonAsDown) override;
    
    // Slider drawing
    void drawLinearSlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPos,
                         float minSliderPos,
                         float maxSliderPos,
                         const juce::Slider::SliderStyle style,
                         juce::Slider& slider) override;
    
    // Label drawing
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Title bar drawing
    void drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                   juce::Graphics& g,
                                   int w, int h,
                                   int titleSpaceX, int titleSpaceW,
                                   const juce::Image* icon,
                                   bool drawTitleTextOnLeft) override;
    
    juce::Button* createDocumentWindowButton(int buttonType) override;
    
    // Color scheme
    static juce::Colour getBackgroundDark() { return juce::Colour(0xff1c1f23); }
    static juce::Colour getBackgroundMedium() { return juce::Colour(0xff2a2d32); }
    static juce::Colour getBackgroundLight() { return juce::Colour(0xff3a3d42); }
    static juce::Colour getAccentTeal() { return juce::Colour(0xff00ff88); }
    static juce::Colour getAccentAmber() { return juce::Colour(0xffffaa00); }
    static juce::Colour getAccentBlue() { return juce::Colour(0xff4a9eff); }
    static juce::Colour getAccentRed() { return juce::Colour(0xffff4d4d); }
    static juce::Colour getTextPrimary() { return juce::Colour(0xffffffff); }
    static juce::Colour getTextSecondary() { return juce::Colour(0xffaaaaaa); }
    static juce::Colour getTextTertiary() { return juce::Colour(0xff888888); }
    
private:
    juce::Font getButtonFont(juce::TextButton& button);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NomadLookAndFeel)
};
