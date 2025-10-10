#include "FloatingWindow.h"
#include "WindowManager.h"

FloatingWindow::FloatingWindow(const juce::String& windowName)
    : name(windowName)
{
    // Register for lightweight drag state updates
    DragStateManager::getInstance().addListener(this);

    // Using shared OpenGL context managed by GPUContextManager (attached in MainComponent)

    setInterceptsMouseClicks(true, true);

    // Register with WindowManager for z-order/focus/layout ops
    WindowManager::get().registerWindow(this);

    // Default bounds constraints: keep fully on-screen
    boundsConstrainer.setMinimumOnscreenAmounts(32, 32, 32, 32);
    boundsConstrainer.setMinimumWidth(200);
    boundsConstrainer.setMinimumHeight(150);
}

FloatingWindow::~FloatingWindow()
{
    // Unregister

    WindowManager::get().unregisterWindow(this);
    DragStateManager::getInstance().removeListener(this);
}

void FloatingWindow::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const bool lightweight = DragStateManager::getInstance().isLightweight();

    paintChrome(g, bounds, lightweight);

    // Content background (in case child is transparent)
    auto style = ThemeManager::get().getWindowStyle(WindowType::Generic);
    g.setColour(style.background);
    g.fillRoundedRectangle(bounds.reduced(1.0f), (float)style.borderRadius);
}

void FloatingWindow::resized()
{
    auto tb = getTitleBarBounds();
    contentBounds = getLocalBounds();
    contentBounds.removeFromTop(tb.getHeight());
    if (content != nullptr)
        content->setBounds(contentBounds);
}

void FloatingWindow::mouseDown(const juce::MouseEvent& e)
{
    if (getTitleBarBounds().contains(e.getPosition()))
    {
        isDragging = true;
        enterLightweightMode();
        // Ensure this window is on top during drag
        toFront(true);
        WindowManager::get().bringToFront(this);
    }
    setActive(true);
}

void FloatingWindow::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        // Use PlaylistComponent's proven drag logic: mouse position relative to parent
        if (auto* parent = getParentComponent())
        {
            // Get the mouse position relative to parent
            auto parentPos = parent->getLocalPoint(nullptr, e.getScreenPosition());
            
            // Calculate new position (keeping the window centered under mouse)
            auto titleBar = getTitleBarBounds();
            auto newX = parentPos.x - (getWidth() / 2);
            auto newY = parentPos.y - (titleBar.getHeight() / 2);
            
            // Constrain to workspace bounds (like PlaylistComponent)
            if (!workspaceBounds.isEmpty())
            {
                newX = juce::jlimit(workspaceBounds.getX(), 
                                   workspaceBounds.getRight() - getWidth(), 
                                   newX);
                newY = juce::jlimit(workspaceBounds.getY(), 
                                   workspaceBounds.getBottom() - getHeight(), 
                                   newY);
            }
            
            setTopLeftPosition(newX, newY);
        }
    }
}

void FloatingWindow::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (isDragging)
    {
        isDragging = false;
        exitLightweightMode();
    }
}

void FloatingWindow::setContent(std::unique_ptr<juce::Component> newContent)
{
    if (content.get() == newContent.get())
        return;

    if (content != nullptr)
        removeChildComponent(content.get());

    content = std::move(newContent);
    if (content != nullptr)
    {
        addAndMakeVisible(content.get());
        resized();
    }
}

void FloatingWindow::setActive(bool shouldBeActive)
{
    if (isActive == shouldBeActive)
        return;
    isActive = shouldBeActive;
    repaint();
}

void FloatingWindow::enterLightweightMode()
{
    DragStateManager::getInstance().enterLightweightMode();
}

void FloatingWindow::exitLightweightMode()
{
    DragStateManager::getInstance().exitLightweightMode();
}

void FloatingWindow::dragStateChanged(bool isLightweight)
{
    juce::ignoreUnused(isLightweight);
    repaint();
}

void FloatingWindow::paintChrome(juce::Graphics& g, const juce::Rectangle<float>& bounds, bool lightweight)
{
    auto style = ThemeManager::get().getWindowStyle(WindowType::Generic);

    // Shadow (skip in lightweight mode) using cache
    if (!lightweight)
    {
        auto shadowImg = effectCache.getShadow(bounds.getSmallestIntegerContainer(), 12, style.shadowOpacity);
        g.drawImageAt(shadowImg, -8, -8);
    }

    // Window body
    g.setColour(style.background);
    g.fillRoundedRectangle(bounds, (float)style.borderRadius);

    // Title bar
    auto header = juce::Rectangle<float>(bounds.getX(), bounds.getY(), bounds.getWidth(), (float)getTitleBarBounds().getHeight());
    g.setColour(juce::Colour(0xff1a1525));
    g.fillRect(header);

    // Accent border when active
    if (isActive)
    {
        g.setColour(style.border.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, (float)style.borderRadius, 2.0f);
    }
}

juce::Rectangle<int> FloatingWindow::getTitleBarBounds() const
{
    return juce::Rectangle<int>(0, 0, getWidth(), 32);
}


