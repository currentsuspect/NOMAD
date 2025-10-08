# Bug Fixes

## Window Control Buttons Rendering Issue

**Problem**: The minimize, maximize, and close buttons were using Unicode characters (−, □, ×) that weren't rendering properly on Windows.

**Solution**: Created a custom `WindowControlButton` class that draws the symbols graphically using JUCE's Graphics API instead of relying on Unicode characters.

### Changes Made:

1. **Created `Source/UI/WindowControlButton.h`**
   - Custom button class that draws minimize (horizontal line), maximize (square outline), and close (X) symbols
   - Proper hover effects with background color changes
   - Close button shows red background on hover

2. **Updated `Source/MainComponent.h`**
   - Replaced `juce::TextButton` with `WindowControlButton` for window controls
   - Added include for `WindowControlButton.h`

3. **Updated `Source/MainComponent.cpp`**
   - Simplified button setup (no need to set text or colors manually)
   - Buttons now render consistently across all platforms

4. **Updated `CMakeLists.txt`**
   - Added `Source/UI/WindowControlButton.h` to the source list

## BPM Not Working

**Problem**: The BPM display was read-only and users couldn't change the tempo.

**Solution**: Made the tempo label editable so users can click and type a new BPM value.

### Changes Made:

1. **Updated `Source/UI/TransportComponent.cpp`**
   - Made `tempoValueLabel` editable by calling `setEditable(true, true, false)`
   - Added `onTextChange` callback to parse user input and update the transport controller
   - Added validation to ensure BPM stays within valid range (20-999)
   - Added visual feedback with different colors when editing (green accent)
   - Changed mouse cursor to I-beam when hovering over the BPM display

### How to Use:

- Click on the BPM value in the transport bar
- Type a new tempo value (20-999 BPM)
- Press Enter or click away to apply the change
- Invalid values will revert to the current tempo

## Custom Borderless Window with Resizing

**Problem**: The Windows native title bar and status bar were taking up space and didn't match the custom UI design. Window was not resizable.

**Solution**: Removed the native title bar and created a fully custom window with our own title bar, window controls, and resize functionality.

### Changes Made:

1. **Updated `Source/Main.cpp`**
   - Disabled native title bar with `setUsingNativeTitleBar(false)`
   - Set title bar height to 0 with `setTitleBarHeight(0)`
   - Removed Windows-specific dark mode title bar code (no longer needed)

2. **Created `Source/UI/CustomResizer.h`**
   - Custom resizer component with dark theme styling
   - Subtle diagonal grip lines in the corner
   - Matches the overall dark aesthetic

3. **Updated `Source/MainComponent.h`**
   - Added `mouseDown()` and `mouseDrag()` methods for window dragging
   - Added `CustomResizer` for window resizing
   - Added `ComponentBoundsConstrainer` to set min/max window sizes
   - Added `draggableArea` rectangle to define the draggable region

4. **Updated `Source/MainComponent.cpp`**
   - Implemented `mouseDown()` to start dragging when clicking in the title bar
   - Implemented `mouseDrag()` to move the window
   - Set `draggableArea` to the title bar region (excluding buttons)
   - Added resize constraints (min: 800x600, max: 3840x2160)
   - Made window control buttons more compact (32px instead of 45px)
   - Made settings button more compact (70px instead of 90px, "Settings" instead of "SETTINGS")
   - Positioned resizer in bottom-right corner

5. **Updated `CMakeLists.txt`**
   - Added `Source/UI/CustomResizer.h` to the source list

### Features:

- **Fully Custom UI**: No native Windows elements visible
- **Draggable Title Bar**: Click and drag the title bar area to move the window
- **Resizable Window**: Drag from the bottom-right corner to resize (minimum 800x600, maximum 4K)
- **Custom Window Controls**: Compact minimize, maximize, and close buttons (32px each)
- **Compact Action Buttons**: Smaller "Settings" button for a cleaner look
- **Clean Design**: Seamless, professional appearance matching modern DAW aesthetics

## Testing

All fixes have been tested and verified:
- Window control buttons now render correctly with clear, crisp symbols (compact 32px size)
- BPM can be edited by clicking and typing a new value
- Window can be moved by dragging the title bar
- Window can be resized by dragging from the bottom-right corner
- Window respects minimum size (800x600) and maximum size (4K)
- Window controls (minimize, maximize, close) work correctly
- Settings button is more compact and cleaner
- No native Windows title bar or status bar visible
- Custom resizer grip is visible and styled to match the dark theme
- All changes compile without errors or warnings (except for unused parameter warnings which are harmless)
