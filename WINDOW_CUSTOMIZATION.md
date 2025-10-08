# NOMAD DAW - Complete Window Customization

## Overview

NOMAD now features a fully custom, borderless window with complete control over appearance and behavior.

## Features Implemented

### 1. Borderless Window ✓
- No native Windows title bar
- No native window borders
- Fully custom UI from edge to edge

### 2. Window Dragging ✓
- Click and drag the title bar to move the window
- Draggable area: Left side of title bar (where "NOMAD" text is)
- Non-draggable areas: Buttons (Settings, Minimize, Maximize, Close)

### 3. Window Resizing ✓
- Drag from bottom-right corner to resize
- Custom resizer with subtle diagonal grip lines
- Minimum size: 800x600 pixels
- Maximum size: 3840x2160 pixels (4K)
- Maintains aspect ratio constraints

### 4. Compact Controls ✓
- Window control buttons: 32px × 32px (down from 45px)
- Settings button: 70px wide (down from 90px)
- Button text: "Settings" (down from "SETTINGS")
- Cleaner, more professional appearance

### 5. Custom Window Controls ✓
- **Minimize**: Horizontal line symbol
- **Maximize**: Square outline symbol
- **Close**: X symbol with red hover effect
- All buttons have hover states
- Graphically rendered (no Unicode issues)

## Technical Details

### Window Constraints
```cpp
Minimum: 800 × 600 pixels
Maximum: 3840 × 2160 pixels (4K)
```

### Component Sizes
```cpp
Title Bar Height: 45px
Window Control Buttons: 32px × 32px
Settings Button: 70px × 29px (with 6px padding)
Transport Bar Height: 50px
Status Bar Height: 24px
Resizer Grip: 16px × 16px
```

### Draggable Regions
- **Title Bar**: Left portion (excluding buttons)
- **Buttons**: Not draggable (clickable only)
- **Resizer**: Bottom-right corner only

## User Interaction Guide

### Moving the Window
1. Click anywhere in the title bar (left side with "NOMAD" text)
2. Drag to desired position
3. Release mouse button

### Resizing the Window
1. Move mouse to bottom-right corner
2. Cursor changes to resize cursor
3. Click and drag to resize
4. Window respects minimum/maximum size constraints

### Window Controls
- **Minimize**: Click the minimize button (—)
- **Maximize**: Click the maximize button (□)
- **Close**: Click the close button (×) - turns red on hover

### Editing Tempo
1. Click on the BPM value in the transport bar
2. Type new tempo (20-999)
3. Press Enter or click away to apply

## Files Modified/Created

### Created
- `Source/UI/WindowControlButton.h` - Custom window control buttons
- `Source/UI/CustomResizer.h` - Custom resizer with dark theme
- `WINDOW_CUSTOMIZATION.md` - This documentation

### Modified
- `Source/Main.cpp` - Borderless window setup
- `Source/MainComponent.h` - Dragging and resizing support
- `Source/MainComponent.cpp` - Mouse handling, compact buttons
- `Source/UI/TransportComponent.cpp` - Editable BPM
- `CMakeLists.txt` - Added new header files
- `FIXES.md` - Updated documentation

## Design Philosophy

The window customization follows these principles:

1. **Minimal Chrome**: Remove all unnecessary UI elements
2. **Dark Theme**: Consistent dark color scheme throughout
3. **Compact Controls**: Smaller buttons for a cleaner look
4. **Professional Feel**: Modern DAW aesthetic
5. **User Control**: Full control over window position and size
6. **Visual Feedback**: Hover states and cursor changes

## Benefits

- **Consistent Look**: No OS-specific UI elements
- **More Screen Space**: No wasted space on title bars
- **Professional Appearance**: Matches industry-standard DAWs
- **Better Branding**: Custom UI reinforces NOMAD identity
- **Cross-Platform Ready**: Same look on all operating systems
- **User-Friendly**: Intuitive dragging and resizing

## Future Enhancements

Potential additions to the window system:

- Double-click title bar to maximize/restore
- Snap to screen edges
- Remember window position/size
- Multi-monitor support enhancements
- Custom window animations
- Fullscreen mode toggle

## Conclusion

NOMAD now has a fully custom, professional window system that provides complete control over appearance and behavior while maintaining excellent usability.
