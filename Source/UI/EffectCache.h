#pragma once

#include <JuceHeader.h>

/**
 * EffectCache: caches pre-rendered shadows/borders for a given window size.
 */
class EffectCache
{
public:
    void invalidate() { cachedImage = {}; lastSize = {}; }

    const juce::Image& getShadow(const juce::Rectangle<int>& rect, int radius, float opacity)
    {
        if (cachedImage.isNull() || lastSize != rect)
        {
            lastSize = rect;
            cachedImage = juce::Image(juce::Image::ARGB, rect.getWidth() + 16, rect.getHeight() + 16, true);
            juce::Graphics g(cachedImage);
            g.addTransform(juce::AffineTransform::translation(8.0f, 8.0f));
            juce::DropShadow ds(juce::Colours::black.withAlpha(opacity), radius, {});
            juce::Path p;
            p.addRoundedRectangle(rect.toFloat(), 8.0f);
            ds.drawForPath(g, p);
        }
        return cachedImage;
    }

private:
    juce::Image cachedImage;
    juce::Rectangle<int> lastSize;
};


