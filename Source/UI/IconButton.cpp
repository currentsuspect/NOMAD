#include "IconButton.h"

IconButton::IconButton(const juce::String& buttonName)
    : juce::Button(buttonName)
{
    setClickingTogglesState(true);
}

void IconButton::loadSVG(const juce::File& svgFile)
{
    if (svgFile.existsAsFile())
    {
        auto svgXml = juce::parseXML(svgFile);
        if (svgXml != nullptr)
        {
            iconDrawable = juce::Drawable::createFromSVG(*svgXml);
            iconDrawableActive = juce::Drawable::createFromSVG(*svgXml);
            repaint();
        }
    }
}

void IconButton::loadSVGFromString(const juce::String& svgContent)
{
    auto svgXml = juce::parseXML(svgContent);
    if (svgXml != nullptr)
    {
        iconDrawable = juce::Drawable::createFromSVG(*svgXml);
        iconDrawableActive = juce::Drawable::createFromSVG(*svgXml);
        repaint();
    }
}

void IconButton::setIconColour(juce::Colour colour)
{
    iconColour = colour;
    repaint();
}

void IconButton::setIconColourActive(juce::Colour colour)
{
    iconColourActive = colour;
    repaint();
}

void IconButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto& lf = getLookAndFeel();
    lf.drawButtonBackground(g, *this,
                           findColour(getToggleState() ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId),
                           shouldDrawButtonAsHighlighted,
                           shouldDrawButtonAsDown);
    
    // Draw icon
    auto* drawableToUse = getToggleState() ? iconDrawableActive.get() : iconDrawable.get();
    
    if (drawableToUse != nullptr)
    {
        auto bounds = getLocalBounds().reduced(10).toFloat();
        
        // Clone the drawable so we don't modify the original
        auto clonedDrawable = drawableToUse->createCopy();
        
        // Apply color tint - replace all colors with our desired color
        auto colourToUse = getToggleState() ? iconColourActive : iconColour;
        clonedDrawable->replaceColour(juce::Colours::black, colourToUse);
        clonedDrawable->replaceColour(juce::Colours::white, colourToUse);
        clonedDrawable->replaceColour(juce::Colour(0xff4a9eff), colourToUse);
        clonedDrawable->replaceColour(juce::Colour(0xffff4d4d), colourToUse);
        clonedDrawable->replaceColour(juce::Colour(0xffaaaaaa), colourToUse);
        
        clonedDrawable->drawWithin(g, bounds, juce::RectanglePlacement::centred, 1.0f);
    }
}
