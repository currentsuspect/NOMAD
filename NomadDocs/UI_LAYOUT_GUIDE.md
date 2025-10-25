# NOMAD DAW UI Layout & Coordinate System Documentation
# For Developers: Panel Positioning and Layout Guide

## Overview
This document explains the coordinate system and panel positioning logic in NOMAD DAW. Understanding this system is crucial for adding new UI components or modifying existing panel layouts.

## Core Layout Structure

### Window Hierarchy
```
┌────────────────────────────────────────────────┐
│ Window Title Bar (handled by NUICustomWindow)  │
├────────────────────────────────────────────────┤
│ Transport Bar (60px height)                    │
├────────────────────────────────────────────────┤
│ Content Area (Window Height - Transport Height)│
│ ├─ File Browser (250px width)  ─┬─ Track Pane  │
│ │ - Starts at: contentBounds.y  │   (remaining │
│ │   + transportBarHeight        │   width)     │
│ │ - Width: 250px (configurable) │              │
│ │ - Height: Full content area   │              │
│ └───────────────────────────────┴──────────────┘
└────────────────────────────────────────────────┘
```

### Coordinate System
- **Origin (0,0)**: Top-left corner of the content area (below title bar)
- **X-axis**: Increases rightward
- **Y-axis**: Increases downward
- **Content Bounds**: The main content area starts at `contentBounds.y` which is below the title bar

## Panel Positioning

### 1. Transport Bar
```cpp
// Position: Top of content area
m_transportBar->setBounds(NUIRect(
    contentBounds.x,           // X: 0 (left edge)
    contentBounds.y,           // Y: contentBounds.y (top of content)
    width,                     // Width: Full window width
    layout.transportBarHeight  // Height: 60px (from theme)
));
```

**Theme Configuration:**
```yaml
transportBarHeight: 60.0        # Height of transport controls
transportButtonSize: 40.0       # Size of play/pause/stop buttons
transportButtonSpacing: 8.0     # Space between transport buttons
```

### 2. File Browser
```cpp
// Position: Left side, immediately below transport bar
m_fileBrowser->setBounds(NUIRect(
    contentBounds.x,                              // X: 0 (left edge)
    contentBounds.y + layout.transportBarHeight,  // Y: After transport bar
    layout.fileBrowserWidth,                      // Width: 250px (configurable)
    height - layout.transportBarHeight            // Height: Full remaining height
));
```

**Theme Configuration:**
```yaml
fileBrowserWidth: 250.0         # Left sidebar width
components:
  fileBrowser:
    itemHeight: 32.0              # Height of each file item
    headerHeight: 45.0            # Height of toolbar + path bar
    iconSize: 24.0                # Size of file icons
```

### 3. Track Manager (Panel)
```cpp
// Position: Right side, immediately below transport bar
m_trackManagerUI->setBounds(NUIRect(
    layout.fileBrowserWidth,                      // X: After file browser (250px)
    contentBounds.y + layout.transportBarHeight,  // Y: Same as file browser
    width - layout.fileBrowserWidth,               // Width: Remaining space
    height - layout.transportBarHeight            // Height: Full remaining height
));
```

**Track Manager Internal Layout:**
```
┌─────────────────────────────────────────┐
│[+] Add Track Button                     │ ← Perfectly flush left (0px margin)
├─────────────────────────────────────────┤
│ Track 1 (Orange)  [M] [S] [●]  Track 1  │ ← Compact buttons (20x15px)
│ Track 2 (Green)   [M] [S] [●]  Track 2  │ ← 2px spacing between buttons
│ Track 3 (Purple)  [M] [S] [●]  Track 3  │ ← Thin black border around each track
└─────────────────────────────────────────┘
```

### Compact Button Design
- **Size**: 20x15px (reduced from 25x20px)
- **Spacing**: 2px between buttons (reduced from 5px)
- **Position**: Start at 20px from left edge (moved closer from 100px)
- **Alignment**: Vertically centered as a tight group
- **Style**: Same as add track button - flush and compact

### Layout Configuration
```yaml
# Compact control buttons
controlButtonWidth: 20.0         # Width of M/S/R buttons
controlButtonHeight: 15.0        # Height of M/S/R buttons
controlButtonSpacing: 2.0        # Space between control buttons
controlButtonStartX: 20.0        # X position (aligned like add button)

# Component-specific settings
trackControls:
  buttonStartX: 20.0             # Matches main layout setting
  buttonSpacing: 2.0             # Tight button spacing
```

### Button Layout Calculation
```cpp
// In TrackUIComponent::onResize()
float buttonX = themeManager.getComponentDimension("trackControls", "buttonStartX"); // 20px from left
float buttonY = bounds.y + (bounds.height - buttonGroupHeight) / 2.0f; // Center vertically

// Buttons positioned in compact vertical group
m_muteButton->setBounds(NUIAbsolute(bounds, buttonX, buttonY, 20, 15));
m_soloButton->setBounds(NUIAbsolute(bounds, buttonX, buttonY + 17, 20, 15)); // 15px + 2px spacing
m_recordButton->setBounds(NUIAbsolute(bounds, buttonX, buttonY + 34, 20, 15)); // 2 * (15px + 2px)
```

**Theme Configuration:**
```yaml
trackControlsWidth: 150.0       # Track controls panel width
trackHeight: 80.0               # Height of each track row
trackSpacing: 5.0               # Space between tracks (currently 0 for touching)
```

## Track Layout Details

### Compact Layout (Current)
- **No top margin**: Tracks start immediately at the top of the track manager area
- **Add track button**: Positioned at very top with 10px left margin
- **Track spacing**: 0px (tracks touch each other)
- **Total height per track**: 80px (just the track height)

### Layout Calculation
```cpp
// In TrackManagerUI::layoutTracks()
float currentY = 0.0f;                    // Start at very top
// Add track button at y=0
currentY = 30.0f;                         // After 30px button
// Track 1 at y=30, Track 2 at y=110, Track 3 at y=190, etc.
```

### Adding Spacing Between Tracks
To add space between tracks, modify the layout calculation:
```cpp
// Change this line in TrackManagerUI::layoutTracks()
currentY += layout.trackHeight; // Currently no spacing

// To this for 5px spacing:
currentY += layout.trackHeight + layout.trackSpacing;
```

### Adding Top Panel Space
To add a panel above the tracks (like a toolbar), modify:
```cpp
float currentY = 50.0f; // Start 50px from top for panel space
// Add track button at y=50
currentY = 80.0f; // After 30px button (total 80px from top)
// Tracks start at y=80
```

## Layout Dimensions System

### Core Dimensions (from `nomad_ui_config.yaml`)
```yaml
layout:
  # Panel widths
  fileBrowserWidth: 250.0          # Left sidebar width
  trackControlsWidth: 150.0        # Track controls panel width

  # Transport bar dimensions
  transportBarHeight: 60.0         # Height of transport controls

  # Track dimensions
  trackHeight: 80.0                # Height of each track row
  trackSpacing: 5.0                # Space between tracks

  # Margins and padding
  panelMargin: 10.0                # Margin around panels
  componentPadding: 8.0            # Padding inside components
```

## Adding New Panels

### Best Practices

1. **Use Content Bounds**: Always reference `contentBounds` for positioning
   ```cpp
   NomadUI::NUIRect contentBounds = getBounds();
   ```

2. **Consistent Y Positioning**: All panels below transport bar should use:
   ```cpp
   contentBounds.y + layout.transportBarHeight
   ```

3. **Theme-Driven Sizing**: Use theme dimensions for consistent sizing:
   ```cpp
   auto& themeManager = NUIThemeManager::getInstance();
   const auto& layout = themeManager.getLayoutDimensions();
   ```

4. **Responsive Width**: Calculate width based on remaining space:
   ```cpp
   float remainingWidth = width - existingPanelsWidth;
   ```

### Example: Adding a New Panel

```cpp
void NomadContent::onResize(int width, int height) {
    auto& themeManager = NUIThemeManager::getInstance();
    const auto& layout = themeManager.getLayoutDimensions();
    NomadUI::NUIRect contentBounds = getBounds();

    // Existing panels...
    // File browser: contentBounds.x to layout.fileBrowserWidth
    // Track panel: layout.fileBrowserWidth to width

    // New panel positioning
    float newPanelX = layout.fileBrowserWidth + layout.trackControlsWidth;
    float newPanelWidth = width - newPanelX;

    if (m_newPanel) {
        m_newPanel->setBounds(NUIRect(
            newPanelX,                                    // X: After track controls
            contentBounds.y + layout.transportBarHeight,  // Y: After transport bar
            newPanelWidth,                                // Width: Remaining space
            height - layout.transportBarHeight           // Height: Full remaining
        ));
    }
}
```

## Common Pitfalls to Avoid

### ❌ **Wrong: Using absolute coordinates**
```cpp
// DON'T: This ignores content bounds
m_panel->setBounds(NUIRect(0, 60, 200, 400));
```

### ✅ **Correct: Using relative coordinates**
```cpp
// DO: Use content bounds + theme dimensions
NomadUI::NUIRect contentBounds = getBounds();
m_panel->setBounds(NUIRect(
    contentBounds.x,
    contentBounds.y + layout.transportBarHeight,
    layout.someWidth,
    height - layout.transportBarHeight
));
```

### ❌ **Wrong: Inconsistent coordinate systems**
```cpp
// DON'T: Mix different reference points
m_panel1->setBounds(NUIRect(0, layout.transportBarHeight, w, h));      // Wrong
m_panel2->setBounds(NUIRect(0, contentBounds.y + layout.transportBarHeight, w, h)); // Correct
```

### ✅ **Correct: Consistent coordinate systems**
```cpp
// DO: Use same reference point for all panels
NomadUI::NUIRect contentBounds = getBounds();
m_panel1->setBounds(NUIRect(contentBounds.x, contentBounds.y + layout.transportBarHeight, w1, h));
m_panel2->setBounds(NUIRect(contentBounds.x + w1, contentBounds.y + layout.transportBarHeight, w2, h));
```

## Theme Modification

### Adding New Layout Dimensions

1. **Update `nomad_ui_config.yaml`**:
```yaml
layout:
  newPanelWidth: 300.0
  newPanelHeight: 200.0
```

2. **Use in code**:
```cpp
float newPanelWidth = layout.newPanelWidth;
float newPanelHeight = layout.newPanelHeight;
```

### Panel-Specific Settings

```yaml
components:
  fileBrowser:
    itemHeight: 32.0
    headerHeight: 45.0
  trackControls:
    buttonSpacing: 5.0
  newPanel:
    customSetting: 100.0
```

Access in code:
```cpp
float customSetting = themeManager.getComponentDimension("newPanel", "customSetting");
```

## Debugging Layout Issues

### Enable Debug Logging
```cpp
// In Main.cpp, enable debug output
Log::info("Panel positioning:");
Log::info("  Content bounds: " + std::to_string(contentBounds.x) + "," + std::to_string(contentBounds.y));
Log::info("  Panel bounds: " + std::to_string(panelBounds.x) + "," + std::to_string(panelBounds.y));
```

### Common Issues and Solutions

1. **Panel overlapping transport bar**:
   - **Cause**: Using `layout.transportBarHeight` instead of `contentBounds.y + layout.transportBarHeight`
   - **Solution**: Always use content bounds as reference

2. **Panels not resizing properly**:
   - **Cause**: Not calling `onResize()` in parent `onResize()` method
   - **Solution**: Ensure `onResize()` calls `setBounds()` with new dimensions

3. **Inconsistent spacing**:
   - **Cause**: Hard-coded values instead of theme dimensions
   - **Solution**: Use `themeManager.getLayoutDimensions()`

## File Structure for UI Components

```
Source/
├── Main.cpp                    # Main layout coordination
├── TransportBar.cpp/h          # Transport controls
├── FileBrowser.cpp/h           # File browser panel
├── TrackManagerUI.cpp/h        # Track panel management
└── TrackUIComponent.cpp/h      # Individual track UI
```

## Window Management & Minimize/Restore Fix

### Problem Fixed
- **Issue**: Borderless windows need proper taskbar integration for minimize/restore functionality
- **Solution**: Use `WS_POPUP` with `WS_EX_APPWINDOW` and custom message handling
- **Result**: Truly borderless window with full Windows integration

### Technical Implementation

#### Window Creation (PlatformWindowWin32.cpp)
```cpp
// CORRECT: Proper borderless window with taskbar support
if (!desc.decorated) {
    style = WS_POPUP;  // ✅ Truly borderless, works with minimize/restore
    exStyle = WS_EX_APPWINDOW;  // ✅ Appears in taskbar and Alt+Tab
}

// WRONG: Shows system title bar
if (!desc.decorated) {
    style = WS_OVERLAPPEDWINDOW;  // ❌ Shows system title bar
}
```

#### Message Handling
```cpp
// Handle minimize/restore from taskbar (WS_POPUP windows)
case WM_SYSCOMMAND: {
    if (!(style & WS_CAPTION)) {  // Borderless window
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(m_hwnd, SW_MINIMIZE);  // ✅ Proper minimize
            return 0;
        }
        else if ((wParam & 0xFFF0) == SC_RESTORE) {
            ShowWindow(m_hwnd, SW_RESTORE);   // ✅ Proper restore
            return 0;
        }
    }
    break;
}

// Prevent system title bar drawing
case WM_NCPAINT: {
    if (!(style & WS_CAPTION)) {
        return 0;  // Don't paint non-client area (no system title bar)
    }
    break;
}
```

### Why This Works
1. **Taskbar Integration**: `WS_EX_APPWINDOW` makes the window appear in taskbar and Alt+Tab
2. **Borderless Appearance**: `WS_POPUP` style creates truly borderless window
3. **Custom Message Handling**: `WM_SYSCOMMAND` intercepts minimize/restore commands
4. **Hidden Title Bar**: `WM_NCPAINT` prevents system title bar from being drawn
5. **Custom Hit Testing**: `WM_NCHITTEST` provides dragging and resizing without visible chrome

### Benefits
- ✅ **Minimize from taskbar works**
- ✅ **Restore from taskbar works**
- ✅ **Window appears in Alt+Tab**
- ✅ **Maintains borderless custom UI**
- ✅ **Preserves all custom dragging and resizing**

This fix ensures NOMAD DAW behaves like a proper Windows application while maintaining its custom, professional interface design!

## Sound Preview System

### Overview
NOMAD DAW now includes a sophisticated sound preview system that allows users to audition audio files directly from the file browser.

### Features
- **5-Second Previews**: Audio files play for exactly 5 seconds when clicked
- **Unique Tones**: Each file generates a unique preview tone based on filename hash
- **Automatic Management**: Previews stop when selecting different files or after 5 seconds
- **Lower Volume**: Preview plays at 30% volume to avoid startling users
- **Hidden Preview Track**: Dedicated preview track doesn't appear in main UI

### Implementation Details
```cpp
// FileBrowser callback setup
m_fileBrowser->setOnSoundPreview([this](const FileItem& file) {
    playSoundPreview(file);
});

// Preview system
void playSoundPreview(const FileItem& file) {
    // Stop current preview
    stopSoundPreview();

    // Load file into preview track
    m_previewTrack->loadAudioFile(file.path);
    m_previewTrack->play();
    m_previewStartTime = std::chrono::steady_clock::now();
}

// Timer-based preview management
void updateSoundPreview() {
    if (m_previewIsPlaying) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - m_previewStartTime);
        if (elapsed.count() >= 5.0) {
            stopSoundPreview();
        }
    }
}
```

## Final Summary

This comprehensive guide covers the complete NOMAD DAW UI system:

### ✅ **Layout & Positioning**
- **Consistent coordinate system** using `contentBounds.y + layout.transportBarHeight`
- **Responsive design** with theme-driven dimensions
- **Compact track layout** with no gaps and professional alignment

### ✅ **Visual Design**
- **Thin black borders** around tracks for clear differentiation
- **Numbered track names** ("Track 1", "Track 2", etc.)
- **Compact control buttons** (20x15px) with tight 2px spacing
- **Professional spacing** and alignment throughout

### ✅ **Window Management**
- **Proper minimize/restore** from taskbar (fixed from broken `WS_POPUP`)
- **Borderless appearance** with full Windows integration
- **Taskbar compatibility** and Alt+Tab support

### ✅ **Developer Experience**
- **Clear documentation** for adding new panels
- **Theme configuration** system for easy customization
- **Debugging guides** for layout troubleshooting

The NOMAD DAW now has a **professional, responsive, and fully functional UI** that matches modern DAW standards! 🎯
