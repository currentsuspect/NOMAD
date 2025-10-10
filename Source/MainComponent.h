#pragma once

#include <JuceHeader.h>
#include "Audio/AudioEngine.h"
#include "UI/TransportComponent.h"
#include "UI/NomadLookAndFeel.h"
#include "UI/WindowControlButton.h"
#include "UI/CustomResizer.h"
#include "UI/FileBrowserComponent.h"
#include "UI/PlaylistComponent.h"
#include "UI/SequencerView.h"
#include "UI/MixerComponent.h"
#include "UI/GPUContextManager.h"
#include "UI/PerformanceMonitor.h"

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::FileDragAndDropTarget,
                      private juce::Timer,
                      public juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    // FileDragAndDropTarget interface
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    // KeyListener interface
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    
    // Focus management for floating windows
    void updateComponentFocus();

private:
    void showAudioSettings();
    void updateAudioInfo();
    void timerCallback() override;
    
    // Audio engine
    AudioEngine audioEngine;
    
    // Custom look and feel
    NomadLookAndFeel nomadLookAndFeel;
    
    // UI components
    juce::TextButton audioSettingsButton;
    WindowControlButton minimizeButton { WindowControlButton::Type::Minimize };
    WindowControlButton maximizeButton { WindowControlButton::Type::Maximize };
    WindowControlButton closeButton { WindowControlButton::Type::Close };
    juce::Label audioInfoLabel;
    TransportComponent transportComponent;
    FileExplorerPanel fileBrowser;
    PlaylistComponent playlistWindow;
    SequencerView sequencerView;
    MixerComponent mixerComponent;
    
    // Window dragging and resizing
    juce::ComponentDragger windowDragger;
    CustomResizer resizer;
    juce::ComponentBoundsConstrainer resizeConstraints;
    juce::Rectangle<int> draggableArea;
    
    // File browser resizing
    int fileBrowserWidth = 250;
    bool isDraggingDivider = false;
    juce::Rectangle<int> dividerArea;
    
    // Focus management for rendering optimization
    juce::Component* lastFocusedWindow = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
