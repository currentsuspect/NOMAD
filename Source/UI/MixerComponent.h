#pragma once

#include <JuceHeader.h>
#include "../Audio/Mixer.h"
#include "GPUContextManager.h"
#include "DragStateManager.h"
#include "FloatingWindow.h"

/**
 * Mixer channel strip component with NOMAD styling
 */
class MixerChannelStrip : public juce::Component,
                          public juce::Timer
{
public:
    MixerChannelStrip(MixerChannel* channel, int channelIndex);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void timerCallback() override;
    
    void updateFromChannel();
    
private:
    MixerChannel* mixerChannel;
    int channelIndex;
    
    // UI state
    bool isDraggingFader = false;
    bool isDraggingPan = false;
    float faderValue = 0.8f;
    float panValue = 0.0f;
    bool isMuted = false;
    bool isSolo = false;
    
    // Metering
    float peakLevel = -60.0f;
    float rmsLevel = -60.0f;
    
    // UI areas
    juce::Rectangle<int> faderArea;
    juce::Rectangle<int> panArea;
    juce::Rectangle<int> muteButtonArea;
    juce::Rectangle<int> soloButtonArea;
    juce::Rectangle<int> meterArea;
    juce::Rectangle<int> labelArea;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelStrip)
};

/**
 * Window control button for mixer
 */
class MixerControlButton : public juce::Button
{
public:
    enum class Type { Minimize, Maximize, Close };
    
    MixerControlButton(Type t) : juce::Button(""), type(t) {}
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
private:
    Type type;
};

/**
 * Main mixer component with NOMAD styling - floating window
 */
class MixerComponent : public FloatingWindow
{
public:
    MixerComponent(Mixer& mixer);
    ~MixerComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    void refreshChannels();
    void setWorkspaceBounds(juce::Rectangle<int> bounds);
    void minimize();
    void toggleMaximize();
    
    // Rendering control for performance
    void setRenderingActive(bool shouldRender);
    bool isRenderingActive() const { return renderingActive; }
    
private:
    Mixer& mixer;
    juce::OwnedArray<MixerChannelStrip> channelStrips;
    
    int channelStripWidth = 70; // Slightly narrower like FL Studio
    int headerHeight = 32;
    
    juce::Rectangle<int> titleBarArea;
    juce::Rectangle<int> workspaceBounds;
    juce::Rectangle<int> normalBounds;
    bool isMaximized = false;
    
    // Window controls
    MixerControlButton minimizeButton{MixerControlButton::Type::Minimize};
    MixerControlButton maximizeButton{MixerControlButton::Type::Maximize};
    MixerControlButton closeButton{MixerControlButton::Type::Close};
    
    // Rendering control
    bool renderingActive = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)

protected:
    juce::Rectangle<int> getTitleBarBounds() const override { return titleBarArea; }
};
