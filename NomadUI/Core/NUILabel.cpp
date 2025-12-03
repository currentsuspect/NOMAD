// Â© 2025 Nomad Studios â€” All Rights Reserved. Licensed for personal & educational use only.
#include "NUILabel.h"
#include "NUITheme.h"
#include "Graphics/NUIRenderer.h"

namespace NomadUI {

NUILabel::NUILabel(const std::string& text)
    : text_(text)
{
    setSize(100, 20); // Default size
}

/**
 * @brief Renders the label into the provided renderer.
 *
 * Draws the label's optional background and border, then renders the label text
 * positioned vertically centered within the control and horizontally according
 * to the current alignment (Left, Center, Right, Justified). Text measurement
 * is cached and updated only when the text or font size changes.
 *
 * @param renderer Renderer used to draw rectangles and text.
 */
void NUILabel::onRender(NUIRenderer& renderer)
{
    auto bounds = getBounds();
    if (bounds.isEmpty()) return;

    // Draw background if visible
    if (backgroundVisible_)
    {
        renderer.fillRect(bounds, backgroundColor_);
    }

    // Draw border if visible
    if (borderVisible_ && borderWidth_ > 0.0f)
    {
        renderer.strokeRect(bounds, borderWidth_, borderColor_);
    }

    // Draw text
    if (!text_.empty())
    {
        float fontSize = fontSize_;
        
        // OPTIMIZATION: Cache text measurements - only measure when text or size changes
        if (!textSizeValid_) {
            cachedTextSize_ = renderer.measureText(text_, fontSize);
            textSizeValid_ = true;
        }
        
        // Calculate text position based on alignment
        float textX = bounds.x;
        float textY = bounds.y + (bounds.height - cachedTextSize_.height) / 2.0f;
        
        switch (alignment_)
        {
            case Alignment::Left:
                textX = bounds.x + 4.0f; // Small padding from left edge
                break;
            case Alignment::Center:
                textX = bounds.x + (bounds.width - cachedTextSize_.width) / 2.0f;
                break;
            case Alignment::Right:
                textX = bounds.x + bounds.width - cachedTextSize_.width - 4.0f; // Small padding from right edge
                break;
            case Alignment::Justified:
                textX = bounds.x + (bounds.width - cachedTextSize_.width) / 2.0f; // Center for now
                break;
        }
        
        renderer.drawText(text_, NUIPoint(textX, textY), fontSize, textColor_);
    }
}

void NUILabel::setText(const std::string& text)
{
    if (text_ != text) {
        text_ = text;
        textSizeValid_ = false; // Invalidate cache when text changes
        repaint();
    }
}

// TODO: Implement font setting when NUIFont is available
// void NUILabel::setFont(const NUIFont& font)
// {
//     font_ = font;
//     repaint();
/**
 * @brief Updates the label's text color.
 *
 * Updates the color used to draw the label's text and requests a repaint so the change becomes visible.
 *
 * @param color New text color.
 */

void NUILabel::setTextColor(const NUIColor& color)
{
    textColor_ = color;
    repaint();
}

/**
 * @brief Set the font size used to render the label's text.
 *
 * Updates the label's font size, invalidates the cached text measurement so it will be remeasured,
 * and schedules a repaint to reflect the change.
 *
 * @param size Font size to use when rendering the text.
 */
void NUILabel::setFontSize(float size)
{
    fontSize_ = size;
    textSizeValid_ = false;
    repaint();
}

/**
 * @brief Sets the horizontal text alignment for the label and schedules a repaint.
 *
 * @param alignment Desired text alignment (e.g., Left, Center, Right, Justified).
 */
void NUILabel::setAlignment(Alignment alignment)
{
    alignment_ = alignment;
    repaint();
}

void NUILabel::setMultiline(bool multiline)
{
    multiline_ = multiline;
    repaint();
}

void NUILabel::setWordWrap(bool wordWrap)
{
    wordWrap_ = wordWrap;
    repaint();
}

void NUILabel::setBackgroundColor(const NUIColor& color)
{
    backgroundColor_ = color;
    repaint();
}

void NUILabel::setBackgroundVisible(bool visible)
{
    backgroundVisible_ = visible;
    repaint();
}

void NUILabel::setBorderColor(const NUIColor& color)
{
    borderColor_ = color;
    repaint();
}

void NUILabel::setBorderWidth(float width)
{
    borderWidth_ = width;
    repaint();
}

void NUILabel::setBorderVisible(bool visible)
{
    borderVisible_ = visible;
    repaint();
}

void NUILabel::setEditable(bool editable)
{
    editable_ = editable;
    // TODO: Implement text editing functionality
}

} // namespace NomadUI