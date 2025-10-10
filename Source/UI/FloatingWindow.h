#pragma once

#include <JuceHeader.h>
#include "GPUContextManager.h"
#include "DragStateManager.h"
#include "ThemeManager.h"
#include "EffectCache.h"

/**
 * FloatingWindow: unified base class for all Nomad windows (Playlist, Mixer, Sequencer).
 * Provides consistent chrome, drag/resize hooks, and GPU setup points.
 */
class FloatingWindow : public juce::Component,
                       public DragStateManager::Listener
{
public:
    explicit FloatingWindow(const juce::String& windowName);
    ~FloatingWindow() override;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Content management
    void setContent(std::unique_ptr<juce::Component> newContent);
    juce::Component* getContent() const { return content.get(); }

    // State
    void setActive(bool shouldBeActive);
    bool isActiveWindow() const { return isActive; }

    // Lightweight drag mode (proxied to DragStateManager)
    void enterLightweightMode();
    void exitLightweightMode();

    // DragStateManager::Listener
    void dragStateChanged(bool isLightweight) override;
    
    // Public accessor for bounds constrainer (needed by MainComponent::resized)
    juce::ComponentBoundsConstrainer& getBoundsConstrainer() { return boundsConstrainer; }
    
    // Workspace bounds (for constraining dragging like PlaylistComponent)
    void setWorkspaceBounds(juce::Rectangle<int> bounds) { workspaceBounds = bounds; }
    juce::Rectangle<int> getWorkspaceBounds() const { return workspaceBounds; }

protected:
    // Subclasses can override to customize chrome painting
    virtual void paintChrome(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool lightweight);

    // Subclasses can override to declare title bar area used for dragging
    virtual juce::Rectangle<int> getTitleBarBounds() const;

    // Cached rectangles
    juce::Rectangle<int> contentBounds;
    
    // Protected drag state so subclasses can check it
    bool isDragging = false;

private:
    juce::String name;
    std::unique_ptr<juce::Component> content;

    // Dragging
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer boundsConstrainer;
    juce::Rectangle<int> workspaceBounds; // Bounds for constraining drag (like PlaylistComponent)
    bool isActive = false;

    // No per-window OpenGL context; using GPUContextManager shared context

    // Visual caches
    EffectCache effectCache;
};


