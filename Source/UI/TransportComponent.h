#pragma once

#include <JuceHeader.h>
#include "../Audio/TransportController.h"
#include "IconButton.h"

// Forward declaration
class AudioEngine;

/**
 * UI component for transport controls (play, stop, record) and tempo display/editing.
 */
class TransportComponent : public juce::Component,
                          public TransportController::Listener,
                          private juce::Timer
{
public:
    TransportComponent(TransportController& transport);
    ~TransportComponent() override;
    
    // Set audio engine for mode switching
    void setAudioEngine(class AudioEngine* engine);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    // TransportController::Listener
    void transportStateChanged(TransportController::State newState) override;
    void transportPositionChanged(double positionInBeats) override;
    void tempoChanged(double newTempo) override;
    
private:
    void timerCallback() override;
    void updateButtonStates();
    void updatePositionDisplay();
    void updateTempoDisplay();
    void drawButtonHoverGlow(juce::Graphics& g, const IconButton& button, juce::Colour glowColour);
    void drawRecordGlow(juce::Graphics& g);
    void loadIcons();
    
    TransportController& transportController;
    
    // Transport buttons
    IconButton playButton;
    IconButton stopButton;
    IconButton recordButton;
    
    // Position display
    juce::Label positionLabel;
    juce::Label timeFormatLabel;
    bool useMusicalTime = true; // true = b:s:t, false = m:s:cs
    juce::String formatPosition(double beats);
    juce::String formatPositionMusical(double beats);
    juce::String formatPositionAbsolute(double beats);
    
    // Tempo display and editing
    juce::Label tempoLabel;
    juce::Label tempoValueLabel;
    juce::TextEditor tempoEditor;
    juce::TextButton tempoUpButton;
    juce::TextButton tempoDownButton;
    bool isEditingTempo = false;
    
    // Playback mode buttons
    juce::TextButton patternModeButton;
    juce::TextButton songModeButton;
    class AudioEngine* audioEngine = nullptr;
    
    void startTempoEditing();
    void finishTempoEditing();
    void incrementTempo();
    void decrementTempo();
    void validateAndSetTempo(const juce::String& text);
    
    // Animation
    float recordPulsePhase = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
};
