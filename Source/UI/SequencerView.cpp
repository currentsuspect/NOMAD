#include "SequencerView.h"
#include "../Audio/SequencerEngine.h"

SequencerView::SequencerView(PatternManager& pm, TransportController& transport)
    : patternManager(pm)
    , transportController(transport)
{
    // Setup pattern selector
    patternSelector.onChange = [this]() {
        int selectedIndex = patternSelector.getSelectedItemIndex();
        if (selectedIndex >= 0)
        {
            auto patternIDs = patternManager.getAllPatternIDs();
            if (selectedIndex < patternIDs.size())
            {
                setActivePattern(patternIDs[selectedIndex]);
            }
        }
    };
    addAndMakeVisible(patternSelector);
    
    // Setup pattern length controls
    patternLengthLabel.setText("Length:", juce::dontSendNotification);
    patternLengthLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(patternLengthLabel);
    
    patternLengthSlider.setRange(4, 64, 1);
    patternLengthSlider.setValue(16);
    patternLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    patternLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    patternLengthSlider.onValueChange = [this]() {
        if (activePatternID >= 0)
        {
            if (auto* pattern = patternManager.getPattern(activePatternID))
            {
                pattern->setLength((int)patternLengthSlider.getValue());
                repaint();
            }
        }
    };
    addAndMakeVisible(patternLengthSlider);
    
    // Setup steps per beat controls
    stepsPerBeatLabel.setText("Steps/Beat:", juce::dontSendNotification);
    stepsPerBeatLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(stepsPerBeatLabel);
    
    stepsPerBeatSlider.setRange(1, 8, 1);
    stepsPerBeatSlider.setValue(4);
    stepsPerBeatSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    stepsPerBeatSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    stepsPerBeatSlider.onValueChange = [this]() {
        if (activePatternID >= 0)
        {
            if (auto* pattern = patternManager.getPattern(activePatternID))
            {
                pattern->setStepsPerBeat((int)stepsPerBeatSlider.getValue());
                repaint();
            }
        }
    };
    addAndMakeVisible(stepsPerBeatSlider);
    
    // Setup new pattern button
    newPatternButton.setButtonText("New Pattern");
    newPatternButton.onClick = [this]() { createNewPattern(); };
    addAndMakeVisible(newPatternButton);
    
    // Setup delete pattern button
    deletePatternButton.setButtonText("Delete Pattern");
    deletePatternButton.onClick = [this]() { deleteCurrentPattern(); };
    addAndMakeVisible(deletePatternButton);
    
    // Initialize track names (FL Studio style)
    trackNames = {
        "Kick",
        "Snare",
        "Hi-Hat",
        "Clap",
        "Tom",
        "Cymbal",
        "Perc 1",
        "Perc 2"
    };
    
    // Initialize mute/solo states
    trackMuteStates.resize(numTracks, false);
    trackSoloStates.resize(numTracks, false);
    
    // Setup horizontal scrollbar
    horizontalScrollbar.setAutoHide(false);
    horizontalScrollbar.addListener(this);
    addAndMakeVisible(horizontalScrollbar);
    
    // Create initial pattern if none exists
    if (patternManager.getPatternCount() == 0)
    {
        createNewPattern();
    }
    
    updatePatternSelector();
    
    // Register as transport listener
    transportController.addListener(this);
    
    // Start timer for playback animation (60 FPS)
    startTimer(16);
}

SequencerView::~SequencerView()
{
    transportController.removeListener(this);
}

void SequencerView::paint(juce::Graphics& g)
{
    // FL Studio style: Darker background
    g.fillAll(juce::Colour(0xff0d0d0d));
    
    // Draw grid
    drawGrid(g);
    
    // Draw step numbers
    drawStepNumbers(g);
    
    // Draw track labels
    drawTrackLabels(g);
    
    // Draw notes
    drawNotes(g);
    
    // Draw playback indicator
    if (transportController.isPlaying())
    {
        drawPlaybackIndicator(g);
    }
    
    // Scrollbar is drawn as a component, not manually
}

void SequencerView::resized()
{
    auto bounds = getLocalBounds();
    
    // Header area for controls
    auto headerArea = bounds.removeFromTop(headerHeight);
    headerArea.reduce(10, 10);
    
    // Pattern selector
    patternSelector.setBounds(headerArea.removeFromLeft(200));
    headerArea.removeFromLeft(10);
    
    // New/Delete buttons
    newPatternButton.setBounds(headerArea.removeFromLeft(100));
    headerArea.removeFromLeft(5);
    deletePatternButton.setBounds(headerArea.removeFromLeft(100));
    headerArea.removeFromLeft(20);
    
    // Pattern length controls
    patternLengthLabel.setBounds(headerArea.removeFromLeft(60));
    patternLengthSlider.setBounds(headerArea.removeFromLeft(150));
    headerArea.removeFromLeft(20);
    
    // Steps per beat controls
    stepsPerBeatLabel.setBounds(headerArea.removeFromLeft(80));
    stepsPerBeatSlider.setBounds(headerArea.removeFromLeft(150));
    
    // Horizontal scrollbar at bottom
    int scrollbarHeight = 12;
    auto scrollbarBounds = bounds.removeFromBottom(scrollbarHeight);
    scrollbarBounds.removeFromLeft(trackLabelWidth); // Start after track labels
    horizontalScrollbar.setBounds(scrollbarBounds);
    
    // Update scrollbar range based on pattern length
    if (activePatternID >= 0)
    {
        if (auto* pattern = patternManager.getPattern(activePatternID))
        {
            int patternLength = pattern->getLength();
            int totalWidth = patternLength * cellWidth;
            int visibleWidth = getWidth() - trackLabelWidth;
            
            horizontalScrollbar.setRangeLimits(0.0, totalWidth);
            horizontalScrollbar.setCurrentRange(scrollOffsetX, visibleWidth);
        }
    }
}

void SequencerView::mouseDown(const juce::MouseEvent& event)
{
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
    juce::ignoreUnused(event);
}

void SequencerView::mouseDrag(const juce::MouseEvent& event)
{
    if (!isInGrid(event.x, event.y))
        return;
    
    int step = getStepAtX(event.x);
    int track = getTrackAtY(event.y);
    
    if (step >= 0 && track >= 0)
    {
        // Add note on drag (paint mode)
        if (activePatternID >= 0)
        {
            if (auto* pattern = patternManager.getPattern(activePatternID))
            {
                Pattern::Note note;
                note.step = step;
                note.track = track;
                note.pitch = 60 + (numTracks - 1 - track) * 2; // Map tracks to pitches
                note.velocity = 0.8f;
                note.duration = 1;
                
                pattern->addNote(note);
                repaint();
            }
        }
    }
}

void SequencerView::timerCallback()
{
    if (transportController.isPlaying())
    {
        // Update playback position from transport controller
        playbackPosition = transportController.getPosition();
        repaint();
    }
}

void SequencerView::transportStateChanged(TransportController::State newState)
{
    juce::ignoreUnused(newState);
    repaint();
}

void SequencerView::transportPositionChanged(double positionInBeats)
{
    playbackPosition = positionInBeats;
}

void SequencerView::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved == &horizontalScrollbar)
    {
        scrollOffset = (int)newRangeStart;
        scrollOffsetX = newRangeStart;
        repaint();
    }
}

void SequencerView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(event, wheel);
    // Horizontal scrolling handled by scrollbar
}

bool SequencerView::keyPressed(const juce::KeyPress& key)
{
    if (activePatternID < 0)
        return false;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return false;
    
    // Delete key - clear pattern
    if (key == juce::KeyPress::deleteKey)
    {
        pattern->clearAllNotes();
        repaint();
        return true;
    }
    
    // Ctrl+C - copy pattern (placeholder)
    if (key == juce::KeyPress('c', juce::ModifierKeys::ctrlModifier, 0))
    {
        // TODO: Implement copy
        return true;
    }
    
    // Ctrl+V - paste pattern (placeholder)
    if (key == juce::KeyPress('v', juce::ModifierKeys::ctrlModifier, 0))
    {
        // TODO: Implement paste
        return true;
    }
    
    return false;
}

void SequencerView::showContextMenu(const juce::Point<int>& position)
{
    juce::PopupMenu menu;
    
    menu.addItem(1, "Clear Pattern", activePatternID >= 0);
    menu.addSeparator();
    menu.addItem(2, "Copy Pattern", activePatternID >= 0);
    menu.addItem(3, "Paste Pattern", activePatternID >= 0);
    menu.addSeparator();
    menu.addItem(4, "Randomize", activePatternID >= 0);
    menu.addItem(5, "Shift Left", activePatternID >= 0);
    menu.addItem(6, "Shift Right", activePatternID >= 0);
    
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(juce::Rectangle<int>(position, position)),
                      [this](int result)
                      {
                          handleContextMenuResult(result);
                      });
}

void SequencerView::handleContextMenuResult(int result)
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    switch (result)
    {
        case 1: // Clear Pattern
            pattern->clearAllNotes();
            repaint();
            break;
            
        case 2: // Copy Pattern
            // TODO: Implement copy
            break;
            
        case 3: // Paste Pattern
            // TODO: Implement paste
            break;
            
        case 4: // Randomize
            randomizePattern();
            break;
            
        case 5: // Shift Left
            shiftPatternLeft();
            break;
            
        case 6: // Shift Right
            shiftPatternRight();
            break;
    }
}

void SequencerView::randomizePattern()
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    // Clear existing notes
    pattern->clearAllNotes();
    
    // Add random notes
    juce::Random random;
    int patternLength = pattern->getLength();
    
    for (int track = 0; track < numTracks; ++track)
    {
        for (int step = 0; step < patternLength; ++step)
        {
            // 25% chance of note on each step
            if (random.nextFloat() < 0.25f)
            {
                Pattern::Note note;
                note.step = step;
                note.track = track;
                note.pitch = 60 + (numTracks - 1 - track) * 2;
                note.velocity = 0.5f + random.nextFloat() * 0.5f; // Random velocity 0.5-1.0
                note.duration = 1;
                
                pattern->addNote(note);
            }
        }
    }
    
    repaint();
}

void SequencerView::shiftPatternLeft()
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    auto notes = pattern->getAllNotes();
    pattern->clearAllNotes();
    
    // Shift all notes left by 1 step
    for (auto note : notes)
    {
        note.step = (note.step - 1 + pattern->getLength()) % pattern->getLength();
        pattern->addNote(note);
    }
    
    repaint();
}

void SequencerView::shiftPatternRight()
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    auto notes = pattern->getAllNotes();
    pattern->clearAllNotes();
    
    // Shift all notes right by 1 step
    for (auto note : notes)
    {
        note.step = (note.step + 1) % pattern->getLength();
        pattern->addNote(note);
    }
    
    repaint();
}

void SequencerView::setSequencerEngine(SequencerEngine* engine)
{
    sequencerEngine = engine;
    
    // Sync current active pattern if we have one
    if (activePatternID >= 0 && sequencerEngine)
    {
        sequencerEngine->setActivePattern(activePatternID);
    }
    else if (sequencerEngine)
    {
        // If no active pattern but we have patterns, select the first one
        auto patternIDs = patternManager.getAllPatternIDs();
        if (!patternIDs.empty())
        {
            setActivePattern(patternIDs[0]);
        }
    }
}

void SequencerView::setActivePattern(PatternManager::PatternID id)
{
    activePatternID = id;
    
    // Sync with sequencer engine
    if (sequencerEngine)
    {
        sequencerEngine->setActivePattern(id);
    }
    
    if (auto* pattern = patternManager.getPattern(id))
    {
        patternLengthSlider.setValue(pattern->getLength(), juce::dontSendNotification);
        stepsPerBeatSlider.setValue(pattern->getStepsPerBeat(), juce::dontSendNotification);
        
        // Update pattern name display
        patternNameLabel.setText(pattern->getName(), juce::dontSendNotification);
        
        // Update scrollbar range
        horizontalScrollbar.setRangeLimits(0.0, pattern->getLength());
        horizontalScrollbar.setCurrentRange(scrollOffset, juce::jmin(scrollOffset + maxVisibleSteps, pattern->getLength()));
    }
    
    repaint();
}

PatternManager::PatternID SequencerView::getActivePattern() const
{
    return activePatternID;
}

void SequencerView::setPatternLength(int steps)
{
    patternLengthSlider.setValue(steps);
}

void SequencerView::setStepsPerBeat(int steps)
{
    stepsPerBeatSlider.setValue(steps);
}

void SequencerView::drawGrid(juce::Graphics& g)
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    int patternLength = pattern->getLength();
    int stepsPerBeat = pattern->getStepsPerBeat();
    
    int gridX = trackLabelWidth;
    int gridY = headerHeight + stepNumberHeight;
    
    // FL Studio style: Draw grid cells with rounded buttons
    for (int track = 0; track < numTracks; ++track)
    {
        for (int step = 0; step < patternLength; ++step)
        {
            int x = gridX + step * cellWidth - (int)horizontalScrollOffset;
            int y = gridY + track * cellHeight;
            
            // Skip if not visible
            if (x + cellWidth < gridX || x > getWidth())
                continue;
            
            // FL Studio style: Darker background with subtle beat markers
            bool isBeatStart = (step % stepsPerBeat) == 0;
            if (isBeatStart)
                g.setColour(juce::Colour(0xff1f1f1f));
            else
                g.setColour(juce::Colour(0xff1a1a1a));
            
            g.fillRect(x, y, cellWidth, cellHeight);
            
            // Draw button slot (rounded rectangle)
            auto buttonBounds = juce::Rectangle<float>(x + 3, y + 3, cellWidth - 6, cellHeight - 6);
            g.setColour(juce::Colour(0xff0d0d0d));
            g.fillRoundedRectangle(buttonBounds, 3.0f);
            
            // Subtle border
            g.setColour(juce::Colour(0xff2a2a2a));
            g.drawRoundedRectangle(buttonBounds, 3.0f, 0.5f);
        }
    }
}

void SequencerView::drawStepNumbers(juce::Graphics& g)
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    int patternLength = pattern->getLength();
    int stepsPerBeat = pattern->getStepsPerBeat();
    
    int gridX = trackLabelWidth;
    int gridY = headerHeight;
    
    g.setFont(juce::Font(9.0f));
    
    // Draw step numbers (1, 2, 3, 4...)
    for (int step = 0; step < patternLength; ++step)
    {
        int x = gridX + step * cellWidth - (int)horizontalScrollOffset;
        
        // Skip if not visible
        if (x + cellWidth < gridX || x > getWidth())
            continue;
        
        // Highlight beat numbers
        bool isBeatStart = (step % stepsPerBeat) == 0;
        if (isBeatStart)
            g.setColour(juce::Colour(0xffa855f7)); // Purple for beats
        else
            g.setColour(juce::Colour(0xff666666)); // Gray for steps
        
        g.drawText(juce::String(step + 1), x, gridY, cellWidth, stepNumberHeight, 
                   juce::Justification::centred);
    }
}

void SequencerView::drawNotes(juce::Graphics& g)
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    int gridX = trackLabelWidth;
    int gridY = headerHeight + stepNumberHeight;
    
    auto notes = pattern->getAllNotes();
    
    // FL Studio style: Rounded, glowing buttons
    for (const auto& note : notes)
    {
        if (note.track >= numTracks)
            continue;
        
        // Only draw notes in visible range
        if (note.step < scrollOffset || note.step >= scrollOffset + maxVisibleSteps)
            continue;
        
        int visibleStep = note.step - scrollOffset;
        int x = gridX + visibleStep * cellWidth;
        int y = gridY + note.track * cellHeight;
        
        auto buttonBounds = juce::Rectangle<float>(x + 3, y + 3, cellWidth - 6, cellHeight - 6);
        
        // Velocity-based brightness (0.4 to 1.0 range for better visibility)
        float velocityBrightness = 0.4f + (note.velocity * 0.6f);
        
        // Glow effect - stronger for higher velocity
        g.setColour(juce::Colour(0xffa855f7).withAlpha(velocityBrightness * 0.4f));
        g.fillRoundedRectangle(buttonBounds.expanded(2), 4.0f);
        
        // Main button with velocity-based brightness
        g.setColour(juce::Colour(0xffa855f7).withBrightness(velocityBrightness));
        g.fillRoundedRectangle(buttonBounds, 3.0f);
        
        // Highlight - brighter for higher velocity
        g.setColour(juce::Colours::white.withAlpha(note.velocity * 0.5f));
        auto highlightBounds = buttonBounds.removeFromTop(buttonBounds.getHeight() * 0.4f);
        g.fillRoundedRectangle(highlightBounds, 3.0f);
        
        // Border - more prominent for higher velocity
        g.setColour(juce::Colours::white.withAlpha(0.4f + note.velocity * 0.4f));
        g.drawRoundedRectangle(buttonBounds, 3.0f, 1.0f);
    }
}

void SequencerView::drawPlaybackIndicator(juce::Graphics& g)
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    int stepsPerBeat = pattern->getStepsPerBeat();
    int patternLength = pattern->getLength();
    
    // Calculate current step based on playback position
    double patternLengthBeats = (double)patternLength / stepsPerBeat;
    double positionInPattern = std::fmod(playbackPosition, patternLengthBeats);
    int currentStep = (int)(positionInPattern * stepsPerBeat);
    
    if (currentStep >= 0 && currentStep < patternLength)
    {
        int gridX = trackLabelWidth;
        int gridY = headerHeight + stepNumberHeight;
        int x = gridX + currentStep * cellWidth;
        
        // FL Studio style: Small block indicator above the grid (below step numbers)
        int indicatorHeight = 6;
        int indicatorY = gridY - indicatorHeight - 1;
        
        // Draw the indicator block with purple glow
        juce::Colour purpleGlow(0xffa855f7);
        
        // Glow effect
        g.setColour(purpleGlow.withAlpha(0.3f));
        g.fillRect(x - 2, indicatorY - 2, cellWidth + 4, indicatorHeight + 4);
        
        // Main block
        g.setColour(purpleGlow);
        g.fillRect(x, indicatorY, cellWidth, indicatorHeight);
        
        // Highlight border
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawRect(x, indicatorY, cellWidth, indicatorHeight, 1);
        
        // Subtle column highlight
        g.setColour(purpleGlow.withAlpha(0.05f));
        g.fillRect(x, gridY, cellWidth, numTracks * cellHeight);
    }
}

void SequencerView::drawTrackLabels(juce::Graphics& g)
{
    int gridY = headerHeight + stepNumberHeight;
    
    g.setFont(juce::Font(11.0f));
    
    for (int track = 0; track < numTracks; ++track)
    {
        int y = gridY + track * cellHeight;
        
        // FL Studio style: Darker background
        g.setColour(juce::Colour(0xff151515));
        g.fillRect(0, y, trackLabelWidth, cellHeight);
        
        // Instrument name
        g.setColour(juce::Colour(0xff999999));
        juce::String label = track < trackNames.size() ? trackNames[track] : "Track " + juce::String(track + 1);
        g.drawText(label, 8, y, trackLabelWidth - 70, cellHeight, juce::Justification::centredLeft);
        
        // Volume slider background (drawn behind the actual slider)
        auto volumeSliderBounds = juce::Rectangle<int>(trackLabelWidth - 60, y + 2, 12, cellHeight - 4);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(volumeSliderBounds.toFloat(), 2.0f);
        
        // Mute button (M) - with glow when active
        auto muteButtonBounds = juce::Rectangle<int>(trackLabelWidth - 42, y + 4, 16, 16);
        bool isMuted = track < trackMuteStates.size() && trackMuteStates[track];
        if (isMuted)
        {
            // Glow effect
            g.setColour(juce::Colour(0xffef4444).withAlpha(0.3f));
            g.fillRoundedRectangle(muteButtonBounds.toFloat().expanded(2), 3.0f);
        }
        g.setColour(isMuted ? juce::Colour(0xffef4444) : juce::Colour(0xff444444));
        g.fillRoundedRectangle(muteButtonBounds.toFloat(), 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("M", muteButtonBounds, juce::Justification::centred);
        
        // Solo button (S) - with glow when active
        auto soloButtonBounds = juce::Rectangle<int>(trackLabelWidth - 24, y + 4, 16, 16);
        bool isSoloed = track < trackSoloStates.size() && trackSoloStates[track];
        if (isSoloed)
        {
            // Glow effect
            g.setColour(juce::Colour(0xff22c55e).withAlpha(0.3f));
            g.fillRoundedRectangle(soloButtonBounds.toFloat().expanded(2), 3.0f);
        }
        g.setColour(isSoloed ? juce::Colour(0xff22c55e) : juce::Colour(0xff444444));
        g.fillRoundedRectangle(soloButtonBounds.toFloat(), 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawText("S", soloButtonBounds, juce::Justification::centred);
        
        // Subtle separator
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawLine(0, y + cellHeight, trackLabelWidth, y + cellHeight, 0.5f);
    }
}

int SequencerView::getStepAtX(int x) const
{
    int gridX = trackLabelWidth;
    if (x < gridX)
        return -1;
    
    return (x - gridX) / cellWidth;
}

int SequencerView::getTrackAtY(int y) const
{
    int gridY = headerHeight + stepNumberHeight; // Fixed: account for step numbers
    if (y < gridY)
        return -1;
    
    return (y - gridY) / cellHeight;
}

bool SequencerView::isInGrid(int x, int y) const
{
    return x >= trackLabelWidth && y >= (headerHeight + stepNumberHeight); // Fixed: account for step numbers
}

void SequencerView::toggleNoteAtPosition(int step, int track)
{
    if (activePatternID < 0)
        return;
    
    auto* pattern = patternManager.getPattern(activePatternID);
    if (!pattern)
        return;
    
    // Check if note exists at this position
    auto notes = pattern->getNotesInRange(step, step + 1);
    bool noteExists = false;
    
    for (const auto& note : notes)
    {
        if (note.track == track)
        {
            noteExists = true;
            break;
        }
    }
    
    if (noteExists)
    {
        // Remove note
        pattern->removeNote(step, track);
    }
    else
    {
        // Add note
        Pattern::Note note;
        note.step = step;
        note.track = track;
        note.pitch = 60 + (numTracks - 1 - track) * 2; // Map tracks to pitches
        note.velocity = 0.8f;
        note.duration = 1;
        
        pattern->addNote(note);
    }
    
    repaint();
}

void SequencerView::updatePatternSelector()
{
    patternSelector.clear();
    
    auto patternIDs = patternManager.getAllPatternIDs();
    
    for (size_t i = 0; i < patternIDs.size(); ++i)
    {
        auto* pattern = patternManager.getPattern(patternIDs[i]);
        if (pattern)
        {
            patternSelector.addItem(pattern->getName(), (int)i + 1);
        }
    }
    
    // Select first pattern if available
    if (!patternIDs.empty() && activePatternID < 0)
    {
        setActivePattern(patternIDs[0]);
        patternSelector.setSelectedItemIndex(0);
    }
}

void SequencerView::createNewPattern()
{
    auto newPatternID = patternManager.createPattern("Pattern " + juce::String(patternManager.getPatternCount() + 1));
    updatePatternSelector();
    
    // Select the new pattern
    auto patternIDs = patternManager.getAllPatternIDs();
    for (size_t i = 0; i < patternIDs.size(); ++i)
    {
        if (patternIDs[i] == newPatternID)
        {
            patternSelector.setSelectedItemIndex((int)i);
            setActivePattern(newPatternID);
            break;
        }
    }
}

void SequencerView::deleteCurrentPattern()
{
    if (activePatternID < 0)
        return;
    
    // Don't delete if it's the last pattern
    if (patternManager.getPatternCount() <= 1)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Cannot Delete",
            "Cannot delete the last pattern. Create a new pattern first."
        );
        return;
    }
    
    patternManager.deletePattern(activePatternID);
    activePatternID = -1;
    
    updatePatternSelector();
    
    // Select first available pattern
    auto patternIDs = patternManager.getAllPatternIDs();
    if (!patternIDs.empty())
    {
        setActivePattern(patternIDs[0]);
        patternSelector.setSelectedItemIndex(0);
    }
}
