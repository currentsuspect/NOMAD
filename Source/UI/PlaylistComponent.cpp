#include "PlaylistComponent.h"
#include "../Audio/AudioEngine.h"

PlaylistComponent::PlaylistComponent()
{
    setSize(300, 400);
    
    // Enable keyboard focus for delete key
    setWantsKeyboardFocus(true);
    
    // PERFORMANCE: Enable GPU acceleration for buttery smooth rendering
    openGLContext.attachTo(*this);
    openGLContext.setSwapInterval(1); // Enable VSync for smooth 60 FPS
    
    // Start timer for playhead animation (60 FPS)
    startTimer(16);
    
    // Setup window control buttons
    minimizeButton.onClick = [this] { minimize(); };
    addAndMakeVisible(minimizeButton);
    
    maximizeButton.onClick = [this] { toggleMaximize(); };
    addAndMakeVisible(maximizeButton);
    
    closeButton.onClick = [this] { setVisible(false); };
    addAndMakeVisible(closeButton);
    
    // Remove test button - we now use right-click context menu from file browser
    
    // Initialize track mute states (100 tracks pre-allocated, all unmuted)
    trackMuteStates.resize(100, false);
    
    // Clear debug message
    debugMessage = "";
    
    // Setup minimal scrollbars
    horizontalScrollbar.onScroll = [this](double pos) {
        horizontalScrollOffset = pos;
        repaint();
    };
    
    horizontalScrollbar.onZoom = [this](double start, double size) {
        horizontalScrollOffset = start;
        // Here you would update your zoom level based on the size
        // For now, just update the offset and repaint
        repaint();
    };
    horizontalScrollbar.setInterceptsMouseClicks(true, false);
    addAndMakeVisible(horizontalScrollbar);
    
    verticalScrollbar.onScroll = [this](double pos) {
        verticalScrollOffset = pos;
        repaint();
    };
    verticalScrollbar.setInterceptsMouseClicks(true, false);
    addAndMakeVisible(verticalScrollbar);
}

void PlaylistComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Flat dark background
    g.setColour(juce::Colour(0xff151618));
    g.fillRect(bounds);
    
    // Title bar
    auto titleBar = bounds.removeFromTop(32);
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(titleBar);
    
    // Title text
    g.setFont(juce::Font("Arial", 12.0f, juce::Font::plain));
    g.setColour(purpleGlow);
    g.drawText("Playlist", titleBar.reduced(12, 0).removeFromLeft(200), juce::Justification::centredLeft, true);
    
    // Thin separator under title bar
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, 32, (float)getWidth(), 32, 1.0f);
    
    // Ruler area
    auto rulerArea = bounds.removeFromTop(rulerHeight);
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(rulerArea.withX(trackListWidth + 4));
    
    // Draw ruler with bar numbers and beat subdivisions (4/4 time signature)
    // INFINITE GRID: Calculate only visible bars dynamically
    g.setFont(juce::Font("Arial", 9.0f, juce::Font::plain));
    int beatsPerBar = 4; // 4/4 time signature
    int beatWidth = 20; // Width of one beat
    int barWidth = beatWidth * beatsPerBar; // Width of one bar (4 beats)
    int rulerStartX = trackListWidth + 4;
    
    // Calculate which bars are visible
    int firstVisibleBar = juce::jmax(1, (int)((horizontalScrollOffset) / barWidth) + 1);
    int lastVisibleBar = (int)((horizontalScrollOffset + getWidth()) / barWidth) + 2;
    
    for (int bar = firstVisibleBar; bar <= lastVisibleBar; bar++)
    {
        int barX = rulerStartX + (bar - 1) * barWidth - (int)horizontalScrollOffset;
        
        // Draw bar number (only if it's in the visible grid area, not behind track list)
        if (barX >= rulerStartX && barX < getWidth())
        {
            g.setColour(juce::Colour(0xffaaaaaa));
            g.drawText(juce::String(bar), barX + 2, 32 + scrollbarThickness, 30, rulerHeight, juce::Justification::centredLeft, true);
        }
        
        // Draw beat subdivision lines (4 beats per bar in 4/4 time)
        for (int beat = 0; beat < beatsPerBar; beat++)
        {
            int beatX = barX + beat * beatWidth;
            if (beatX >= rulerStartX && beatX < getWidth())
            {
                g.setColour(purpleGlow.withAlpha(beat == 0 ? 0.3f : 0.15f));
                g.drawLine((float)beatX, 32 + rulerHeight - 8, (float)beatX, 32 + rulerHeight, beat == 0 ? 2.0f : 1.0f);
            }
        }
    }
    
    // Separator under ruler
    g.setColour(juce::Colour(0xff000000));
    g.drawLine((float)(trackListWidth + 4), 32 + rulerHeight, (float)getWidth(), 32 + rulerHeight, 1.0f);
    
    // Content area
    auto contentArea = bounds;
    
    // Track list panel on the left
    auto trackListArea = contentArea.removeFromLeft(trackListWidth);
    
    // Draw track list background
    g.setColour(juce::Colour(0xff0d0e0f));
    g.fillRect(trackListArea);
    
    // Draw track items - INFINITE TRACKS
    g.setFont(juce::Font("Arial", 12.0f, juce::Font::plain));
    
    int firstVisibleTrackList = juce::jmax(0, (int)(verticalScrollOffset / trackHeight));
    int lastVisibleTrackList = juce::jmin(100, (int)((verticalScrollOffset + getHeight()) / trackHeight) + 1);
    
    for (int i = firstVisibleTrackList; i <= lastVisibleTrackList; i++)
    {
        int trackY = 32 + scrollbarThickness + rulerHeight + i * trackHeight - (int)verticalScrollOffset;
        auto trackBounds = juce::Rectangle<int>(0, trackY, trackListWidth, trackHeight);
        
        // SAFETY: Check bounds before accessing
        bool isMuted = (i < trackMuteStates.size()) ? trackMuteStates[i] : false;
        bool isSelected = (i == selectedTrack);
        
        // Selection highlight
        if (isSelected)
        {
            g.setColour(purpleGlow.withAlpha(0.2f));
            g.fillRect(trackBounds);
        }
        // Alternate row colors
        else if (i % 2 == 0)
        {
            g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.3f));
            g.fillRect(trackBounds);
        }
        
        // Mute button (green circle)
        int buttonSize = 12;
        int buttonY = trackY + (trackHeight - buttonSize) / 2;
        auto muteButtonBounds = juce::Rectangle<int>(8, buttonY, buttonSize, buttonSize);
        g.setColour(isMuted ? juce::Colour(0xff444444) : juce::Colour(0xff4CAF50));
        g.fillEllipse(muteButtonBounds.toFloat());
        
        // Mute button outline
        g.setColour(isMuted ? juce::Colour(0xff666666) : juce::Colour(0xff66BB6A));
        g.drawEllipse(muteButtonBounds.toFloat(), 1.5f);
        
        // Track text (greyed out if muted)
        g.setColour(isMuted ? juce::Colour(0xff444444) : juce::Colour(0xff888888));
        auto textBounds = trackBounds.reduced(12, 0).withTrimmedLeft(24);
        g.drawText("Track " + juce::String(i + 1), textBounds, juce::Justification::centredLeft, true);
    }
    
    // Draw resizable divider between track list and pattern view
    if (!trackListDividerArea.isEmpty())
    {
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(trackListDividerArea);
        
        g.setColour(purpleGlow.withAlpha(0.3f));
        g.fillRect(trackListDividerArea.reduced(1, 0));
    }
    
    // Pattern view area (right side)
    g.setColour(juce::Colour(0xff0d0e0f));
    g.fillRect(contentArea);
    
    // Draw grid lines for pattern view (4/4 time signature)
    // INFINITE GRID: Only draw visible lines
    int patternStartY = 32 + scrollbarThickness + rulerHeight;
    
    // Vertical grid lines (bars and beats) - INFINITE
    for (int bar = firstVisibleBar; bar <= lastVisibleBar; bar++)
    {
        int barX = rulerStartX + bar * barWidth - (int)horizontalScrollOffset;
        
        if (barX >= rulerStartX && barX < getWidth())
        {
            // Draw bar line (thicker)
            g.setColour(purpleGlow.withAlpha(0.15f));
            g.drawLine((float)barX, (float)patternStartY, (float)barX, (float)getHeight(), 1.5f);
        }
        
        // Draw beat subdivision lines (4 beats per bar)
        for (int beat = 1; beat < beatsPerBar; beat++)
        {
            int beatX = barX + beat * beatWidth;
            if (beatX >= rulerStartX && beatX < getWidth())
            {
                g.setColour(purpleGlow.withAlpha(0.08f));
                g.drawLine((float)beatX, (float)patternStartY, (float)beatX, (float)getHeight(), 1.0f);
            }
        }
    }
    
    // Horizontal grid lines (tracks) - INFINITE
    int firstVisibleTrack = juce::jmax(0, (int)(verticalScrollOffset / trackHeight));
    int lastVisibleTrack = juce::jmin(100, (int)((verticalScrollOffset + getHeight()) / trackHeight) + 1);
    
    for (int i = firstVisibleTrack; i <= lastVisibleTrack; i++)
    {
        int y = patternStartY + i * trackHeight - (int)verticalScrollOffset;
        
        if (y >= patternStartY && y < getHeight())
        {
            // Dim the track lane if muted (SAFETY: bounds check)
            if (i < trackMuteStates.size() && trackMuteStates[i])
            {
                auto laneBounds = juce::Rectangle<int>(trackListWidth + 4, y, 
                                                        getWidth() - trackListWidth - 4 - scrollbarThickness, 
                                                        trackHeight);
                g.setColour(juce::Colour(0xff000000).withAlpha(0.3f));
                g.fillRect(laneBounds);
            }
        }
        
        g.setColour(purpleGlow.withAlpha(0.08f));
        g.drawLine((float)(trackListWidth + 4), (float)y, (float)getWidth(), (float)y, 1.0f);
    }
    
    // Draw vertical border line at grid start (where track list ends)
    int gridBorderX = trackListWidth + 4;
    g.setColour(juce::Colour(0xff000000).withAlpha(0.6f)); // Dark border
    g.drawLine((float)gridBorderX, (float)patternStartY, (float)gridBorderX, (float)getHeight(), 2.0f);
    
    // Optional: Add a subtle highlight on the right side of the border
    g.setColour(purpleGlow.withAlpha(0.15f));
    g.drawLine((float)(gridBorderX + 1), (float)patternStartY, (float)(gridBorderX + 1), (float)getHeight(), 1.0f);
    
    // Draw audio clips
    for (size_t i = 0; i < audioClips.size(); ++i)
    {
        drawAudioClip(g, audioClips[i], (int)i == selectedClipIndex);
    }
    
    // Draw playhead
    int playheadGridStartX = trackListWidth + 4;
    int playheadGridStartY = 32 + scrollbarThickness + rulerHeight;
    int playheadX = worldToScreenX(playheadPosition);
    if (playheadX >= playheadGridStartX && playheadX < getWidth() - scrollbarThickness)
    {
        // Draw playhead line
        g.setColour(juce::Colours::white);
        g.drawLine((float)playheadX, (float)playheadGridStartY, (float)playheadX, (float)getHeight(), 2.0f);
        
        // Draw playhead triangle at top
        juce::Path triangle;
        triangle.addTriangle((float)(playheadX - 6), (float)playheadGridStartY,
                           (float)(playheadX + 6), (float)playheadGridStartY,
                           (float)playheadX, (float)(playheadGridStartY + 10));
        g.fillPath(triangle);
    }
    
    // Draw drag-over indicator
    if (isDragOver)
    {
        g.setColour(purpleGlow.withAlpha(0.3f));
        g.fillRect(contentArea);
        g.setColour(purpleGlow);
        g.drawRect(contentArea.toFloat(), 2.0f);
        
        // Draw "DROP HERE" text
        g.setColour(purpleGlow);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText("DROP AUDIO FILE HERE", contentArea, juce::Justification::centred, true);
    }
    
    // Draw debug message overlay
    if (!debugMessage.isEmpty())
    {
        auto debugArea = getLocalBounds().removeFromTop(100).reduced(10);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRect(debugArea);
        
        g.setColour(juce::Colours::yellow);
        g.setFont(juce::Font(12.0f));
        g.drawMultiLineText(debugMessage, debugArea.getX() + 10, debugArea.getY() + 20, debugArea.getWidth() - 20);
    }
    
    // Draw loading indicator if files are being loaded asynchronously
    int pending = pendingLoads.load();
    if (pending > 0)
    {
        auto loadingArea = getLocalBounds().removeFromBottom(40).reduced(10);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(loadingArea.toFloat(), 5.0f);
        
        g.setColour(purpleGlow);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        juce::String loadingText = "Loading " + juce::String(pending) + " file" + (pending > 1 ? "s" : "") + "...";
        g.drawText(loadingText, loadingArea, juce::Justification::centred, true);
    }
}

void PlaylistComponent::resized()
{
    auto bounds = getLocalBounds();
    titleBarArea = bounds.removeFromTop(32);
    
    // Position window control buttons in title bar - extra small
    int buttonSize = 20;
    auto buttonY = (titleBarArea.getHeight() - buttonSize) / 2;
    closeButton.setBounds(titleBarArea.getRight() - buttonSize - 4, buttonY, buttonSize, buttonSize);
    maximizeButton.setBounds(titleBarArea.getRight() - (buttonSize * 2) - 6, buttonY, buttonSize, buttonSize);
    minimizeButton.setBounds(titleBarArea.getRight() - (buttonSize * 3) - 8, buttonY, buttonSize, buttonSize);
    
    // Track list divider area (4px wide, draggable)
    trackListDividerArea = juce::Rectangle<int>(trackListWidth, 32 + scrollbarThickness, 4, getHeight() - 32 - scrollbarThickness);
    
    // Position scrollbars
    // Horizontal scrollbar above ruler (after track list)
    int hScrollX = trackListWidth + 4;
    int hScrollY = 32;
    int hScrollWidth = getWidth() - hScrollX - scrollbarThickness;
    horizontalScrollbar.setBounds(hScrollX, hScrollY, hScrollWidth, scrollbarThickness);
    
    // Vertical scrollbar on right side
    int vScrollX = getWidth() - scrollbarThickness;
    int vScrollY = 32 + scrollbarThickness;
    int vScrollHeight = getHeight() - vScrollY;
    verticalScrollbar.setBounds(vScrollX, vScrollY, scrollbarThickness, vScrollHeight);
    
    // Set scrollbar ranges - INFINITE SCROLLING
    // Horizontal: Support up to 1000 bars (4000 beats)
    horizontalScrollbar.setRange(0.0, 10000.0);
    horizontalScrollbar.setViewRange(horizontalScrollOffset, 400.0);
    
    // Vertical: Support up to 100 tracks
    verticalScrollbar.setRange(0.0, 5000.0);
    verticalScrollbar.setViewRange(verticalScrollOffset, 400.0);
}

void PlaylistComponent::mouseDown(const juce::MouseEvent& event)
{
    // Check if clicking on scrollbars - let them handle it
    if (horizontalScrollbar.getBounds().contains(event.getPosition()) ||
        verticalScrollbar.getBounds().contains(event.getPosition()))
    {
        return;
    }
    
    // Check if clicking on track list divider
    if (trackListDividerArea.contains(event.getPosition()))
    {
        isDraggingTrackListDivider = true;
        return;
    }
    
    // Check if clicking on a mute button (check visible tracks only)
    int firstVisibleTrack = juce::jmax(0, (int)(verticalScrollOffset / trackHeight));
    int lastVisibleTrack = juce::jmin(99, (int)((verticalScrollOffset + getHeight()) / trackHeight) + 1);
    
    for (int i = firstVisibleTrack; i <= lastVisibleTrack; i++)
    {
        auto muteButtonBounds = getMuteButtonBounds(i);
        if (muteButtonBounds.contains(event.getPosition()))
        {
            // SAFETY: Bounds check before accessing
            if (i < trackMuteStates.size())
            {
                trackMuteStates[i] = !trackMuteStates[i];
                repaint();
                return;
            }
        }
    }
    
    // Check if clicking in the grid area (where clips are)
    int gridStartX = trackListWidth + 4;
    int gridStartY = 32 + scrollbarThickness + rulerHeight;
    int gridEndX = getWidth() - scrollbarThickness;
    int gridEndY = getHeight();
    
    if (event.getPosition().x >= gridStartX && event.getPosition().x < gridEndX &&
        event.getPosition().y >= gridStartY && event.getPosition().y < gridEndY)
    {
        // Check if clicking on a clip
        int clipIndex = getClipAtPosition(event.getPosition().x, event.getPosition().y);
        if (clipIndex >= 0)
        {
            // Right-click to delete
            if (event.mods.isRightButtonDown())
            {
                selectedClipIndex = clipIndex;
                deleteSelectedClip();
                return;
            }
            
            // Only allow resizing if this clip is already selected
            if (clipIndex == selectedClipIndex)
            {
                // Check if clicking on resize edge
                ResizeEdge edge = getResizeEdgeAtPosition(clipIndex, event.getPosition().x);
                if (edge != ResizeEdge::None)
                {
                    isResizingClip = true;
                    resizingEdge = edge;
                    dragStartPos = event.getPosition();
                    clipDragStartTime = audioClips[clipIndex].startTime;
                    clipOriginalDuration = audioClips[clipIndex].duration;
                    repaint();
                    return;
                }
            }
            
            // Select the clip and prepare for dragging
            selectedClipIndex = clipIndex;
            isDraggingClip = true;
            dragStartPos = event.getPosition();
            clipDragStartTime = audioClips[clipIndex].startTime;
            clipDragStartTrack = audioClips[clipIndex].trackIndex;
            repaint();
            return;
        }
        else
        {
            // Clicked in grid but not on a clip - deselect
            selectedClipIndex = -1;
            repaint();
            return;
        }
    }
    
    // Check if clicking on a track in the track list (for track selection)
    for (int i = 0; i < 20; i++)
    {
        auto trackBounds = getTrackBounds(i);
        if (trackBounds.contains(event.getPosition()))
        {
            selectedTrack = i;
            repaint();
            return;
        }
    }
    
    if (titleBarArea.contains(event.getPosition()))
    {
        isDragging = true;
        
        // Restore from maximized if dragging
        if (isMaximized)
        {
            isMaximized = false;
            setBounds(normalBounds);
        }
    }
}

juce::Rectangle<int> PlaylistComponent::getMuteButtonBounds(int trackIndex)
{
    int buttonSize = 12;
    int trackY = 32 + scrollbarThickness + rulerHeight + trackIndex * trackHeight - (int)verticalScrollOffset;
    int buttonY = trackY + (trackHeight - buttonSize) / 2;
    return juce::Rectangle<int>(8, buttonY, buttonSize, buttonSize);
}

juce::Rectangle<int> PlaylistComponent::getTrackBounds(int trackIndex)
{
    int trackY = 32 + scrollbarThickness + rulerHeight + trackIndex * trackHeight - (int)verticalScrollOffset;
    return juce::Rectangle<int>(0, trackY, trackListWidth, trackHeight);
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Handle clip resizing
    if (isResizingClip && selectedClipIndex >= 0 && selectedClipIndex < audioClips.size())
    {
        auto delta = event.getPosition() - dragStartPos;
        double timeDelta = delta.x / (double)pixelsPerBeat;
        
        if (resizingEdge == ResizeEdge::Left)
        {
            // Resize from left edge (change start time and duration)
            double newStartTime = clipDragStartTime + timeDelta;
            newStartTime = snapToGrid(newStartTime);
            newStartTime = juce::jmax(0.0, newStartTime);
            
            double timeDiff = newStartTime - audioClips[selectedClipIndex].startTime;
            double newDuration = audioClips[selectedClipIndex].duration - timeDiff;
            
            // Minimum duration of 0.25 beats
            if (newDuration >= 0.25)
            {
                audioClips[selectedClipIndex].startTime = newStartTime;
                audioClips[selectedClipIndex].duration = newDuration;
            }
        }
        else if (resizingEdge == ResizeEdge::Right)
        {
            // Resize from right edge (change duration only)
            double newDuration = clipOriginalDuration + timeDelta;
            newDuration = snapToGrid(newDuration);
            
            // Minimum duration of 0.25 beats
            newDuration = juce::jmax(0.25, newDuration);
            audioClips[selectedClipIndex].duration = newDuration;
        }
        
        repaint();
        return;
    }
    
    // Handle clip dragging
    if (isDraggingClip && selectedClipIndex >= 0 && selectedClipIndex < audioClips.size())
    {
        auto delta = event.getPosition() - dragStartPos;
        
        // Require minimum drag distance before actually moving (prevents accidental moves)
        if (!hasStartedDragging)
        {
            int dragThreshold = 5; // pixels
            if (std::abs(delta.x) < dragThreshold && std::abs(delta.y) < dragThreshold)
            {
                return; // Not enough movement yet
            }
            hasStartedDragging = true;
        }
        
        // Calculate new time and track
        double timeDelta = delta.x / (double)pixelsPerBeat;
        int trackDelta = delta.y / trackHeight;
        
        double newTime = clipDragStartTime + timeDelta;
        int newTrack = clipDragStartTrack + trackDelta;
        
        // Snap to grid
        newTime = snapToGrid(newTime);
        
        // Clamp to valid ranges
        newTime = juce::jmax(0.0, newTime);
        newTrack = juce::jlimit(0, 19, newTrack);
        
        // Update clip position
        audioClips[selectedClipIndex].startTime = newTime;
        audioClips[selectedClipIndex].trackIndex = newTrack;
        
        repaint();
        return;
    }
    
    // Handle track list divider dragging
    if (isDraggingTrackListDivider)
    {
        trackListWidth = juce::jlimit(100, 400, event.getPosition().x);
        resized();
        repaint();
        return;
    }
    
    if (isDragging)
    {
        // Get the mouse position relative to parent
        auto parentPos = getParentComponent()->getLocalPoint(nullptr, event.getScreenPosition());
        
        // Calculate new position (keeping the window centered under mouse)
        auto newX = parentPos.x - (getWidth() / 2);
        auto newY = parentPos.y - (titleBarArea.getHeight() / 2);
        
        // Constrain to workspace bounds
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

void PlaylistComponent::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    
    if (isResizingClip)
    {
        isResizingClip = false;
        resizingEdge = ResizeEdge::None;
        repaint();
    }
    
    if (isDraggingClip)
    {
        isDraggingClip = false;
        hasStartedDragging = false;
        repaint();
    }
    
    if (isDraggingTrackListDivider)
    {
        isDraggingTrackListDivider = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    if (isDragging)
    {
        isDragging = false;
        checkSnapToCorner();
    }
}

void PlaylistComponent::mouseMove(const juce::MouseEvent& event)
{
    // Change cursor when hovering over track list divider
    if (trackListDividerArea.contains(event.getPosition()))
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }
    
    // Check if hovering over clip resize edge
    if (selectedClipIndex >= 0 && selectedClipIndex < audioClips.size())
    {
        ResizeEdge edge = getResizeEdgeAtPosition(selectedClipIndex, event.getPosition().x);
        if (edge != ResizeEdge::None)
        {
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            return;
        }
    }
    
    if (!isDraggingTrackListDivider && !isResizingClip)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void PlaylistComponent::setWorkspaceBounds(juce::Rectangle<int> bounds)
{
    workspaceBounds = bounds;
}

void PlaylistComponent::minimize()
{
    setVisible(false);
    isMinimized = true;
}

void PlaylistComponent::toggleMaximize()
{
    if (isMaximized)
    {
        // Restore to normal size
        setBounds(normalBounds);
        isMaximized = false;
    }
    else
    {
        // Store current bounds and maximize to workspace
        normalBounds = getBounds();
        setBounds(workspaceBounds);
        isMaximized = true;
    }
}

void PlaylistComponent::checkSnapToCorner()
{
    if (workspaceBounds.isEmpty())
        return;
    
    auto pos = getBounds().getCentre();
    auto ws = workspaceBounds;
    
    // Define snap zones (50px from each corner)
    int snapZone = 50;
    
    bool nearTopLeft = pos.x < ws.getX() + snapZone && pos.y < ws.getY() + snapZone;
    bool nearTopRight = pos.x > ws.getRight() - snapZone && pos.y < ws.getY() + snapZone;
    bool nearBottomLeft = pos.x < ws.getX() + snapZone && pos.y > ws.getBottom() - snapZone;
    bool nearBottomRight = pos.x > ws.getRight() - snapZone && pos.y > ws.getBottom() - snapZone;
    
    if (nearTopLeft || nearTopRight || nearBottomLeft || nearBottomRight)
    {
        // Snap to fullscreen (workspace bounds)
        normalBounds = getBounds();
        setBounds(workspaceBounds);
        isMaximized = true;
    }
}

void PlaylistComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Check if mouse is over the ruler or grid area
    auto rulerArea = juce::Rectangle<int>(trackListWidth + 4, 32, getWidth() - trackListWidth - 4, rulerHeight);
    auto gridArea = juce::Rectangle<int>(trackListWidth + 4, 32 + scrollbarThickness + rulerHeight, 
                                         getWidth() - trackListWidth - 4 - scrollbarThickness, 
                                         getHeight() - 32 - scrollbarThickness - rulerHeight);
    
    if (rulerArea.contains(event.getPosition()))
    {
        // Horizontal scroll when over ruler
        horizontalScrollOffset -= wheel.deltaX * 50.0;
        horizontalScrollOffset -= wheel.deltaY * 50.0; // Also allow vertical wheel for horizontal scroll
        horizontalScrollOffset = juce::jlimit(0.0, 1600.0, horizontalScrollOffset);
        
        horizontalScrollbar.setViewRange(horizontalScrollOffset, 400.0);
        repaint();
    }
    else if (gridArea.contains(event.getPosition()))
    {
        // Vertical scroll when over grid
        verticalScrollOffset -= wheel.deltaY * 50.0;
        verticalScrollOffset -= wheel.deltaX * 50.0; // Also allow horizontal wheel for vertical scroll
        verticalScrollOffset = juce::jlimit(0.0, 600.0, verticalScrollOffset);
        
        verticalScrollbar.setViewRange(verticalScrollOffset, 400.0);
        repaint();
    }
}

// FileDragAndDropTarget implementation
bool PlaylistComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    debugMessage = "isInterestedInFileDrag called!\n";
    debugMessage += "Files: " + juce::String(files.size()) + "\n";
    
    // Check if any of the files are audio files
    for (const auto& file : files)
    {
        juce::File f(file);
        debugMessage += "File: " + f.getFileName() + "\n";
        
        if (f.hasFileExtension("wav") || f.hasFileExtension("mp3") || 
            f.hasFileExtension("flac") || f.hasFileExtension("ogg") || 
            f.hasFileExtension("aiff") || f.hasFileExtension("aif"))
        {
            debugMessage += "ACCEPTED!";
            repaint();
            return true;
        }
    }
    
    debugMessage += "REJECTED - not audio";
    repaint();
    return false;
}

void PlaylistComponent::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(files);
    debugMessage = "DRAG ENTERED!\n";
    debugMessage += "Position: " + juce::String(x) + ", " + juce::String(y);
    isDragOver = true;
    repaint();
}

void PlaylistComponent::fileDragExit(const juce::StringArray& files)
{
    juce::ignoreUnused(files);
    debugMessage = "DRAG EXITED";
    isDragOver = false;
    repaint();
    
    // Clear debug message after 2 seconds
    juce::Timer::callAfterDelay(2000, [this]() {
        debugMessage = "";
        repaint();
    });
}

void PlaylistComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    isDragOver = false;
    
    // Determine time position
    double time = getTimeAtPosition(x);
    
    // Load audio clips asynchronously to prevent UI freezes
    for (const auto& filePath : files)
    {
        juce::File file(filePath);
        
        if (!file.existsAsFile())
            continue;
        
        bool isAudioFile = file.hasFileExtension("wav") || file.hasFileExtension("mp3") || 
                          file.hasFileExtension("flac") || file.hasFileExtension("ogg") || 
                          file.hasFileExtension("aiff") || file.hasFileExtension("aif");
        
        if (!isAudioFile)
            continue;
        
        // Load file asynchronously on background thread
        loadAudioFileAsync(file, time);
    }
    
    repaint();
}

void PlaylistComponent::loadAudioFileAsync(const juce::File& file, double startTime)
{
    // Increment pending loads counter
    pendingLoads++;
    
    // Add loading job to thread pool
    loadingThreadPool.addJob([this, file, startTime]()
    {
        // Load audio data on background thread (doesn't block UI)
        AudioClip clip(file, 0, startTime);
        
        if (clip.loadAudioData())
        {
            // Generate waveform cache on background thread
            clip.generateWaveformCache(400, 48);
            
            // Find next available track
            int availableTrack = findNextAvailableTrack(startTime, clip.duration);
            clip.trackIndex = availableTrack;
            
            // Add clip to list on message thread (thread-safe)
            juce::MessageManager::callAsync([this, clip = std::move(clip)]() mutable
            {
                const juce::ScopedLock lock(clipLoadingLock);
                audioClips.push_back(std::move(clip));
                pendingLoads--;
                repaint();
            });
        }
        else
        {
            // Failed to load, decrement counter
            pendingLoads--;
        }
    });
}

bool PlaylistComponent::isTrackOccupied(int track, double startTime, double duration) const
{
    double endTime = startTime + duration;
    
    for (const auto& clip : audioClips)
    {
        if (clip.trackIndex == track)
        {
            double clipEnd = clip.startTime + clip.duration;
            
            // Check for overlap
            if (!(endTime <= clip.startTime || startTime >= clipEnd))
            {
                return true; // Overlap detected
            }
        }
    }
    
    return false;
}

int PlaylistComponent::findNextAvailableTrack(double startTime, double duration) const
{
    // Start from track 0 and find first available
    for (int track = 0; track < 20; ++track)
    {
        if (!isTrackOccupied(track, startTime, duration))
        {
            return track;
        }
    }
    
    // If all tracks are occupied, return last track
    return 19;
}

int PlaylistComponent::getTrackAtPosition(int y) const
{
    return juce::jlimit(0, 19, screenToWorldY(y));
}

double PlaylistComponent::getTimeAtPosition(int x) const
{
    return juce::jmax(0.0, screenToWorldX(x));
}

void PlaylistComponent::drawAudioClip(juce::Graphics& g, const AudioClip& clip, bool isSelected)
{
    int gridStartX = trackListWidth + 4;
    int gridStartY = 32 + scrollbarThickness + rulerHeight;
    
    // Transform clip from world space to screen space
    int screenX = worldToScreenX(clip.startTime);
    int screenY = worldToScreenY(clip.trackIndex);
    int screenWidth = (int)(clip.duration * pixelsPerBeat);
    int screenHeight = trackHeight;
    
    auto clipBounds = juce::Rectangle<int>(screenX, screenY, screenWidth, screenHeight);
    
    // Define playlist bounds for clipping
    auto playlistBounds = juce::Rectangle<int>(gridStartX, gridStartY, 
                                                getWidth() - gridStartX - scrollbarThickness, 
                                                getHeight() - gridStartY);
    
    // Only draw if visible (but don't modify clipBounds)
    if (!clipBounds.intersects(playlistBounds))
        return;
    
    // Save graphics state and clip drawing to playlist bounds
    juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(playlistBounds);
    
    // Check if track is muted
    bool isTrackMuted = (clip.trackIndex >= 0 && clip.trackIndex < trackMuteStates.size()) 
                        ? trackMuteStates[clip.trackIndex] : false;
    float muteAlpha = isTrackMuted ? 0.3f : 1.0f;
    
    auto innerBounds = clipBounds.reduced(2);
    
    // Purple top bar for clip name
    int headerHeight = 16;
    auto headerArea = innerBounds.removeFromTop(headerHeight);
    
    // Purple header background (brighter if selected, dimmed if muted)
    auto headerColor = isSelected ? purpleGlow.brighter(0.3f) : purpleGlow;
    g.setColour(headerColor.withAlpha(muteAlpha));
    g.fillRect(headerArea);
    
    // Clip name in white (dimmed if muted)
    g.setColour(juce::Colours::white.withAlpha(muteAlpha));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(clip.name, headerArea.reduced(4, 0), juce::Justification::centredLeft, true);
    
    // Transparent/dark background for waveform area
    g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.3f * muteAlpha));
    g.fillRect(innerBounds);
    
    // Draw waveform if there's space
    if (clipBounds.getWidth() > 40 && clip.audioData.getNumSamples() > 0)
    {
        auto waveformArea = innerBounds.reduced(2);
        
        // PERFORMANCE: Use cached waveform if available
        if (clip.hasValidWaveformCache())
        {
            // Draw cached waveform (super fast!)
            g.setOpacity(muteAlpha);
            g.drawImage(clip.getWaveformCache(), waveformArea.toFloat(),
                       juce::RectanglePlacement::stretchToFit);
        }
        else
        {
            // Generate cache on first draw (one-time cost)
            const_cast<AudioClip&>(clip).generateWaveformCache(waveformArea.getWidth(), waveformArea.getHeight());
            
            // Draw it immediately
            if (clip.hasValidWaveformCache())
            {
                g.setOpacity(muteAlpha);
                g.drawImage(clip.getWaveformCache(), waveformArea.toFloat(),
                           juce::RectanglePlacement::stretchToFit);
            }
        }
        
        // OLD CODE REMOVED - now using cache
        int numChannels = clip.audioData.getNumChannels();
        if (false && numChannels > 0)
        {
            const float* channelData = clip.audioData.getReadPointer(0);
            int numSamples = clip.audioData.getNumSamples();
            float samplesPerPixel = numSamples / waveformArea.getWidth();
            
            // OPTIMIZED: Draw simplified waveform (skip every other pixel for speed)
            juce::Path waveformTop, waveformBottom;
            float centerY = waveformArea.getCentreY();
            
            int step = juce::jmax(1, (int)(waveformArea.getWidth() / 200)); // Max 200 points
            
            for (int x = 0; x < waveformArea.getWidth(); x += step)
            {
                // Get min/max for this pixel range
                int startSample = (int)(x * samplesPerPixel);
                int endSample = (int)((x + step) * samplesPerPixel);
                endSample = juce::jmin(endSample, numSamples);
                
                // OPTIMIZED: Sample every Nth sample instead of all
                float minVal = 0.0f, maxVal = 0.0f;
                int sampleStep = juce::jmax(1, (endSample - startSample) / 10);
                for (int i = startSample; i < endSample; i += sampleStep)
                {
                    float sample = channelData[i];
                    minVal = juce::jmin(minVal, sample);
                    maxVal = juce::jmax(maxVal, sample);
                }
                
                float topY = centerY - (maxVal * waveformArea.getHeight() * 0.45f);
                float bottomY = centerY - (minVal * waveformArea.getHeight() * 0.45f);
                float xPos = waveformArea.getX() + x;
                
                if (x == 0)
                {
                    waveformTop.startNewSubPath(xPos, centerY);
                    waveformBottom.startNewSubPath(xPos, centerY);
                }
                
                waveformTop.lineTo(xPos, topY);
                waveformBottom.lineTo(xPos, bottomY);
            }
            
            // Close the path
            waveformBottom.lineTo(waveformArea.getRight(), centerY);
            waveformTop.lineTo(waveformArea.getRight(), centerY);
            
            // Draw filled waveform with purple color (dimmed if muted)
            g.setColour(purpleGlow.withAlpha(0.3f * muteAlpha));
            g.fillPath(waveformTop);
            g.fillPath(waveformBottom);
            
            // Draw outline for definition (dimmed if muted)
            g.setColour(purpleGlow.withAlpha(0.8f * muteAlpha));
            g.strokePath(waveformTop, juce::PathStrokeType(1.0f));
            g.strokePath(waveformBottom, juce::PathStrokeType(1.0f));
        }
    }
    
    // Draw selection border
    if (isSelected)
    {
        g.setColour(purpleGlow.withAlpha(muteAlpha));
        g.drawRect(clipBounds.reduced(1).toFloat(), 2.0f);
        
        // Draw resize handles on left and right edges
        int handleWidth = 6;
        auto leftHandle = clipBounds.removeFromLeft(handleWidth).reduced(0, clipBounds.getHeight() / 4);
        auto rightHandle = clipBounds.withLeft(clipBounds.getRight() - handleWidth).reduced(0, clipBounds.getHeight() / 4);
        
        g.setColour(purpleGlow.brighter(0.5f).withAlpha(muteAlpha));
        g.fillRect(leftHandle);
        g.fillRect(rightHandle);
    }
}

int PlaylistComponent::getClipAtPosition(int x, int y) const
{
    for (size_t i = 0; i < audioClips.size(); ++i)
    {
        const auto& clip = audioClips[i];
        
        // Transform clip from world space to screen space
        int screenX = worldToScreenX(clip.startTime);
        int screenY = worldToScreenY(clip.trackIndex);
        int screenWidth = (int)(clip.duration * pixelsPerBeat);
        int screenHeight = trackHeight;
        
        auto clipBounds = juce::Rectangle<int>(screenX, screenY, screenWidth, screenHeight);
        
        if (clipBounds.contains(x, y))
        {
            return (int)i;
        }
    }
    
    return -1;
}

double PlaylistComponent::snapToGrid(double time) const
{
    // Snap to nearest beat (or fraction based on grid resolution)
    double gridResolution = 0.25; // Snap to 1/4 beat (16th notes)
    return std::round(time / gridResolution) * gridResolution;
}

PlaylistComponent::ResizeEdge PlaylistComponent::getResizeEdgeAtPosition(int clipIndex, int x) const
{
    if (clipIndex < 0 || clipIndex >= audioClips.size())
        return ResizeEdge::None;
    
    const auto& clip = audioClips[clipIndex];
    
    // Transform clip from world space to screen space
    int screenX = worldToScreenX(clip.startTime);
    int screenY = worldToScreenY(clip.trackIndex);
    int screenWidth = (int)(clip.duration * pixelsPerBeat);
    int screenHeight = trackHeight;
    
    auto clipBounds = juce::Rectangle<int>(screenX, screenY, screenWidth, screenHeight);
    
    int edgeThreshold = 8; // pixels from edge to trigger resize
    
    if (x >= clipBounds.getX() && x <= clipBounds.getX() + edgeThreshold)
        return ResizeEdge::Left;
    
    if (x >= clipBounds.getRight() - edgeThreshold && x <= clipBounds.getRight())
        return ResizeEdge::Right;
    
    return ResizeEdge::None;
}

void PlaylistComponent::deleteSelectedClip()
{
    if (selectedClipIndex >= 0 && selectedClipIndex < audioClips.size())
    {
        audioClips.erase(audioClips.begin() + selectedClipIndex);
        selectedClipIndex = -1;
        repaint();
    }
}

bool PlaylistComponent::keyPressed(const juce::KeyPress& key)
{
    // Delete or Backspace key
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedClip();
        return true;
    }
    
    return false;
}

// World-to-screen coordinate transformation helpers
int PlaylistComponent::worldToScreenX(double worldX) const
{
    // World X is in beats, convert to pixels and apply viewport offset
    int gridStartX = trackListWidth + 4;
    return (int)(worldX * pixelsPerBeat) - (int)horizontalScrollOffset + gridStartX;
}

int PlaylistComponent::worldToScreenY(int worldY) const
{
    // World Y is in track indices, convert to pixels and apply viewport offset
    int gridStartY = 32 + scrollbarThickness + rulerHeight;
    return worldY * trackHeight - (int)verticalScrollOffset + gridStartY;
}

double PlaylistComponent::screenToWorldX(int screenX) const
{
    // Convert screen X back to world beats
    int gridStartX = trackListWidth + 4;
    return (screenX - gridStartX + (int)horizontalScrollOffset) / (double)pixelsPerBeat;
}

int PlaylistComponent::screenToWorldY(int screenY) const
{
    // Convert screen Y back to world track index
    int gridStartY = 32 + scrollbarThickness + rulerHeight;
    return (screenY - gridStartY + (int)verticalScrollOffset) / trackHeight;
}

void PlaylistComponent::setPlayheadPosition(double timeInBeats)
{
    // Calculate velocity based on position change
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    
    if (lastFrameTime > 0)
    {
        double deltaTime = (currentTime - lastFrameTime) / 1000.0; // Convert to seconds
        
        if (deltaTime > 0.0 && deltaTime < 1.0) // Sanity check
        {
            double positionDelta = timeInBeats - playheadPosition;
            playheadVelocity = positionDelta / deltaTime; // Beats per second
        }
    }
    
    lastFrameTime = currentTime;
    playheadPosition = timeInBeats;
}

void PlaylistComponent::setAudioEngine(AudioEngine* engine)
{
    audioEngine = engine;
}

void PlaylistComponent::timerCallback()
{
    // Only update playhead in Song mode
    if (audioEngine && audioEngine->getPlaybackMode() == AudioEngine::PlaybackMode::Pattern)
    {
        // In pattern mode, don't update playlist playhead
        return;
    }
    
    // Advance playhead based on velocity for smooth gliding
    bool needsRepaint = false;
    
    if (std::abs(playheadVelocity) > 0.001)
    {
        double deltaTime = 0.016; // Approximately 60 FPS
        
        // SMOOTH EASING: Apply cubic easing for more natural movement
        // This makes the playhead feel more "organic" like Ableton/FL Studio
        auto easeOutCubic = [](float t) -> float {
            return 1.0f - std::pow(1.0f - t, 3.0f);
        };
        
        float dampingFactor = 0.95f;
        dampingFactor = easeOutCubic(dampingFactor);
        
        playheadPosition += playheadVelocity * deltaTime;
        
        // Gradually slow down velocity with smooth easing
        playheadVelocity *= dampingFactor;
        needsRepaint = true;
    }
    
    // OPTIMIZATION: Only repaint if playhead is actually moving
    if (needsRepaint)
    {
        repaint();
    }
}
