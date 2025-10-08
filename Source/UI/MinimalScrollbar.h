#pragma once

#include <JuceHeader.h>

/**
 * Minimal flat scrollbar component matching DAW style
 */
class MinimalScrollbar : public juce::Component
{
public:
    MinimalScrollbar(bool isVertical);
    ~MinimalScrollbar() override = default;
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    
    void setRange(double newMin, double newMax);
    void setViewRange(double newStart, double newSize);
    double getScrollPosition() const { return viewStart; }
    double getViewSize() const { return viewSize; }
    
    std::function<void(double)> onScroll;
    std::function<void(double, double)> onZoom; // Called when horizontal scrollbar is resized (start, size)
    
private:
    enum class DragMode { None, Scrolling, ResizingLeft, ResizingRight };
    
    bool vertical;
    double rangeMin = 0.0;
    double rangeMax = 1000.0;
    double viewStart = 0.0;
    double viewSize = 100.0;
    DragMode dragMode = DragMode::None;
    int dragStartPos = 0;
    double dragStartValue = 0.0;
    double dragStartSize = 0.0;
    
    int resizeHandleWidth = 6; // Width of the resize handles on horizontal scrollbar
    
    juce::Rectangle<float> getThumbBounds() const;
    juce::Rectangle<float> getLeftResizeHandle() const;
    juce::Rectangle<float> getRightResizeHandle() const;
    bool isOverLeftHandle(juce::Point<int> pos) const;
    bool isOverRightHandle(juce::Point<int> pos) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MinimalScrollbar)
};
