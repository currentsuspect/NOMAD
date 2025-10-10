#pragma once

#include <JuceHeader.h>
#include "MinimalScrollbar.h"
#include "../Models/AudioClip.h"
#include "GPUContextManager.h"
#include "DragStateManager.h"
#include "FloatingWindow.h"

/**
 * Simple button that draws window control symbols
 */
class PlaylistControlButton : public juce::Button
{
public:
    enum class Type { Minimize, Maximize, Close };
    
    PlaylistControlButton(Type t) : juce::Button(""), type(t) {}
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(6);
        
        // Hover/press effect
        if (shouldDrawButtonAsHighlighted)
            g.setColour(juce::Colour(0xff2a2a2a));
        else
            g.setColour(juce::Colours::transparentBlack);
        g.fillRect(getLocalBounds().toFloat());
        
        // Draw symbol - smaller and thinner
        g.setColour(shouldDrawButtonAsDown ? juce::Colour(0xffffffff) : juce::Colour(0xff888888));
        
        if (type == Type::Minimize)
        {
            // Draw horizontal line
            auto lineY = bounds.getCentreY();
            g.drawLine(bounds.getX(), lineY, bounds.getRight(), lineY, 1.5f);
        }
        else if (type == Type::Maximize)
        {
            // Draw square
            g.drawRect(bounds, 1.5f);
        }
        else if (type == Type::Close)
        {
            // Draw X
            g.drawLine(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 1.5f);
            g.drawLine(bounds.getRight(), bounds.getY(), bounds.getX(), bounds.getBottom(), 1.5f);
        }
    }
    
private:
    Type type;
};

/**
 * Dockable playlist window component
 */
class PlaylistComponent : public FloatingWindow,
                          public juce::FileDragAndDropTarget,
                          private juce::Timer
{
public:
    PlaylistComponent();
    
    // GPU acceleration
    void enableOpenGL();
    ~PlaylistComponent() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    // FileDragAndDropTarget interface
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    
    void setWorkspaceBounds(juce::Rectangle<int> bounds);
    void minimize();
    void toggleMaximize();
    void setDocked(bool shouldBeDocked); // Set whether playlist is docked or floating
    
    // Rendering control for performance
    void setRenderingActive(bool shouldRender);
    bool isRenderingActive() const { return renderingActive; }
    
    // Playback
    void setPlayheadPosition(double timeInBeats);
    double getPlayheadPosition() const { return playheadPosition; }
    const std::vector<AudioClip>& getAudioClips() const { return audioClips; }
    
    // Set audio engine for mode checking
    void setAudioEngine(class AudioEngine* engine);
    
private:
    void checkSnapToCorner();
    void timerCallback() override;
    
    // Lightweight drag mode methods
    void enterLightweightMode();
    void exitLightweightMode();
    
    juce::Rectangle<int> titleBarArea;
    juce::Rectangle<int> workspaceBounds;
    juce::Rectangle<int> normalBounds; // Store bounds before maximize
    juce::Colour purpleGlow{0xffa855f7};
    
    PlaylistControlButton minimizeButton{PlaylistControlButton::Type::Minimize};
    PlaylistControlButton maximizeButton{PlaylistControlButton::Type::Maximize};
    PlaylistControlButton closeButton{PlaylistControlButton::Type::Close};
    
    bool isMaximized = false;
    bool isMinimized = false;
    bool isDragging = false;
    bool isDocked = false; // Whether playlist is docked (not floating)
    
    // Lightweight drag mode for performance
    bool shadowEnabled = true;
    bool blurEnabled = true;
    
    // Resizable track list and track heights
    int trackListWidth = 200;
    int trackHeight = 48;
    bool isDraggingTrackListDivider = false;
    juce::Rectangle<int> trackListDividerArea;
    
    // Track selection and mute states
    int selectedTrack = -1;
    std::vector<bool> trackMuteStates;
    int rulerHeight = 24;
    
    // Minimal scrollbars
    MinimalScrollbar horizontalScrollbar{false}; // horizontal
    MinimalScrollbar verticalScrollbar{true};    // vertical
    int scrollbarThickness = 8;
    double horizontalScrollOffset = 0.0;
    double verticalScrollOffset = 0.0;
    
    // Audio clips
    std::vector<AudioClip> audioClips;
    bool isDragOver = false;
    int pixelsPerBeat = 20; // For converting time to pixels
    
    // Playback
    double playheadPosition = 0.0; // Current playback position in beats
    double playheadVelocity = 0.0; // Velocity in beats per second for smooth movement
    juce::int64 lastFrameTime = 0; // For calculating delta time
    
    // Async file loading (prevents UI freezes)
    juce::ThreadPool loadingThreadPool{2}; // 2 background threads for loading
    juce::CriticalSection clipLoadingLock;
    std::atomic<int> pendingLoads{0};
    
    // Audio engine for mode checking
    class AudioEngine* audioEngine = nullptr;
    
    // Clip dragging and resizing
    int selectedClipIndex = -1;
    bool isDraggingClip = false;
    bool isResizingClip = false;
    bool hasStartedDragging = false; // Track if drag threshold exceeded
    enum class ResizeEdge { None, Left, Right };
    ResizeEdge resizingEdge = ResizeEdge::None;
    juce::Point<int> dragStartPos;
    double clipDragStartTime = 0.0;
    int clipDragStartTrack = 0;
    double clipOriginalDuration = 0.0;
    
    // Debug info
    juce::String debugMessage;
    int debugMessageCounter = 0;
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    juce::Rectangle<int> getMuteButtonBounds(int trackIndex);
    juce::Rectangle<int> getTrackBounds(int trackIndex);
    void drawAudioClip(juce::Graphics& g, const AudioClip& clip, bool isSelected);
    int getTrackAtPosition(int y) const;
    double getTimeAtPosition(int x) const;
    int findNextAvailableTrack(double startTime, double duration) const;
    bool isTrackOccupied(int track, double startTime, double duration) const;
    int getClipAtPosition(int x, int y) const;
    double snapToGrid(double time) const;
    ResizeEdge getResizeEdgeAtPosition(int clipIndex, int x) const;
    void deleteSelectedClip();
    void loadAudioFileAsync(const juce::File& file, double startTime);
    
    // World-to-screen coordinate transformation helpers
    int worldToScreenX(double worldX) const;
    int worldToScreenY(int worldY) const;
    double screenToWorldX(int screenX) const;
    int screenToWorldY(int screenY) const;
    
    // Rendering control
    bool renderingActive = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)

protected:
    juce::Rectangle<int> getTitleBarBounds() const override { return titleBarArea; }
};
