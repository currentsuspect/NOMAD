#pragma once

#include "NUIComponent.h"
#include "NUITypes.h"
#include <functional>

namespace NomadUI {

/**
 * NUIScrollbar - A scrollbar component for scrollable content
 * Supports both horizontal and vertical scrolling with customizable appearance
 * Replaces juce::ScrollBar with NomadUI styling and theming
 */
class NUIScrollbar : public NUIComponent
{
public:
    // Scrollbar orientations
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    // Scrollbar parts
    enum class Part
    {
        None,
        Track,
        Thumb,
        LeftArrow,   // or UpArrow for vertical
        RightArrow,  // or DownArrow for vertical
        LeftTrack,   // Track before thumb
        RightTrack   // Track after thumb
    };

    NUIScrollbar(Orientation orientation = Orientation::Vertical);
    ~NUIScrollbar() override = default;

    // Component interface
    void onRender(NUIRenderer& renderer) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;

    // Scroll properties
    void setCurrentRange(double start, double size);
    double getCurrentRangeStart() const { return currentRangeStart_; }
    double getCurrentRangeSize() const { return currentRangeSize_; }

    void setRangeLimit(double start, double size);
    double getRangeLimitStart() const { return rangeLimitStart_; }
    double getRangeLimitSize() const { return rangeLimitSize_; }

    void setSingleStepSize(double step);
    double getSingleStepSize() const { return singleStepSize_; }

    void setPageStepSize(double step);
    double getPageStepSize() const { return pageStepSize_; }

    void setAutoHide(bool autoHide);
    bool isAutoHide() const { return autoHide_; }

    void setAutoHideDelay(double delay);
    double getAutoHideDelay() const { return autoHideDelay_; }

    // Visual properties
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return orientation_; }

    void setThumbSize(double size);
    double getThumbSize() const { return thumbSize_; }

    void setMinimumThumbSize(double size);
    double getMinimumThumbSize() const { return minimumThumbSize_; }

    void setTrackColor(const NUIColor& color);
    NUIColor getTrackColor() const { return trackColor_; }

    void setThumbColor(const NUIColor& color);
    NUIColor getThumbColor() const { return thumbColor_; }

    void setThumbHoverColor(const NUIColor& color);
    NUIColor getThumbHoverColor() const { return thumbHoverColor_; }

    void setThumbPressedColor(const NUIColor& color);
    NUIColor getThumbPressedColor() const { return thumbPressedColor_; }

    void setArrowColor(const NUIColor& color);
    NUIColor getArrowColor() const { return arrowColor_; }

    void setArrowHoverColor(const NUIColor& color);
    NUIColor getArrowHoverColor() const { return arrowHoverColor_; }

    void setArrowPressedColor(const NUIColor& color);
    NUIColor getArrowPressedColor() const { return arrowPressedColor_; }

    void setBorderColor(const NUIColor& color);
    NUIColor getBorderColor() const { return borderColor_; }

    void setBorderWidth(float width);
    float getBorderWidth() const { return borderWidth_; }

    void setBorderRadius(float radius);
    float getBorderRadius() const { return borderRadius_; }

    void setArrowSize(float size);
    float getArrowSize() const { return arrowSize_; }

    // Scrolling methods
    void scrollBy(double delta);
    void scrollTo(double position);
    void scrollToStart();
    void scrollToEnd();
    void scrollByPage(double direction);
    void scrollByLine(double direction);

    // Utility
    double getCurrentPosition() const;
    double getMaximumPosition() const;
    double getThumbPosition() const;
    double getThumbLength() const;
    bool isAtStart() const;
    bool isAtEnd() const;

    // Event callbacks
    void setOnScroll(std::function<void(double)> callback);
    void setOnScrollStart(std::function<void()> callback);
    void setOnScrollEnd(std::function<void()> callback);

protected:
    // Override these for custom scrollbar appearance
    virtual void drawTrack(NUIRenderer& renderer);
    virtual void drawThumb(NUIRenderer& renderer);
    virtual void drawArrows(NUIRenderer& renderer);
    virtual void drawLeftArrow(NUIRenderer& renderer);
    virtual void drawRightArrow(NUIRenderer& renderer);

    // Hit testing
    virtual Part getPartAtPosition(const NUIPoint& position) const;
    virtual NUIRect getThumbRect() const;
    virtual NUIRect getLeftArrowRect() const;
    virtual NUIRect getRightArrowRect() const;
    virtual NUIRect getTrackRect() const;

    // Scrolling calculations
    virtual double positionToValue(const NUIPoint& position) const;
    virtual NUIPoint valueToPosition(double value) const;

    // Helper drawing methods
    virtual void drawArrowIcon(NUIRenderer& renderer, const NUIRect& rect, float rotation, const NUIColor& color);

private:
    void updateThumbSize();
    void updateThumbPosition();
    void startAutoHideTimer();
    void stopAutoHideTimer();
    void triggerScroll();
    void triggerScrollStart();
    void triggerScrollEnd();
    
    // Enhanced drawing methods
    void drawEnhancedTrack(NUIRenderer& renderer, const NUIRect& trackRect);
    void drawEnhancedThumb(NUIRenderer& renderer, const NUIRect& thumbRect);

    // Scroll state
    double currentRangeStart_ = 0.0;
    double currentRangeSize_ = 0.0;
    double rangeLimitStart_ = 0.0;
    double rangeLimitSize_ = 1.0;
    double singleStepSize_ = 0.1;
    double pageStepSize_ = 0.5;
    double thumbSize_ = 0.0;
    double minimumThumbSize_ = 0.1;

    // Visual properties
    Orientation orientation_ = Orientation::Vertical;
    NUIColor trackColor_ = NUIColor(0.15f, 0.15f, 0.18f, 1.0f); // Dark charcoal track
    NUIColor thumbColor_ = NUIColor(0.8f, 0.8f, 0.8f, 0.8f); // Faded white thumb base
    NUIColor thumbHoverColor_ = NUIColor(0.9f, 0.9f, 0.9f, 0.9f); // Lighter white on hover
    NUIColor thumbPressedColor_ = NUIColor(0.7f, 0.7f, 0.7f, 0.9f); // Darker white when pressed
    NUIColor arrowColor_ = NUIColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray arrows
    NUIColor arrowHoverColor_ = NUIColor(1.0f, 1.0f, 1.0f, 1.0f); // White on hover
    NUIColor arrowPressedColor_ = NUIColor(0.6f, 0.6f, 0.6f, 1.0f); // Darker grey when pressed
    NUIColor borderColor_ = NUIColor(0.3f, 0.3f, 0.35f, 1.0f); // Subtle border
    float borderWidth_ = 1.0f;
    float borderRadius_ = 4.0f;
    float arrowSize_ = 12.0f;

    // Auto-hide behavior
    bool autoHide_ = false;
    double autoHideDelay_ = 1.0;
    double autoHideTimer_ = 0.0;
    bool isAutoHidden_ = false;

    // Interaction state
    bool isHovered_ = false;
    bool isPressed_ = false;
    Part pressedPart_ = Part::None;
    NUIPoint dragStartPosition_;
    double dragStartValue_ = 0.0;
    bool isDragging_ = false;

    // Animation state
    bool isAnimating_ = false;
    double animationStartValue_ = 0.0;
    double animationTargetValue_ = 0.0;
    double animationTime_ = 0.0;
    double animationDuration_ = 0.2;

    // Callbacks
    std::function<void(double)> onScrollCallback_;
    std::function<void()> onScrollStartCallback_;
    std::function<void()> onScrollEndCallback_;
};

} // namespace NomadUI
