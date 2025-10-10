#pragma once

#include <JuceHeader.h>
#include "../Models/Pattern.h"
#include "../Models/PatternManager.h"
#include "../Audio/TransportController.h"
#include "FloatingWindow.h"
#include "WindowControlButton.h"

/**
 * SequencerView - FL Studio-style Pattern Editor / Channel Rack
 * A clean, minimal step sequencer with proper window behavior
 */
class SequencerView : public FloatingWindow,
                      public juce::Timer,
                      public TransportController::Listener
{
public:
    SequencerView(PatternManager& patternManager, TransportController& transport);
    ~SequencerView() override;
    
    // FloatingWindow overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    // Title bar bounds for dragging
    juce::Rectangle<int> getTitleBarBounds() const override;
    
    // Timer callback for playback animation
    void timerCallback() override;
    
    // TransportController::Listener
    void transportStateChanged(TransportController::State newState) override;
    void transportPositionChanged(double positionInBeats) override;
    
    // Pattern management
    void setActivePattern(int patternIndex);
    int getActivePattern() const { return activePatternIndex; }

private:
    // Core references
    PatternManager& patternManager;
    TransportController& transportController;
    
    // UI State
    int activePatternIndex = 0;
    int stepsPerPattern = 16;
    int visibleTracks = 8;
    
    // Grid dimensions
    static constexpr int titleBarHeight = 32;
    static constexpr int trackLabelWidth = 120;
    static constexpr int stepWidth = 32;
    static constexpr int trackHeight = 32;
    
    // Playback state
    double currentPlayPosition = 0.0;
    bool isPlaying = false;
    
    // Window controls
    WindowControlButton minimizeButton{"−", juce::Colour(0xff444444)};
    WindowControlButton maximizeButton{"□", juce::Colour(0xff444444)};
    WindowControlButton closeButton{"×", juce::Colour(0xffff6b9d)};
    
    // Helper methods
    bool isInGrid(int x, int y) const;
    int getStepAtX(int x) const;
    int getTrackAtY(int y) const;
    void toggleNoteAtPosition(int step, int track);
    void paintGrid(juce::Graphics& g);
    void paintPlayhead(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerView)
};
