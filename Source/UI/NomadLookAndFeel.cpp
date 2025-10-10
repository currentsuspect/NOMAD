#include "NomadLookAndFeel.h"

NomadLookAndFeel::NomadLookAndFeel()
{
    // Set default colors
    setColour(juce::ResizableWindow::backgroundColourId, getBackgroundDark());
    setColour(juce::TextButton::buttonColourId, getBackgroundMedium());
    setColour(juce::TextButton::textColourOffId, getTextSecondary());
    setColour(juce::Label::textColourId, getTextPrimary());
    
    // FL Studio style: Purple sliders instead of teal
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffa855f7)); // Purple
    setColour(juce::Slider::trackColourId, juce::Colour(0xff7c3aed)); // Deep purple
}

void NomadLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                           juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    // Make buttons circular if they're square-ish
    bool isCircular = std::abs(bounds.getWidth() - bounds.getHeight()) < 5.0f;
    
    // Get button state colors
    auto baseColour = backgroundColour;
    
    if (button.getToggleState())
    {
        // Button is active/toggled
        baseColour = button.findColour(juce::TextButton::buttonOnColourId);
    }
    
    // Apply interaction effects - modern flat style
    float alpha = 1.0f;
    if (shouldDrawButtonAsDown)
    {
        baseColour = baseColour.brighter(0.2f);
        bounds = bounds.reduced(2.0f); // Subtle press effect
        alpha = 0.9f;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = baseColour.brighter(0.15f);
        
        // Subtle hover glow
        if (isCircular)
        {
            g.setColour(baseColour.withAlpha(0.15f));
            g.fillEllipse(bounds.expanded(3.0f));
        }
        else
        {
            g.setColour(baseColour.withAlpha(0.15f));
            g.fillRoundedRectangle(bounds.expanded(2.0f), bounds.getHeight() * 0.5f);
        }
    }
    
    // Draw main button - flat, modern style
    g.setColour(baseColour.withAlpha(alpha));
    
    if (isCircular)
    {
        // Circular button
        g.fillEllipse(bounds);
        
        // Subtle border for definition
        g.setColour(baseColour.brighter(0.3f).withAlpha(0.3f));
        g.drawEllipse(bounds, 1.0f);
    }
    else
    {
        // Pill-shaped button
        float cornerSize = bounds.getHeight() * 0.5f;
        g.fillRoundedRectangle(bounds, cornerSize);
        
        // Subtle border for definition
        g.setColour(baseColour.brighter(0.3f).withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
}

void NomadLookAndFeel::drawButtonText(juce::Graphics& g,
                                     juce::TextButton& button,
                                     bool shouldDrawButtonAsHighlighted,
                                     bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto font = getButtonFont(button);
    g.setFont(font);
    
    auto textColour = button.getToggleState()
        ? button.findColour(juce::TextButton::textColourOnId)
        : button.findColour(juce::TextButton::textColourOffId);
    
    g.setColour(textColour);
    
    auto bounds = button.getLocalBounds();
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred, true);
}

void NomadLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const juce::Slider::SliderStyle style,
                                       juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style);
    
    auto trackWidth = juce::jmin(6.0f, (float)height * 0.25f);
    auto thumbWidth = 16.0f;
    auto thumbHeight = 20.0f;
    
    juce::Point<float> startPoint(x + width * 0.5f, y + height * 0.5f);
    juce::Point<float> endPoint(startPoint);
    
    if (slider.isHorizontal())
    {
        startPoint.x = (float)x;
        endPoint.x = (float)(x + width);
        startPoint.y = endPoint.y = y + height * 0.5f;
    }
    else
    {
        startPoint.y = (float)(y + height);
        endPoint.y = (float)y;
        startPoint.x = endPoint.x = x + width * 0.5f;
    }
    
    // Draw track background
    juce::Path track;
    track.startNewSubPath(startPoint);
    track.lineTo(endPoint);
    g.setColour(getBackgroundLight());
    g.strokePath(track, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Draw filled track
    juce::Path filledTrack;
    filledTrack.startNewSubPath(startPoint);
    
    juce::Point<float> thumbPoint;
    if (slider.isHorizontal())
        thumbPoint = {sliderPos, startPoint.y};
    else
        thumbPoint = {startPoint.x, sliderPos};
    
    filledTrack.lineTo(thumbPoint);
    
    auto trackColour = slider.findColour(juce::Slider::thumbColourId);
    g.setColour(trackColour);
    g.strokePath(filledTrack, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Draw thumb with glow
    auto thumbBounds = juce::Rectangle<float>(thumbWidth, thumbHeight).withCentre(thumbPoint);
    
    // Glow effect
    g.setColour(trackColour.withAlpha(0.3f));
    g.fillEllipse(thumbBounds.expanded(4.0f));
    
    // Thumb
    g.setColour(trackColour);
    g.fillRoundedRectangle(thumbBounds, 4.0f);
    
    // Thumb highlight
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillRoundedRectangle(thumbBounds.removeFromTop(thumbBounds.getHeight() * 0.5f), 4.0f);
}

void NomadLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        auto font = label.getFont();
        
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(font);
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        
        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                        juce::jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                        label.getMinimumHorizontalScale());
        
        // Only draw outline if it's not transparent
        auto outlineColour = label.findColour(juce::Label::outlineColourId);
        if (!outlineColour.isTransparent() && outlineColour.getAlpha() > 0)
        {
            g.setColour(outlineColour.withMultipliedAlpha(alpha));
            g.drawRect(label.getLocalBounds());
        }
    }
    else if (label.isEnabled())
    {
        // When editing, show a subtle outline
        g.setColour(getAccentBlue().withAlpha(0.5f));
        g.drawRect(label.getLocalBounds(), 1);
    }
}

juce::Font NomadLookAndFeel::getButtonFont(juce::TextButton& button)
{
    juce::ignoreUnused(button);
    return juce::Font(13.0f, juce::Font::bold);
}

void NomadLookAndFeel::drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                                 juce::Graphics& g,
                                                 int w, int h,
                                                 int titleSpaceX, int titleSpaceW,
                                                 const juce::Image* icon,
                                                 bool drawTitleTextOnLeft)
{
    juce::ignoreUnused(icon, drawTitleTextOnLeft);
    
    // Dark title bar background
    g.fillAll(juce::Colour(0xff151618));
    
    // Subtle separator at bottom
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, (float)h, (float)w, (float)h, 1.0f);
    
    // Draw title text
    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::Font(13.0f, juce::Font::plain));
    
    auto textW = juce::jmin(titleSpaceW, w - titleSpaceX - 100);
    g.drawText(window.getName(), titleSpaceX, 0, textW, h, juce::Justification::centredLeft, true);
}

juce::Button* NomadLookAndFeel::createDocumentWindowButton(int buttonType)
{
    auto* button = new juce::TextButton(juce::String(), juce::String());
    
    // Style the window buttons
    button->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    button->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    
    if (buttonType == juce::DocumentWindow::closeButton)
    {
        button->setButtonText("×");
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4d4d));
    }
    else if (buttonType == juce::DocumentWindow::minimiseButton)
    {
        button->setButtonText("−");
    }
    else if (buttonType == juce::DocumentWindow::maximiseButton)
    {
        button->setButtonText("□");
    }
    
    return button;
}
