#include "TransportComponent.h"
#include "../Audio/AudioEngine.h"

TransportComponent::TransportComponent(TransportController& transport)
    : transportController(transport),
      playButton("Play"),
      stopButton("Stop"),
      recordButton("Record")
{
    // Load SVG icons
    loadIcons();
    
    // Purple theme color
    juce::Colour purpleGlow(0xffa855f7);
    
    // Setup play button - purple theme
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a).withAlpha(0.3f));
    playButton.setColour(juce::TextButton::buttonOnColourId, purpleGlow.withAlpha(0.3f));
    playButton.setIconColour(purpleGlow.withAlpha(0.7f));
    playButton.setIconColourActive(purpleGlow);
    playButton.setTooltip("Play/Pause (Space)");
    playButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    playButton.onClick = [this]
    {
        // Toggle between play and pause (don't reset position)
        transportController.togglePlayPause();
    };
    addAndMakeVisible(playButton);
    
    // Setup stop button - purple theme
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a).withAlpha(0.3f));
    stopButton.setIconColour(purpleGlow.withAlpha(0.7f));
    stopButton.setTooltip("Stop");
    stopButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    stopButton.onClick = [this]
    {
        transportController.stop();
    };
    addAndMakeVisible(stopButton);
    
    // Setup record button - keep red but with purple tint
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a).withAlpha(0.3f));
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4d4d));
    recordButton.setIconColour(juce::Colour(0xffff6666).withAlpha(0.7f));
    recordButton.setIconColourActive(juce::Colours::white);
    recordButton.setTooltip("Record");
    recordButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    recordButton.onClick = [this]
    {
        if (transportController.isRecording())
            transportController.stop();
        else
            transportController.record();
    };
    addAndMakeVisible(recordButton);
    
    // Setup position display - compact, modern clock-like style with frosted background
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::plain));
    positionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa855f7)); // Purple
    positionLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    positionLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    positionLabel.setInterceptsMouseClicks(false, false); // Let parent handle clicks
    positionLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(positionLabel);
    
    // Setup time format label (clickable to switch between b:s:t and m:s:cs)
    timeFormatLabel.setJustificationType(juce::Justification::centredLeft);
    timeFormatLabel.setFont(juce::Font(9.0f, juce::Font::plain));
    timeFormatLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    timeFormatLabel.setText("b:s:t", juce::dontSendNotification);
    timeFormatLabel.setInterceptsMouseClicks(false, false); // Let parent handle clicks
    timeFormatLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(timeFormatLabel);
    
    // Setup tempo label
    tempoLabel.setText("BPM:", juce::dontSendNotification);
    tempoLabel.setJustificationType(juce::Justification::centredRight);
    tempoLabel.setFont(juce::Font(10.0f));
    tempoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    addAndMakeVisible(tempoLabel);
    
    // Setup tempo value display - editable with purple theme (smaller)
    tempoValueLabel.setJustificationType(juce::Justification::centredRight);
    tempoValueLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    tempoValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    tempoValueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    tempoValueLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    tempoValueLabel.setColour(juce::Label::textWhenEditingColourId, juce::Colour(0xffa855f7)); // Purple when editing
    tempoValueLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
    tempoValueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xffa855f7)); // Purple outline
    tempoValueLabel.setEditable(true, true, false);
    tempoValueLabel.setMouseCursor(juce::MouseCursor::IBeamCursor);
    tempoValueLabel.onTextChange = [this]
    {
        validateAndSetTempo(tempoValueLabel.getText());
    };
    addAndMakeVisible(tempoValueLabel);
    
    // Setup tempo increment button
    tempoUpButton.setButtonText("+");
    tempoUpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a).withAlpha(0.3f));
    tempoUpButton.setColour(juce::TextButton::textColourOffId, purpleGlow.withAlpha(0.7f));
    tempoUpButton.setColour(juce::TextButton::textColourOnId, purpleGlow);
    tempoUpButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    tempoUpButton.onClick = [this]() { incrementTempo(); };
    addAndMakeVisible(tempoUpButton);
    
    // Setup tempo decrement button
    tempoDownButton.setButtonText("-");
    tempoDownButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a).withAlpha(0.3f));
    tempoDownButton.setColour(juce::TextButton::textColourOffId, purpleGlow.withAlpha(0.7f));
    tempoDownButton.setColour(juce::TextButton::textColourOnId, purpleGlow);
    tempoDownButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    tempoDownButton.onClick = [this]() { decrementTempo(); };
    addAndMakeVisible(tempoDownButton);
    
    // Setup pattern mode button
    patternModeButton.setButtonText("PAT");
    patternModeButton.setColour(juce::TextButton::buttonColourId, purpleGlow.withAlpha(0.3f));
    patternModeButton.setColour(juce::TextButton::buttonOnColourId, purpleGlow);
    patternModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    patternModeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    patternModeButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    patternModeButton.setClickingTogglesState(true);
    patternModeButton.setToggleState(true, juce::dontSendNotification); // Default to pattern mode
    patternModeButton.setTooltip("Pattern Mode - Play sequencer patterns");
    patternModeButton.onClick = [this]() {
        if (audioEngine && patternModeButton.getToggleState())
        {
            audioEngine->setPlaybackMode(AudioEngine::PlaybackMode::Pattern);
            songModeButton.setToggleState(false, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(patternModeButton);
    
    // Setup song mode button
    songModeButton.setButtonText("SONG");
    songModeButton.setColour(juce::TextButton::buttonColourId, purpleGlow.withAlpha(0.3f));
    songModeButton.setColour(juce::TextButton::buttonOnColourId, purpleGlow);
    songModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    songModeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    songModeButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    songModeButton.setClickingTogglesState(true);
    songModeButton.setTooltip("Song Mode - Play playlist arrangement");
    songModeButton.onClick = [this]() {
        if (audioEngine && songModeButton.getToggleState())
        {
            audioEngine->setPlaybackMode(AudioEngine::PlaybackMode::Song);
            patternModeButton.setToggleState(false, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(songModeButton);
    
    // Register as listener
    transportController.addListener(this);
    
    // Initialize displays
    updateButtonStates();
    updatePositionDisplay();
    updateTempoDisplay();
    
    // Start timer for position updates and animations
    startTimer(33); // Update ~30 times per second for smooth animations
    
    // Register with GPU context manager
    GPUContextManager::getInstance().registerComponent(this);
    GPUContextManager::getInstance().setComponentRenderingActive(this, true);
}

TransportComponent::~TransportComponent()
{
    // Unregister from GPU context manager
    GPUContextManager::getInstance().unregisterComponent(this);
    
    stopTimer();
    transportController.removeListener(this);
}

void TransportComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    juce::Colour purpleGlow(0xffa855f7);
    
    // Dark background
    g.fillAll(juce::Colour(0xff151618));
    
    // Draw frosted background for time counter - slim with vertical gaps
    auto positionBounds = positionLabel.getBounds().toFloat().reduced(4, 12); // More vertical reduction for gaps
    
    // Frosted glass effect
    g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.6f));
    g.fillRoundedRectangle(positionBounds, 4.0f);
    
    // Purple glow outline
    g.setColour(purpleGlow.withAlpha(0.3f));
    g.drawRoundedRectangle(positionBounds.expanded(1), 4.0f, 2.0f);
    
    g.setColour(purpleGlow.withAlpha(0.5f));
    g.drawRoundedRectangle(positionBounds, 4.0f, 1.0f);
    
    // Draw button hover glows
    drawButtonHoverGlow(g, stopButton, purpleGlow);
    drawButtonHoverGlow(g, playButton, purpleGlow);
    drawButtonHoverGlow(g, recordButton, juce::Colour(0xffff4d4d)); // Keep record red
    
    // Draw record glow when recording
    drawRecordGlow(g);
}

void TransportComponent::resized()
{
    auto bounds = getLocalBounds();
    auto fullWidth = bounds.getWidth();
    auto fullHeight = bounds.getHeight();
    
    // Smaller buttons like reference
    int buttonSize = 28;
    int buttonSpacing = 4;
    
    // Center the transport buttons vertically
    auto buttonY = (fullHeight - buttonSize) / 2;
    
    // Left side: transport buttons - left aligned with small margin
    int leftMargin = 12;
    stopButton.setBounds(leftMargin, buttonY, buttonSize, buttonSize);
    playButton.setBounds(leftMargin + buttonSize + buttonSpacing, buttonY, buttonSize, buttonSize);
    recordButton.setBounds(leftMargin + (buttonSize + buttonSpacing) * 2, buttonY, buttonSize, buttonSize);
    
    // Mode buttons after transport buttons
    int modeButtonWidth = 45;
    int modeButtonHeight = 20;
    int modeButtonY = (fullHeight - modeButtonHeight) / 2;
    int modeButtonX = leftMargin + (buttonSize + buttonSpacing) * 3 + 12;
    
    patternModeButton.setBounds(modeButtonX, modeButtonY, modeButtonWidth, modeButtonHeight);
    songModeButton.setBounds(modeButtonX + modeButtonWidth + 4, modeButtonY, modeButtonWidth, modeButtonHeight);
    
    // Center: position display - more compact
    auto positionWidth = 90;
    auto positionX = (fullWidth - positionWidth) / 2;
    positionLabel.setBounds(positionX, 0, positionWidth, fullHeight);
    
    // Time format label - superscript style positioned to the right of time display
    auto formatWidth = 40;
    auto formatHeight = 14;
    timeFormatLabel.setBounds(positionX + positionWidth + 2, 2, formatWidth, formatHeight);
    
    // Right side: tempo controls - compact, right aligned
    int rightMargin = 12;
    int tempoButtonSize = 20;
    int tempoButtonSpacing = 2;
    auto tempoValueWidth = 45; // Smaller since we removed "BPM" text
    auto tempoLabelWidth = 35;
    
    // Calculate total width needed for tempo controls
    int totalTempoWidth = tempoLabelWidth + tempoValueWidth + (tempoButtonSize * 2) + (tempoButtonSpacing * 2);
    int tempoStartX = fullWidth - totalTempoWidth - rightMargin;
    
    // Layout tempo controls from left to right
    tempoLabel.setBounds(tempoStartX, 0, tempoLabelWidth, fullHeight);
    tempoStartX += tempoLabelWidth;
    
    tempoValueLabel.setBounds(tempoStartX, 0, tempoValueWidth, fullHeight);
    tempoStartX += tempoValueWidth + tempoButtonSpacing;
    
    tempoDownButton.setBounds(tempoStartX, buttonY, tempoButtonSize, tempoButtonSize);
    tempoStartX += tempoButtonSize + tempoButtonSpacing;
    
    tempoUpButton.setBounds(tempoStartX, buttonY, tempoButtonSize, tempoButtonSize);
}

void TransportComponent::transportStateChanged(TransportController::State newState)
{
    juce::ignoreUnused(newState);
    juce::MessageManager::callAsync([this]
    {
        updateButtonStates();
    });
}

void TransportComponent::transportPositionChanged(double positionInBeats)
{
    juce::ignoreUnused(positionInBeats);
    // Position is updated via timer for smoother display
}

void TransportComponent::tempoChanged(double newTempo)
{
    juce::ignoreUnused(newTempo);
    juce::MessageManager::callAsync([this]
    {
        updateTempoDisplay();
    });
}

void TransportComponent::timerCallback()
{
    updatePositionDisplay();
    
    // Update record pulse animation
    if (transportController.isRecording())
    {
        recordPulsePhase += 0.1f;
        if (recordPulsePhase > juce::MathConstants<float>::twoPi)
            recordPulsePhase -= juce::MathConstants<float>::twoPi;
        repaint(recordButton.getBounds().expanded(20));
    }
}

void TransportComponent::drawButtonHoverGlow(juce::Graphics& g, const IconButton& button, juce::Colour glowColour)
{
    if (!button.isMouseOver())
        return;
    
    auto bounds = button.getBounds().toFloat();
    auto center = bounds.getCentre();
    
    // Hover glow effect
    float glowRadius = 20.0f;
    
    juce::ColourGradient glow(
        glowColour.withAlpha(0.4f), center.x, center.y,
        glowColour.withAlpha(0.0f), center.x + glowRadius, center.y,
        true);
    
    g.setGradientFill(glow);
    g.fillEllipse(bounds.expanded(glowRadius));
}

void TransportComponent::drawRecordGlow(juce::Graphics& g)
{
    if (!transportController.isRecording())
        return;
    
    auto bounds = recordButton.getBounds().toFloat();
    auto center = bounds.getCentre();
    
    // Pulsing glow effect
    float pulseAlpha = 0.3f + 0.3f * std::sin(recordPulsePhase);
    float glowRadius = 30.0f + 10.0f * std::sin(recordPulsePhase);
    
    juce::ColourGradient glow(
        juce::Colour(0xffff4d4d).withAlpha(pulseAlpha), center.x, center.y,
        juce::Colour(0xffff4d4d).withAlpha(0.0f), center.x + glowRadius, center.y,
        true);
    
    g.setGradientFill(glow);
    g.fillEllipse(bounds.expanded(glowRadius));
}

void TransportComponent::setAudioEngine(AudioEngine* engine)
{
    audioEngine = engine;
}

void TransportComponent::incrementTempo()
{
    double currentTempo = transportController.getTempo();
    double newTempo = juce::jmin(999.0, currentTempo + 1.0);
    transportController.setTempo(newTempo);
}

void TransportComponent::decrementTempo()
{
    double currentTempo = transportController.getTempo();
    double newTempo = juce::jmax(20.0, currentTempo - 1.0);
    transportController.setTempo(newTempo);
}

void TransportComponent::validateAndSetTempo(const juce::String& text)
{
    // Remove " BPM" suffix if present
    auto cleanText = text.upToFirstOccurrenceOf(" ", false, false);
    double newTempo = cleanText.getDoubleValue();
    
    // Validate range (20-999 BPM)
    if (newTempo >= 20.0 && newTempo <= 999.0)
    {
        transportController.setTempo(newTempo);
    }
    else
    {
        // Revert to current tempo if invalid
        updateTempoDisplay();
    }
}

void TransportComponent::loadIcons()
{
    // Embed SVG data directly
    
    // Play icon
    juce::String playSvg = R"(
        <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
            <path d="M8 5v14l11-7z" fill="currentColor"/>
        </svg>
    )";
    playButton.loadSVGFromString(playSvg);
    
    // Stop icon
    juce::String stopSvg = R"(
        <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
            <rect x="6" y="6" width="12" height="12" rx="2" fill="currentColor"/>
        </svg>
    )";
    stopButton.loadSVGFromString(stopSvg);
    
    // Record icon
    juce::String recordSvg = R"(
        <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
            <circle cx="12" cy="12" r="6" fill="currentColor"/>
        </svg>
    )";
    recordButton.loadSVGFromString(recordSvg);
}

void TransportComponent::updateButtonStates()
{
    auto state = transportController.getState();
    
    // Update play button - switch icon between play and pause
    if (state == TransportController::State::Playing || state == TransportController::State::Recording)
    {
        // Load pause icon
        juce::String pauseSvg = R"(
            <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                <rect x="6" y="5" width="4" height="14" rx="1" fill="currentColor"/>
                <rect x="14" y="5" width="4" height="14" rx="1" fill="currentColor"/>
            </svg>
        )";
        playButton.loadSVGFromString(pauseSvg);
        playButton.setToggleState(true, juce::dontSendNotification);
    }
    else
    {
        // Load play icon
        juce::String playSvg = R"(
            <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                <path d="M8 5v14l11-7z" fill="currentColor"/>
            </svg>
        )";
        playButton.loadSVGFromString(playSvg);
        playButton.setToggleState(false, juce::dontSendNotification);
    }
    
    // Update record button
    if (state == TransportController::State::Recording)
    {
        recordButton.setToggleState(true, juce::dontSendNotification);
    }
    else
    {
        recordButton.setToggleState(false, juce::dontSendNotification);
    }
    
    repaint();
}

void TransportComponent::updatePositionDisplay()
{
    double position = transportController.getPosition();
    positionLabel.setText(formatPosition(position), juce::dontSendNotification);
}

void TransportComponent::updateTempoDisplay()
{
    double tempo = transportController.getTempo();
    // Just show the number, no "BPM" text
    tempoValueLabel.setText(juce::String(tempo, 1), juce::dontSendNotification);
}

void TransportComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Check if mouse is over tempo controls
    if (tempoValueLabel.getBounds().contains(event.getPosition()) ||
        tempoLabel.getBounds().contains(event.getPosition()))
    {
        // Scroll up = increase, scroll down = decrease
        if (wheel.deltaY > 0)
        {
            incrementTempo();
        }
        else if (wheel.deltaY < 0)
        {
            decrementTempo();
        }
    }
}

void TransportComponent::mouseDown(const juce::MouseEvent& event)
{
    // Check if clicking on position label or time format label
    if (positionLabel.getBounds().contains(event.getPosition()) || 
        timeFormatLabel.getBounds().contains(event.getPosition()))
    {
        // Toggle time format
        useMusicalTime = !useMusicalTime;
        timeFormatLabel.setText(useMusicalTime ? "b:s:t" : "m:s:cs", juce::dontSendNotification);
        updatePositionDisplay();
        repaint();
    }
}

juce::String TransportComponent::formatPosition(double beats)
{
    return useMusicalTime ? formatPositionMusical(beats) : formatPositionAbsolute(beats);
}

juce::String TransportComponent::formatPositionMusical(double beats)
{
    // Format as bars:beats:ticks (assuming 4/4 time signature)
    int beatsPerBar = 4;
    int ticksPerBeat = 960;
    
    int totalBeats = static_cast<int>(beats);
    int bars = totalBeats / beatsPerBar + 1; // 1-based
    int beatInBar = totalBeats % beatsPerBar + 1; // 1-based
    
    double fractionalBeat = beats - totalBeats;
    int ticks = static_cast<int>(fractionalBeat * ticksPerBeat);
    
    return juce::String(bars) + ":" + juce::String(beatInBar) + ":" + juce::String(ticks).paddedLeft('0', 3);
}

juce::String TransportComponent::formatPositionAbsolute(double beats)
{
    // Convert beats to time using BPM
    double seconds = transportController.beatsToSeconds(beats);
    
    int minutes = static_cast<int>(seconds / 60.0);
    double remainingSeconds = seconds - (minutes * 60.0);
    int secs = static_cast<int>(remainingSeconds);
    int centiseconds = static_cast<int>((remainingSeconds - secs) * 100.0);
    
    return juce::String(minutes) + ":" + juce::String(secs).paddedLeft('0', 2) + ":" + juce::String(centiseconds).paddedLeft('0', 2);
}
