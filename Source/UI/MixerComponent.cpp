#include "MixerComponent.h"
#include "../MainComponent.h"
#include "DragStateManager.h"
#include "NomadLookAndFeel.h"

// ============================================================================
// MixerControlButton Implementation
// ============================================================================

void MixerControlButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced(6);
    
    // Hover/press effect
    if (shouldDrawButtonAsHighlighted)
        g.setColour(juce::Colour(0xff2a2a2a));
    else
        g.setColour(juce::Colours::transparentBlack);
    g.fillRect(getLocalBounds().toFloat());
    
    // Draw symbol
    g.setColour(shouldDrawButtonAsDown ? juce::Colour(0xffffffff) : juce::Colour(0xff888888));
    
    if (type == Type::Minimize)
    {
        auto lineY = bounds.getCentreY();
        g.drawLine(bounds.getX(), lineY, bounds.getRight(), lineY, 1.5f);
    }
    else if (type == Type::Maximize)
    {
        g.drawRect(bounds, 1.5f);
    }
    else if (type == Type::Close)
    {
        g.drawLine(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 1.5f);
        g.drawLine(bounds.getRight(), bounds.getY(), bounds.getX(), bounds.getBottom(), 1.5f);
    }
}

// ============================================================================
// MixerChannelStrip Implementation
// ============================================================================

MixerChannelStrip::MixerChannelStrip(MixerChannel* channel, int channelIndex)
    : mixerChannel(channel), channelIndex(channelIndex)
{
    if (mixerChannel != nullptr)
    {
        faderValue = mixerChannel->getGain();
        panValue = mixerChannel->getPan();
        isMuted = mixerChannel->isMuted();
        isSolo = mixerChannel->isSolo();
    }
    
    startTimerHz(30); // Update meters at 30fps
}

void MixerChannelStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // FL Studio style: Deep purple/navy background
    g.setColour(juce::Colour(0xff1a1525));
    g.fillRect(bounds);
    
    // Right border separator (darker)
    g.setColour(juce::Colour(0xff0d0a15));
    g.fillRect(bounds.getRight() - 1, bounds.getY(), 1, bounds.getHeight());
    
    // Glowing purple circle indicator at top (FL Studio style)
    auto indicatorCenter = labelArea.getCentre().toFloat();
    indicatorCenter.y = labelArea.getY() + 12.0f;
    
    // Glow effect
    g.setColour(juce::Colour(0xffa855f7).withAlpha(0.3f));
    g.fillEllipse(indicatorCenter.x - 8, indicatorCenter.y - 8, 16, 16);
    
    // Inner circle
    g.setColour(juce::Colour(0xffa855f7).withAlpha(0.8f));
    g.fillEllipse(indicatorCenter.x - 5, indicatorCenter.y - 5, 10, 10);
    
    // Channel label (Insert X or Master)
    g.setColour(juce::Colour(0xff888888));
    g.setFont(10.0f);
    juce::String channelName = (channelIndex == 8) ? "Master" : "Insert " + juce::String(channelIndex + 1);
    g.drawText(channelName, labelArea.getX(), labelArea.getY() + 24, 
               labelArea.getWidth(), 20, juce::Justification::centred);
    
    // Draw meter (FL Studio style - thin vertical bar)
    if (meterArea.getWidth() > 0)
    {
        auto meterBounds = meterArea.toFloat().reduced(1.0f);
        
        // Meter background (dark purple)
        g.setColour(juce::Colour(0xff0d0a15));
        g.fillRect(meterBounds);
        
        // Peak meter with FL Studio colors
        float peakHeight = juce::jmap(peakLevel, -60.0f, 0.0f, 0.0f, meterBounds.getHeight());
        if (peakHeight > 0)
        {
            auto peakBounds = meterBounds.removeFromBottom(peakHeight);
            
            // FL Studio meter colors (purple/violet gradient)
            juce::Colour meterColor;
            if (peakLevel > -3.0f)
                meterColor = juce::Colour(0xffff6b9d); // Pink/red
            else if (peakLevel > -12.0f)
                meterColor = juce::Colour(0xffa855f7); // Purple
            else
                meterColor = juce::Colour(0xff7c3aed); // Deep purple
            
            // Gradient effect
            juce::ColourGradient gradient(meterColor.brighter(0.3f), peakBounds.getX(), peakBounds.getY(),
                                         meterColor, peakBounds.getX(), peakBounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRect(peakBounds);
        }
    }
    
    // Draw fader track (FL Studio style - thin vertical line with glow)
    auto faderTrack = faderArea.toFloat().reduced(faderArea.getWidth() * 0.4f, 0.0f);
    
    // Track background
    g.setColour(juce::Colour(0xff0d0a15));
    g.fillRoundedRectangle(faderTrack, 1.0f);
    
    // Active portion (below thumb) with purple glow
    float faderY = faderArea.getY() + (1.0f - faderValue) * faderArea.getHeight();
    auto activeTrack = faderTrack;
    activeTrack.setTop(faderY);
    g.setColour(juce::Colour(0xff7c3aed).withAlpha(0.4f));
    g.fillRoundedRectangle(activeTrack, 1.0f);
    
    // Draw fader thumb (FL Studio style - rounded rectangle with glow)
    auto thumbBounds = juce::Rectangle<float>(faderArea.getX(), faderY - 8.0f, 
                                               faderArea.getWidth(), 16.0f);
    
    // Outer glow
    if (isDraggingFader)
    {
        g.setColour(juce::Colour(0xffa855f7).withAlpha(0.5f));
        g.fillRoundedRectangle(thumbBounds.expanded(2), 6.0f);
    }
    
    // Thumb shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(thumbBounds.translated(0, 1), 4.0f);
    
    // Thumb body
    g.setColour(isDraggingFader ? juce::Colour(0xffa855f7) : juce::Colour(0xff4a4a5a));
    g.fillRoundedRectangle(thumbBounds, 4.0f);
    
    // Thumb highlight (top edge)
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.fillRoundedRectangle(thumbBounds.reduced(2, 2).removeFromTop(5), 2.0f);
    
    // Draw pan knob (FL Studio style - circular with purple arc indicator)
    auto panCenter = panArea.getCentre().toFloat();
    float panRadius = juce::jmin(panArea.getWidth(), panArea.getHeight()) * 0.32f;
    
    // Knob shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(panCenter.x - panRadius + 1, panCenter.y - panRadius + 1, 
                  panRadius * 2, panRadius * 2);
    
    // Knob background (dark purple)
    g.setColour(juce::Colour(0xff2a2535));
    g.fillEllipse(panCenter.x - panRadius, panCenter.y - panRadius, 
                  panRadius * 2, panRadius * 2);
    
    // Purple arc indicator (FL Studio style)
    float startAngle = -juce::MathConstants<float>::pi * 0.75f;
    float endAngle = startAngle + (panValue + 1.0f) * 0.5f * juce::MathConstants<float>::pi * 1.5f;
    
    juce::Path arcPath;
    arcPath.addCentredArc(panCenter.x, panCenter.y, panRadius - 2, panRadius - 2,
                         0.0f, startAngle, endAngle, true);
    
    g.setColour(isDraggingPan ? juce::Colour(0xffa855f7) : juce::Colour(0xff7c3aed));
    g.strokePath(arcPath, juce::PathStrokeType(2.5f));
    
    // Center dot
    g.setColour(juce::Colour(0xff888888));
    g.fillEllipse(panCenter.x - 2, panCenter.y - 2, 4, 4);
    
    // Pan arrows (◀ ▶)
    g.setColour(juce::Colour(0xff666666));
    g.setFont(10.0f);
    g.drawText("<", panArea.getX() - 8, panArea.getY(), 10, panArea.getHeight(), 
               juce::Justification::centred);
    g.drawText(">", panArea.getRight() - 2, panArea.getY(), 10, panArea.getHeight(), 
               juce::Justification::centred);
    
    // Draw mute button (FL Studio style - circular with glow)
    auto muteCenter = muteButtonArea.getCentre().toFloat();
    float buttonRadius = juce::jmin(muteButtonArea.getWidth(), muteButtonArea.getHeight()) * 0.35f;
    
    // Mute button glow
    if (isMuted)
    {
        g.setColour(juce::Colour(0xffff6b9d).withAlpha(0.4f));
        g.fillEllipse(muteCenter.x - buttonRadius - 3, muteCenter.y - buttonRadius - 3, 
                     (buttonRadius + 3) * 2, (buttonRadius + 3) * 2);
    }
    
    // Mute button body
    g.setColour(isMuted ? juce::Colour(0xffff6b9d) : juce::Colour(0xff2a2535));
    g.fillEllipse(muteCenter.x - buttonRadius, muteCenter.y - buttonRadius, 
                 buttonRadius * 2, buttonRadius * 2);
    
    // Mute text
    g.setColour(isMuted ? juce::Colours::white : juce::Colour(0xff666666));
    g.setFont(9.0f);
    g.drawText("M", muteButtonArea, juce::Justification::centred);
    
    // Draw solo button (FL Studio style - circular with purple glow)
    auto soloCenter = soloButtonArea.getCentre().toFloat();
    
    // Solo button glow
    if (isSolo)
    {
        g.setColour(juce::Colour(0xffa855f7).withAlpha(0.5f));
        g.fillEllipse(soloCenter.x - buttonRadius - 3, soloCenter.y - buttonRadius - 3, 
                     (buttonRadius + 3) * 2, (buttonRadius + 3) * 2);
    }
    
    // Solo button body
    g.setColour(isSolo ? juce::Colour(0xffa855f7) : juce::Colour(0xff2a2535));
    g.fillEllipse(soloCenter.x - buttonRadius, soloCenter.y - buttonRadius, 
                 buttonRadius * 2, buttonRadius * 2);
    
    // Solo text
    g.setColour(isSolo ? juce::Colours::white : juce::Colour(0xff666666));
    g.drawText("S", soloButtonArea, juce::Justification::centred);
    
    // Routing indicators at bottom (FL Studio style - small dots)
    g.setColour(juce::Colour(0xff7c3aed).withAlpha(0.6f));
    float dotY = soloButtonArea.getBottom() + 8;
    g.fillEllipse(muteCenter.x - 2, dotY, 4, 4);
    
    // Triangle indicator (routing direction)
    juce::Path triangle;
    triangle.addTriangle(muteCenter.x - 3, dotY + 8, 
                        muteCenter.x + 3, dotY + 8,
                        muteCenter.x, dotY + 12);
    g.fillPath(triangle);
}

void MixerChannelStrip::resized()
{
    auto bounds = getLocalBounds().reduced(6, 8); // More top/bottom padding
    
    // Layout from top to bottom (FL Studio style)
    labelArea = bounds.removeFromTop(44); // More space for indicator + label
    bounds.removeFromTop(8);
    
    // Meter on the left side
    meterArea = bounds.removeFromLeft(12);
    bounds.removeFromLeft(6);
    
    // Main fader area (takes most of the space, but leave room at bottom)
    faderArea = bounds.removeFromTop(bounds.getHeight() - 100);
    bounds.removeFromTop(10);
    
    // Pan knob
    panArea = bounds.removeFromTop(35);
    bounds.removeFromTop(12);
    
    // Mute and Solo buttons at bottom
    auto buttonArea = bounds.removeFromTop(22);
    muteButtonArea = buttonArea.removeFromLeft((buttonArea.getWidth() - 4) / 2);
    buttonArea.removeFromLeft(4);
    soloButtonArea = buttonArea;
    
    // Leave space at bottom for routing indicators (handled in paint)
}

void MixerChannelStrip::mouseDown(const juce::MouseEvent& event)
{
    if (faderArea.contains(event.getPosition()))
    {
        isDraggingFader = true;
        float newValue = 1.0f - (event.y - faderArea.getY()) / static_cast<float>(faderArea.getHeight());
        faderValue = juce::jlimit(0.0f, 1.0f, newValue);
        
        if (mixerChannel != nullptr)
            mixerChannel->setGain(faderValue);
        
        repaint();
    }
    else if (panArea.contains(event.getPosition()))
    {
        isDraggingPan = true;
    }
    else if (muteButtonArea.contains(event.getPosition()))
    {
        isMuted = !isMuted;
        if (mixerChannel != nullptr)
            mixerChannel->setMute(isMuted);
        repaint();
    }
    else if (soloButtonArea.contains(event.getPosition()))
    {
        isSolo = !isSolo;
        if (mixerChannel != nullptr)
        {
            mixerChannel->setSolo(isSolo);
            // Note: Parent mixer should call handleSoloStateChanged()
        }
        repaint();
    }
}

void MixerChannelStrip::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDraggingFader = false;
    isDraggingPan = false;
    repaint();
}

void MixerChannelStrip::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingFader)
    {
        float newValue = 1.0f - (event.y - faderArea.getY()) / static_cast<float>(faderArea.getHeight());
        faderValue = juce::jlimit(0.0f, 1.0f, newValue);
        
        if (mixerChannel != nullptr)
            mixerChannel->setGain(faderValue);
        
        repaint();
    }
    else if (isDraggingPan)
    {
        auto panCenter = panArea.getCentre();
        float dx = event.x - panCenter.x;
        float dy = event.y - panCenter.y;
        float angle = std::atan2(dx, -dy);
        panValue = juce::jlimit(-1.0f, 1.0f, angle / (juce::MathConstants<float>::pi * 0.5f));
        
        if (mixerChannel != nullptr)
            mixerChannel->setPan(panValue);
        
        repaint();
    }
}

void MixerChannelStrip::timerCallback()
{
    if (mixerChannel != nullptr)
    {
        peakLevel = mixerChannel->getPeakLevel();
        rmsLevel = mixerChannel->getRMSLevel();
        repaint(meterArea);
    }
}

void MixerChannelStrip::updateFromChannel()
{
    if (mixerChannel != nullptr)
    {
        faderValue = mixerChannel->getGain();
        panValue = mixerChannel->getPan();
        isMuted = mixerChannel->isMuted();
        isSolo = mixerChannel->isSolo();
        repaint();
    }
}

// ============================================================================
// MixerComponent Implementation
// ============================================================================

MixerComponent::MixerComponent(Mixer& mixer)
    : FloatingWindow("Mixer"), mixer(mixer)
{
    // Per-window setup handled by FloatingWindow

    // Setup window control buttons
    minimizeButton.onClick = [this] { minimize(); };
    addAndMakeVisible(minimizeButton);
    
    maximizeButton.onClick = [this] { toggleMaximize(); };
    addAndMakeVisible(maximizeButton);
    
    closeButton.onClick = [this] { setVisible(false); };
    addAndMakeVisible(closeButton);
    
    refreshChannels();
}

MixerComponent::~MixerComponent()
{
    // FloatingWindow handles context and listener cleanup
}

void MixerComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw title bar (FL Studio style) with rounded top corners (chrome drawn by base)
    auto headerBounds = juce::Rectangle<float>(0, 0, (float)getWidth(), (float)headerHeight);
    g.setColour(juce::Colour(0xff1a1525));
    g.fillRect(headerBounds); // Simple rect for now, rounded corners handled by main background
    
    // Title bar bottom border
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, (float)headerHeight - 1, (float)getWidth(), (float)headerHeight - 1, 1.0f);
    
    // Purple glow accent line
    g.setColour(juce::Colour(0xffa855f7).withAlpha(0.4f));
    g.drawLine(0, (float)headerHeight - 1, (float)getWidth(), (float)headerHeight - 1, 2.0f);
    
    // Title with purple color
    g.setColour(juce::Colour(0xffa855f7));
    g.setFont(juce::Font("Arial", 12.0f, juce::Font::plain));
    g.drawText("Mixer - Master", titleBarArea.reduced(12, 0), juce::Justification::centredLeft, true);
    
    // Window controls hint (minimize/maximize/close)
    g.setColour(juce::Colour(0xff666666));
    g.setFont(10.0f);
    g.drawText("- □ ×", getWidth() - 60, 0, 50, headerHeight, juce::Justification::centredRight);
}

void MixerComponent::resized()
{
    auto bounds = getLocalBounds();
    titleBarArea = bounds.removeFromTop(headerHeight);
    
    // Position window control buttons in title bar
    int buttonSize = 20;
    auto buttonY = (titleBarArea.getHeight() - buttonSize) / 2;
    closeButton.setBounds(titleBarArea.getRight() - buttonSize - 4, buttonY, buttonSize, buttonSize);
    maximizeButton.setBounds(titleBarArea.getRight() - (buttonSize * 2) - 6, buttonY, buttonSize, buttonSize);
    minimizeButton.setBounds(titleBarArea.getRight() - (buttonSize * 3) - 8, buttonY, buttonSize, buttonSize);
    
    // No padding - channels go edge to edge like FL Studio
    int x = 0;
    for (auto* strip : channelStrips)
    {
        strip->setBounds(x, bounds.getY(), channelStripWidth, bounds.getHeight());
        x += channelStripWidth;
    }
}

void MixerComponent::mouseDown(const juce::MouseEvent& event)
{
    // Let base class handle all drag logic (it now uses PlaylistComponent's proven method)
    FloatingWindow::mouseDown(event);
    
    // Update focus in parent
    if (auto* parent = dynamic_cast<MainComponent*>(getParentComponent()))
    {
        parent->updateComponentFocus();
    }
}

void MixerComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Let base class handle all drag logic (it now uses PlaylistComponent's proven method)
    FloatingWindow::mouseDrag(event);
}

void MixerComponent::mouseUp(const juce::MouseEvent& event)
{
    // Let base class handle all drag logic (it now uses PlaylistComponent's proven method)
    FloatingWindow::mouseUp(event);
    juce::ignoreUnused(event);
    
    // Exit lightweight mode when drag ends
    if (titleBarArea.contains(event.getMouseDownPosition()))
        DragStateManager::getInstance().exitLightweightMode();
}

void MixerComponent::setWorkspaceBounds(juce::Rectangle<int> bounds)
{
    workspaceBounds = bounds;
}

void MixerComponent::minimize()
{
    setVisible(false);
}

void MixerComponent::toggleMaximize()
{
    if (isMaximized)
    {
        setBounds(normalBounds);
        isMaximized = false;
    }
    else
    {
        normalBounds = getBounds();
        setBounds(workspaceBounds);
        isMaximized = true;
    }
}

void MixerComponent::refreshChannels()
{
    channelStrips.clear();
    
    for (int i = 0; i < mixer.getNumChannels(); ++i)
    {
        if (auto* channel = mixer.getChannel(i))
        {
            auto* strip = new MixerChannelStrip(channel, i);
            channelStrips.add(strip);
            addAndMakeVisible(strip);
        }
    }
    
    resized();
}

void MixerComponent::setRenderingActive(bool shouldRender)
{
    if (renderingActive == shouldRender)
        return;
    
    renderingActive = shouldRender;
    
    // Using per-window GL; no shared manager toggle required
    
    if (shouldRender)
    {
        // Resume meter updates for all channel strips
        for (auto* strip : channelStrips)
        {
            if (strip != nullptr)
                strip->startTimerHz(30);
        }
    }
    else
    {
        // Stop meter updates for all channel strips
        for (auto* strip : channelStrips)
        {
            if (strip != nullptr)
                strip->stopTimer();
        }
    }
}

// Removed stale override body that caused compile error
