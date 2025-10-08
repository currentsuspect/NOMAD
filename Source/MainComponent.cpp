#include "MainComponent.h"
#include "UI/AudioSettingsComponent.h"

MainComponent::MainComponent()
    : transportComponent(audioEngine.getTransportController()),
      sequencerView(audioEngine.getPatternManager(), audioEngine.getTransportController()),
      resizer(this, &resizeConstraints)
{
    // Set custom look and feel
    setLookAndFeel(&nomadLookAndFeel);
    
    // Initialize audio engine
    if (!audioEngine.initialize())
    {
        juce::Logger::writeToLog("Warning: Audio engine initialization failed");
    }
    
    // Setup resize constraints
    resizeConstraints.setMinimumSize(800, 600);
    resizeConstraints.setMaximumSize(3840, 2160); // 4K max
    
    // Setup resizer component
    addAndMakeVisible(resizer);
    
    // Setup audio settings button - more compact with purple theme
    audioSettingsButton.setButtonText("Settings");
    audioSettingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c1f23).withAlpha(0.5f));
    audioSettingsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffa855f7).withAlpha(0.3f));
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    audioSettingsButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffa855f7));
    audioSettingsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    audioSettingsButton.onClick = [this] { showAudioSettings(); };
    addAndMakeVisible(audioSettingsButton);
    
    // Setup window control buttons
    minimizeButton.onClick = [this] { 
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
            window->minimiseButtonPressed();
    };
    addAndMakeVisible(minimizeButton);
    
    maximizeButton.onClick = [this] { 
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
            window->maximiseButtonPressed();
    };
    addAndMakeVisible(maximizeButton);
    
    closeButton.onClick = [this] { 
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
            window->closeButtonPressed();
    };
    addAndMakeVisible(closeButton);
    
    // Setup audio info label with modern styling
    audioInfoLabel.setJustificationType(juce::Justification::centredLeft);
    audioInfoLabel.setFont(juce::Font(9.0f));
    audioInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    audioInfoLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(audioInfoLabel);
    
    // Setup playlist window
    playlistWindow.setAudioEngine(&audioEngine);
    addAndMakeVisible(playlistWindow);
    playlistWindow.toFront(false); // Bring to front but don't grab focus
    
    // Connect playlist clips to audio engine for playback
    audioEngine.setAudioClips(&playlistWindow.getAudioClips());
    
    // Setup transport component (will be on top of playlist)
    transportComponent.setAudioEngine(&audioEngine);
    addAndMakeVisible(transportComponent);
    
    // Setup file browser (will be on top of playlist)
    fileBrowser.setPlaylistComponent(&playlistWindow);
    addAndMakeVisible(fileBrowser);
    
    // Setup sequencer view
    sequencerView.setSequencerEngine(&audioEngine.getSequencerEngine());
    addAndMakeVisible(sequencerView);
    
    // Update audio info display
    updateAudioInfo();
    
    // Start a timer to periodically update (60 FPS for smooth playhead)
    startTimer(16);
    
    // Add key listener for global shortcuts
    addKeyListener(this);
    setWantsKeyboardFocus(true);
    
    setSize(1200, 800);
}

MainComponent::~MainComponent()
{
    stopTimer();
    setLookAndFeel(nullptr);
    audioEngine.shutdown();
}

void MainComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Purple theme color
    juce::Colour purpleGlow(0xffa855f7);
    
    // Very dark background
    g.fillAll(juce::Colour(0xff0d0e0f));
    
    // Compact top toolbar
    auto topBar = bounds.removeFromTop(45);
    g.setColour(juce::Colour(0xff151618));
    g.fillRect(topBar);
    
    // NOMAD title - modern, sleek font
    g.setFont(juce::Font("Arial", 13.0f, juce::Font::plain));
    auto titleBounds = topBar.reduced(12, 0);
    g.setColour(purpleGlow);
    g.drawText("NOMAD", titleBounds, juce::Justification::centredLeft, true);
    
    // Thin separator under top bar
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, 45, (float)getWidth(), 45, 1.0f);
    
    // Compact transport bar
    bounds.removeFromTop(50);
    
    // Thin separator under transport
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, 95, (float)getWidth(), 95, 1.0f);
    
    // Workspace area - blank with centered watermark
    auto workspace = bounds.removeFromBottom(bounds.getHeight() - 24);
    g.setColour(juce::Colour(0xff0d0e0f));
    g.fillRect(workspace);
    
    // Draw centered "NOMAD" watermark with subtle purple glow
    g.setFont(juce::Font("Arial", 72.0f, juce::Font::bold));
    auto watermarkBounds = workspace.withTrimmedLeft(fileBrowserWidth + 4);
    
    // Subtle glow effect
    g.setColour(purpleGlow.withAlpha(0.03f));
    g.drawText("NOMAD", watermarkBounds.translated(0, 2), juce::Justification::centred, true);
    g.drawText("NOMAD", watermarkBounds.translated(2, 0), juce::Justification::centred, true);
    g.drawText("NOMAD", watermarkBounds.translated(0, -2), juce::Justification::centred, true);
    g.drawText("NOMAD", watermarkBounds.translated(-2, 0), juce::Justification::centred, true);
    
    // Main watermark text
    g.setColour(purpleGlow.withAlpha(0.08f));
    g.drawText("NOMAD", watermarkBounds, juce::Justification::centred, true);
    
    // Status bar
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(0, getHeight() - 24, getWidth(), 24);
    
    // Status bar separator
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, (float)(getHeight() - 24), (float)getWidth(), (float)(getHeight() - 24), 1.0f);
    
    // Purple accent line at top of status bar with glow
    g.setColour(purpleGlow.withAlpha(0.2f));
    g.drawLine(0, (float)(getHeight() - 24), (float)getWidth(), (float)(getHeight() - 24), 3.0f);
    g.setColour(purpleGlow.withAlpha(0.4f));
    g.drawLine(0, (float)(getHeight() - 24), (float)getWidth(), (float)(getHeight() - 24), 1.5f);
    
    // Draw resizable divider between file browser and main area
    if (!dividerArea.isEmpty())
    {
        // Divider background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(dividerArea);
        
        // Purple glow on divider
        g.setColour(purpleGlow.withAlpha(0.3f));
        g.fillRect(dividerArea.reduced(1, 0));
        
        // Grip indicator in the middle
        auto gripArea = dividerArea.withSizeKeepingCentre(2, 40);
        g.setColour(purpleGlow.withAlpha(0.6f));
        g.fillRoundedRectangle(gripArea.toFloat(), 1.0f);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Compact top toolbar
    auto topBar = bounds.removeFromTop(45);
    
    // Window control buttons in top right - more compact
    int buttonSize = 32;
    int buttonSpacing = 0;
    closeButton.setBounds(topBar.removeFromRight(buttonSize));
    maximizeButton.setBounds(topBar.removeFromRight(buttonSize + buttonSpacing));
    minimizeButton.setBounds(topBar.removeFromRight(buttonSize + buttonSpacing));
    
    // Audio settings button to the left of window controls - more compact
    audioSettingsButton.setBounds(topBar.removeFromRight(70).reduced(6, 8));
    
    // The remaining topBar area is draggable
    draggableArea = topBar;
    
    // Compact transport component
    transportComponent.setBounds(bounds.removeFromTop(50));
    
    // Compact status bar at bottom
    audioInfoLabel.setBounds(bounds.removeFromBottom(24).reduced(8, 3));
    
    // File browser on the left (resizable)
    fileBrowser.setBounds(bounds.removeFromLeft(fileBrowserWidth));
    
    // Divider area (4px wide, draggable)
    dividerArea = bounds.removeFromLeft(4);
    
    // Position the resizer in the bottom-right corner (absolute positioning, doesn't affect layout)
    resizer.setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    
    // Split remaining area: sequencer on top, playlist below
    auto sequencerArea = bounds.removeFromTop(300);
    sequencerView.setBounds(sequencerArea);
    
    // Set workspace bounds for playlist window and position it
    playlistWindow.setWorkspaceBounds(bounds);
    if (!playlistWindow.getBounds().isEmpty())
    {
        // Keep existing position if already placed
        auto currentBounds = playlistWindow.getBounds();
        playlistWindow.setBounds(currentBounds.constrainedWithin(bounds));
    }
    else
    {
        // Initial positioning - center in workspace
        auto workspaceCenter = bounds.getCentre();
        playlistWindow.setCentrePosition(workspaceCenter.x, workspaceCenter.y);
    }
    
    // Main content area (for future use)
    // bounds now contains the remaining space for tracks, mixer, etc.
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
    // Check if clicking on divider
    if (dividerArea.contains(event.getPosition()))
    {
        isDraggingDivider = true;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }
    
    // Only allow dragging from the title bar area
    if (draggableArea.contains(event.getPosition()))
    {
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        {
            windowDragger.startDraggingComponent(window, event);
        }
    }
}

void MainComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Handle divider dragging
    if (isDraggingDivider)
    {
        fileBrowserWidth = juce::jlimit(150, 500, event.getPosition().x);
        resized();
        repaint(); // Force immediate repaint to update grid without trail
        return;
    }
    
    // Only allow dragging from the title bar area
    if (draggableArea.contains(event.getMouseDownPosition()))
    {
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        {
            windowDragger.dragComponent(window, event, nullptr);
        }
    }
}

void MainComponent::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (isDraggingDivider)
    {
        isDraggingDivider = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void MainComponent::mouseMove(const juce::MouseEvent& event)
{
    // Change cursor when hovering over divider
    if (dividerArea.contains(event.getPosition()))
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }
    else if (!isDraggingDivider)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void MainComponent::showAudioSettings()
{
    // Create a custom dialog window with audio settings
    auto* settingsComponent = new AudioSettingsComponent(audioEngine.getDeviceManager());
    settingsComponent->setSize(600, 450);
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(settingsComponent);
    options.dialogTitle = ""; // Empty title since we have custom title bar
    options.dialogBackgroundColour = juce::Colour(0xff0d0e0f);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = false;
    options.resizable = false;
    
    // Create the dialog window
    auto* dialogWindow = options.launchAsync();
    
    // Remove the default title bar completely
    if (dialogWindow != nullptr)
    {
        dialogWindow->setTitleBarHeight(0);
    }
}

void MainComponent::updateAudioInfo()
{
    juce::String info;
    info << audioEngine.getCurrentAudioDeviceName();
    info << "  •  " << juce::String(audioEngine.getSampleRate(), 0) << " Hz";
    info << "  •  " << juce::String(audioEngine.getBlockSize()) << " samples";
    
    if (audioEngine.getSampleRate() > 0)
    {
        double latencyMs = (audioEngine.getBlockSize() / audioEngine.getSampleRate()) * 1000.0;
        info << "  •  " << juce::String(latencyMs, 1) << " ms";
    }
    
    audioInfoLabel.setText(info, juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    // Update audio info less frequently (every ~1 second)
    static int frameCount = 0;
    if (++frameCount >= 60)
    {
        updateAudioInfo();
        frameCount = 0;
    }
    
    // Update playhead position every frame for smooth movement
    double currentPosition = audioEngine.getTransportController().getPosition();
    playlistWindow.setPlayheadPosition(currentPosition);
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);
    
    // Space bar for play/stop (not pause - resets to start)
    if (key == juce::KeyPress::spaceKey)
    {
        auto& transport = audioEngine.getTransportController();
        
        if (transport.isPlaying())
        {
            // Stop and reset to start
            transport.stop();
            transport.setPosition(0.0);
        }
        else
        {
            // Start from beginning
            transport.setPosition(0.0);
            transport.play();
        }
        
        return true;
    }
    
    return false;
}

// FileDragAndDropTarget implementation - forward to playlist
bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Always show interest to debug
    juce::ignoreUnused(files);
    return true;
}

void MainComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // ALWAYS forward to playlist, let it handle the filtering
    auto playlistPos = playlistWindow.getLocalPoint(this, juce::Point<int>(x, y));
    playlistWindow.filesDropped(files, playlistPos.x, playlistPos.y);
}
