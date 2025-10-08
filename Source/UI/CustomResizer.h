#pragma once

#include <JuceHeader.h>

/**
 * Custom resizer component with dark theme styling
 */
class CustomResizer : public juce::ResizableCornerComponent
{
public:
    CustomResizer(juce::Component* componentToResize, juce::ComponentBoundsConstrainer* constrainer)
        : juce::ResizableCornerComponent(componentToResize, constrainer)
    {
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        juce::Colour purpleGlow(0xffa855f7);
        
        // Draw subtle resize grip lines with purple tint
        g.setColour(purpleGlow.withAlpha(0.4f));
        
        // Draw three diagonal lines
        for (int i = 0; i < 3; ++i)
        {
            float offset = i * 4.0f;
            g.drawLine(bounds.getRight() - offset, bounds.getBottom(),
                      bounds.getRight(), bounds.getBottom() - offset, 1.5f);
        }
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomResizer)
};
