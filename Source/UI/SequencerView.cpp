#include "SequencerView.h"
#include "MainComponent.h"

SequencerView::SequencerView(PatternManager& pm, TransportController& transport)
    : FloatingWindow("Pattern Editor", WindowType::Sequencer),
      patternManager(pm),
      transportController(transport)
{
    setSize(trackLabelWidth + (stepsPerPattern * stepWidth), 
            titleBarHeight + (visibleTracks * trackHeight));
    
    // Setup window control buttons
    minimizeButton.onClick = [this] { setVisible(false); };
    addAndMakeVisible(minimizeButton);
    
    maximizeButton.onClick = [this] { 
        // TODO: Implement maximize
    };
    addAndMakeVisible(maximizeButton);
    
    closeButton.onClick = [this] { setVisible(false); };
    addAndMakeVisible(closeButton);
    
    // Listen to transport
    transportController.addListener(this);
    
    // Start timer for playback animation (60 FPS)
    startTimerHz(60);
}

SequencerView::~SequencerView()
{
    transportController.removeListener(this);
    stopTimer();
}

juce::Rectangle<int> SequencerView::getTitleBarBounds() const
{
    return juce::Rectangle<int>(0, 0, getWidth(), titleBarHeight);
}

void SequencerView::resized()
{
    FloatingWindow::resized();
    
    auto titleBar = getTitleBarBounds();
    
    // Position window control buttons (right side of title bar)
    int buttonSize = 20;
    int buttonY = (titleBarHeight - buttonSize) / 2;
    int buttonSpacing = 4;
    
    closeButton.setBounds(titleBar.getRight() - buttonSize - 8, buttonY, buttonSize, buttonSize);
    maximizeButton.setBounds(closeButton.getX() - buttonSize - buttonSpacing, buttonY, buttonSize, buttonSize);
    minimizeButton.setBounds(maximizeButton.getX() - buttonSize - buttonSpacing, buttonY, buttonSize, buttonSize);
}

void SequencerView::paint(juce::Graphics& g)
{
    // Let base class paint window chrome (title bar, borders, shadow)
    FloatingWindow::paint(g);
    
    auto bounds = getLocalBounds();
    
    // Background
    g.setColour(juce::Colour(0xff1e1e2e));
    g.fillRect(bounds);
    
    // Title bar
    auto titleBar = getTitleBarBounds();
    g.setColour(juce::Colour(0xff2a2535));
    g.fillRect(titleBar);
    
    // Title text
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("Pattern Editor", titleBar.withTrimmedRight(100), juce::Justification::centredLeft);
    
    // Paint grid
    paintGrid(g);
    
    // Paint playhead if playing
    if (isPlaying)
    {
        paintPlayhead(g);
    }
}

void SequencerView::paintGrid(juce::Graphics& g)
{
    auto gridArea = getLocalBounds().withTrimmedTop(titleBarHeight);
    
    // Get active pattern
    auto pattern = patternManager.getPattern(activePatternIndex);
    if (!pattern)
        return;
    
    // Draw track labels background
    g.setColour(juce::Colour(0xff252535));
    g.fillRect(0, titleBarHeight, trackLabelWidth, gridArea.getHeight());
    
    // Draw vertical separator
    g.setColour(juce::Colour(0xff3a3a4a));
    g.drawLine(trackLabelWidth, titleBarHeight, trackLabelWidth, getHeight(), 1.0f);
    
    // Draw track labels and grid
    for (int track = 0; track < visibleTracks; ++track)
    {
        int y = titleBarHeight + (track * trackHeight);
        
        // Track label background (alternating)
        if (track % 2 == 0)
        {
            g.setColour(juce::Colour(0xff1a1a2a));
            g.fillRect(0, y, trackLabelWidth, trackHeight);
        }
        
        // Track label text
        g.setColour(juce::Colour(0xff9999aa));
        g.setFont(12.0f);
        juce::String trackName = "Track " + juce::String(track + 1);
        g.drawText(trackName, 8, y, trackLabelWidth - 16, trackHeight, juce::Justification::centredLeft);
        
        // Horizontal grid line
        g.setColour(juce::Colour(0xff2a2a3a));
        g.drawLine(0, y, getWidth(), y, 1.0f);
        
        // Draw steps for this track
        for (int step = 0; step < stepsPerPattern; ++step)
        {
            int x = trackLabelWidth + (step * stepWidth);
            
            // Step background (alternating 4-step groups like FL Studio)
            if ((step / 4) % 2 == 0)
            {
                g.setColour(juce::Colour(0xff1e1e2e));
            }
            else
            {
                g.setColour(juce::Colour(0xff252535));
            }
            g.fillRect(x, y, stepWidth, trackHeight);
            
            // Vertical grid line
            g.setColour(juce::Colour(0xff2a2a3a));
            g.drawLine(x, titleBarHeight, x, getHeight(), 1.0f);
            
            // Check if there's a note at this step
            bool hasNote = pattern->hasNoteAt(step, track);
            
            if (hasNote)
            {
                // Draw active step (FL Studio style - filled circle)
                int centerX = x + stepWidth / 2;
                int centerY = y + trackHeight / 2;
                int radius = 8;
                
                // Glow effect
                g.setColour(juce::Colour(0xffa855f7).withAlpha(0.3f));
                g.fillEllipse(centerX - radius - 2, centerY - radius - 2, 
                             (radius + 2) * 2, (radius + 2) * 2);
                
                // Main circle
                g.setColour(juce::Colour(0xffa855f7));
                g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
            }
        }
    }
    
    // Draw right and bottom borders
    g.setColour(juce::Colour(0xff3a3a4a));
    g.drawLine(trackLabelWidth + (stepsPerPattern * stepWidth), titleBarHeight, 
               trackLabelWidth + (stepsPerPattern * stepWidth), getHeight(), 1.0f);
}

void SequencerView::paintPlayhead(juce::Graphics& g)
{
    // Calculate playhead position
    auto pattern = patternManager.getPattern(activePatternIndex);
    if (!pattern)
        return;
    
    double stepPosition = currentPlayPosition;
    int totalSteps = stepsPerPattern;
    
    // Wrap to pattern length
    while (stepPosition >= totalSteps)
        stepPosition -= totalSteps;
    
    // Draw playhead line (FL Studio style - vertical line)
    int x = trackLabelWidth + static_cast<int>(stepPosition * stepWidth);
    
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.8f));
    g.drawLine(x, titleBarHeight, x, getHeight(), 2.0f);
}

void SequencerView::mouseDown(const juce::MouseEvent& event)
{
    // Let base class handle window dragging
    FloatingWindow::mouseDown(event);
    
    // Update focus
    if (auto* parent = dynamic_cast<MainComponent*>(getParentComponent()))
    {
        parent->updateComponentFocus();
    }
    
    // If clicking on title bar, base class handles it
    if (getTitleBarBounds().contains(event.getPosition()))
    {
        return;
    }
    
    // Check if clicking in grid
    if (!isInGrid(event.x, event.y))
        return;
    
    int step = getStepAtX(event.x);
    int track = getTrackAtY(event.y);
    
    if (step >= 0 && track >= 0)
    {
        toggleNoteAtPosition(step, track);
    }
}

void SequencerView::mouseUp(const juce::MouseEvent& event)
{
    // Let base class handle drag end
    FloatingWindow::mouseUp(event);
    juce::ignoreUnused(event);
}

void SequencerView::mouseDrag(const juce::MouseEvent& event)
{
    // Let base class handle window dragging
    FloatingWindow::mouseDrag(event);
    
    // If base class is dragging, don't do sequencer operations
    if (isDragging)
    {
        return;
    }
    
    // Paint mode: drag to add notes
    if (!isInGrid(event.x, event.y))
        return;
    
    int step = getStepAtX(event.x);
    int track = getTrackAtY(event.y);
    
    if (step >= 0 && track >= 0)
    {
        // Add note if not already there
        auto pattern = patternManager.getPattern(activePatternIndex);
        if (pattern && !pattern->hasNoteAt(step, track))
        {
            toggleNoteAtPosition(step, track);
        }
    }
}

void SequencerView::timerCallback()
{
    // Update playback position for animation
    if (isPlaying)
    {
        repaint();
    }
}

void SequencerView::transportStateChanged(TransportController::State newState)
{
    isPlaying = (newState == TransportController::State::Playing);
    repaint();
}

void SequencerView::transportPositionChanged(double positionInBeats)
{
    currentPlayPosition = positionInBeats;
    if (isPlaying)
    {
        repaint();
    }
}

void SequencerView::setActivePattern(int patternIndex)
{
    if (patternIndex >= 0 && patternIndex < patternManager.getPatternCount())
    {
        activePatternIndex = patternIndex;
        repaint();
    }
}

bool SequencerView::isInGrid(int x, int y) const
{
    return x >= trackLabelWidth && y >= titleBarHeight;
}

int SequencerView::getStepAtX(int x) const
{
    if (x < trackLabelWidth)
        return -1;
    
    int step = (x - trackLabelWidth) / stepWidth;
    if (step < 0 || step >= stepsPerPattern)
        return -1;
    
    return step;
}

int SequencerView::getTrackAtY(int y) const
{
    if (y < titleBarHeight)
        return -1;
    
    int track = (y - titleBarHeight) / trackHeight;
    if (track < 0 || track >= visibleTracks)
        return -1;
    
    return track;
}

void SequencerView::toggleNoteAtPosition(int step, int track)
{
    auto pattern = patternManager.getPattern(activePatternIndex);
    if (!pattern)
        return;
    
    if (pattern->hasNoteAt(step, track))
    {
        pattern->removeNoteAt(step, track);
    }
    else
    {
        // Add note with default velocity and pitch
        int pitch = 60 + track; // C4 + track offset
        pattern->addNoteAt(step, track, pitch, 1.0f, 0.8f);
    }
    
    repaint();
}
