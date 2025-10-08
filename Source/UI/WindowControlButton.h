#pragma once

#include <JuceHeader.h>

/**
 * Custom button for window controls (minimize, maximize, close)
 * Draws symbols graphically instead of using Unicode characters
 */
class WindowControlButton : public juce::Button
{
public:
    enum class Type
    {
        Minimize,
        Maximize,
        Close
    };
    
    WindowControlButton(Type buttonType)
        : juce::Button(""), type(buttonType)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        juce::Colour purpleGlow(0xffa855f7);
        
        // Background and glow on hover
        if (shouldDrawButtonAsHighlighted)
        {
            if (type == Type::Close)
            {
                // Red glow for close button
                g.setColour(juce::Colour(0xffff4d4d).withAlpha(0.8f));
                g.fillRect(bounds);
            }
            else
            {
                // Purple glow for minimize/maximize
                auto center = bounds.getCentre();
                float glowRadius = 16.0f;
                
                juce::ColourGradient glow(
                    purpleGlow.withAlpha(0.3f), center.x, center.y,
                    purpleGlow.withAlpha(0.0f), center.x + glowRadius, center.y,
                    true);
                
                g.setGradientFill(glow);
                g.fillEllipse(bounds.expanded(glowRadius));
                
                // Subtle background
                g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.5f));
                g.fillRect(bounds);
            }
        }
        
        // Symbol color
        if (shouldDrawButtonAsHighlighted)
            g.setColour(juce::Colours::white);
        else
            g.setColour(juce::Colour(0xff888888));
        
        // Draw the symbol
        auto center = bounds.getCentre();
        auto symbolSize = 10.0f;
        
        switch (type)
        {
            case Type::Minimize:
                // Draw a horizontal line
                g.fillRect(center.x - symbolSize / 2, center.y - 1, symbolSize, 2.0f);
                break;
                
            case Type::Maximize:
                // Draw a square outline
                g.drawRect(center.x - symbolSize / 2, center.y - symbolSize / 2, 
                          symbolSize, symbolSize, 1.5f);
                break;
                
            case Type::Close:
                // Draw an X
                {
                    juce::Path cross;
                    float halfSize = symbolSize / 2;
                    cross.startNewSubPath(center.x - halfSize, center.y - halfSize);
                    cross.lineTo(center.x + halfSize, center.y + halfSize);
                    cross.startNewSubPath(center.x + halfSize, center.y - halfSize);
                    cross.lineTo(center.x - halfSize, center.y + halfSize);
                    g.strokePath(cross, juce::PathStrokeType(1.5f));
                }
                break;
        }
    }
    
private:
    Type type;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowControlButton)
};
