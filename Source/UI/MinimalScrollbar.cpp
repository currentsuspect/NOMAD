#include "MinimalScrollbar.h"

MinimalScrollbar::MinimalScrollbar(bool isVertical)
    : vertical(isVertical)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void MinimalScrollbar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Darker recessed track background (like a groove)
    g.setColour(juce::Colour(0xff0a0a0a)); // Very dark, almost black
    g.fillRect(bounds);
    
    // Subtle inner shadow effect for depth
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x20000000), bounds.getX(), bounds.getY(),
        juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + 2.0f,
        false));
    g.fillRect(bounds.withHeight(2.0f));
    
    // Minimal thumb - subtle and flat
    auto thumbBounds = getThumbBounds();
    
    if (isMouseOver() || dragMode != DragMode::None)
    {
        // Slightly brighter when hovered/dragging
        g.setColour(juce::Colour(0xff555555));
    }
    else
    {
        // Very subtle when idle
        g.setColour(juce::Colour(0xff3a3a3a));
    }
    
    // Flat rectangle, no rounded corners
    g.fillRect(thumbBounds);
    
    // For horizontal scrollbar, draw resize handles
    if (!vertical && (isMouseOver() || dragMode != DragMode::None))
    {
        // Draw subtle resize indicators on the edges
        g.setColour(juce::Colour(0xff666666));
        
        auto leftHandle = getLeftResizeHandle();
        auto rightHandle = getRightResizeHandle();
        
        // Draw vertical lines as resize indicators
        g.fillRect(leftHandle.withWidth(1.5f).withX(leftHandle.getX() + 2));
        g.fillRect(rightHandle.withWidth(1.5f).withX(rightHandle.getRight() - 3.5f));
    }
}

void MinimalScrollbar::mouseDown(const juce::MouseEvent& event)
{
    auto thumbBounds = getThumbBounds();
    auto pos = event.getPosition();
    
    // Check for resize handles on horizontal scrollbar
    if (!vertical)
    {
        if (isOverLeftHandle(pos))
        {
            dragMode = DragMode::ResizingLeft;
            dragStartPos = pos.x;
            dragStartValue = viewStart;
            dragStartSize = viewSize;
            return;
        }
        else if (isOverRightHandle(pos))
        {
            dragMode = DragMode::ResizingRight;
            dragStartPos = pos.x;
            dragStartValue = viewStart;
            dragStartSize = viewSize;
            return;
        }
    }
    
    if (thumbBounds.contains(pos.toFloat()))
    {
        dragMode = DragMode::Scrolling;
        dragStartPos = vertical ? pos.y : pos.x;
        dragStartValue = viewStart;
    }
    else
    {
        // Click on track - jump to position
        auto bounds = getLocalBounds().toFloat();
        double clickRatio = vertical 
            ? pos.y / bounds.getHeight()
            : pos.x / bounds.getWidth();
        
        viewStart = clickRatio * (rangeMax - rangeMin) - (viewSize / 2.0);
        viewStart = juce::jlimit(rangeMin, rangeMax - viewSize, viewStart);
        
        if (onScroll)
            onScroll(viewStart);
        
        repaint();
    }
}

void MinimalScrollbar::mouseDrag(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().toFloat();
    int currentPos = vertical ? event.getPosition().y : event.getPosition().x;
    int delta = currentPos - dragStartPos;
    double trackSize = vertical ? bounds.getHeight() : bounds.getWidth();
    
    if (dragMode == DragMode::Scrolling)
    {
        double ratio = delta / trackSize;
        double scrollDelta = ratio * (rangeMax - rangeMin);
        
        viewStart = dragStartValue + scrollDelta;
        viewStart = juce::jlimit(rangeMin, rangeMax - viewSize, viewStart);
        
        if (onScroll)
            onScroll(viewStart);
        
        repaint();
    }
    else if (dragMode == DragMode::ResizingLeft)
    {
        // Resize from left edge (zoom in/out while adjusting start position)
        double ratio = delta / trackSize;
        double sizeDelta = ratio * (rangeMax - rangeMin);
        
        double newStart = dragStartValue + sizeDelta;
        double newSize = dragStartSize - sizeDelta;
        
        // Enforce minimum size
        if (newSize < 20.0)
        {
            newSize = 20.0;
            newStart = dragStartValue + dragStartSize - 20.0;
        }
        
        // Clamp to valid range
        newStart = juce::jlimit(rangeMin, rangeMax - newSize, newStart);
        
        viewStart = newStart;
        viewSize = newSize;
        
        if (onZoom)
            onZoom(viewStart, viewSize);
        
        repaint();
    }
    else if (dragMode == DragMode::ResizingRight)
    {
        // Resize from right edge (zoom in/out)
        double ratio = delta / trackSize;
        double sizeDelta = ratio * (rangeMax - rangeMin);
        
        double newSize = dragStartSize + sizeDelta;
        
        // Enforce minimum size
        newSize = juce::jmax(20.0, newSize);
        
        // Clamp to valid range
        if (viewStart + newSize > rangeMax)
            newSize = rangeMax - viewStart;
        
        viewSize = newSize;
        
        if (onZoom)
            onZoom(viewStart, viewSize);
        
        repaint();
    }
}

void MinimalScrollbar::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    dragMode = DragMode::None;
    repaint();
}

void MinimalScrollbar::mouseEnter(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    repaint();
}

void MinimalScrollbar::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    repaint();
}

void MinimalScrollbar::mouseMove(const juce::MouseEvent& event)
{
    if (!vertical)
    {
        auto pos = event.getPosition();
        
        // Change cursor based on position
        if (isOverLeftHandle(pos) || isOverRightHandle(pos))
        {
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        }
        else if (getThumbBounds().contains(pos.toFloat()))
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
}

void MinimalScrollbar::setRange(double newMin, double newMax)
{
    rangeMin = newMin;
    rangeMax = newMax;
    repaint();
}

void MinimalScrollbar::setViewRange(double newStart, double newSize)
{
    viewStart = newStart;
    viewSize = newSize;
    repaint();
}

juce::Rectangle<float> MinimalScrollbar::getThumbBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    double totalRange = rangeMax - rangeMin;
    
    if (vertical)
    {
        double thumbHeight = (viewSize / totalRange) * bounds.getHeight();
        thumbHeight = juce::jmax(20.0, thumbHeight); // Minimum thumb size
        
        double thumbY = ((viewStart - rangeMin) / totalRange) * bounds.getHeight();
        
        return juce::Rectangle<float>(0, (float)thumbY, bounds.getWidth(), (float)thumbHeight);
    }
    else
    {
        double thumbWidth = (viewSize / totalRange) * bounds.getWidth();
        thumbWidth = juce::jmax(20.0, thumbWidth); // Minimum thumb size
        
        double thumbX = ((viewStart - rangeMin) / totalRange) * bounds.getWidth();
        
        return juce::Rectangle<float>((float)thumbX, 0, (float)thumbWidth, bounds.getHeight());
    }
}

juce::Rectangle<float> MinimalScrollbar::getLeftResizeHandle() const
{
    if (vertical)
        return {};
    
    auto thumbBounds = getThumbBounds();
    return thumbBounds.removeFromLeft((float)resizeHandleWidth);
}

juce::Rectangle<float> MinimalScrollbar::getRightResizeHandle() const
{
    if (vertical)
        return {};
    
    auto thumbBounds = getThumbBounds();
    return thumbBounds.removeFromRight((float)resizeHandleWidth);
}

bool MinimalScrollbar::isOverLeftHandle(juce::Point<int> pos) const
{
    if (vertical)
        return false;
    
    return getLeftResizeHandle().contains(pos.toFloat());
}

bool MinimalScrollbar::isOverRightHandle(juce::Point<int> pos) const
{
    if (vertical)
        return false;
    
    return getRightResizeHandle().contains(pos.toFloat());
}
