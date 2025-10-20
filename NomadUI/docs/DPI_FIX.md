# 🔧 Custom Title Bar DPI/Resolution Fix

## Problem

Custom title bar windows in NomadUI are appearing with incorrect resolution/scaling on Windows because DPI awareness isn't being set.

## Root Cause

Windows scales applications that don't declare DPI awareness, causing:
- Blurry rendering
- Incorrect window sizes
- Mouse coordinates misalignment
- Title bar sizing issues

## Solution

### 1. Created DPI Helper

**Files Created:**
- `NomadUI/Platform/Windows/NUIDPIHelper.h`
- `NomadUI/Platform/Windows/NUIDPIHelper.cpp`

**Features:**
- Sets per-monitor DPI awareness (Windows 10+)
- Falls back to system DPI awareness (Windows 8.1+)
- Provides DPI scale factor queries
- Helper functions for DPI scaling

### 2. How to Use

#### In Your Application Startup:

```cpp
#include "NomadUI/Platform/Windows/NUIDPIHelper.h"

int main() {
    // MUST be called before creating any windows!
    NomadUI::NUIDPIHelper::initializeDPI();
    
    // Now create your windows
    NUIWindowWin32 window;
    window.create("My App", 800, 600);
    
    // ...
}
```

#### In Window Creation:

```cpp
// Get DPI scale for the window
float dpiScale = NUIDPIHelper::getDPIScale(hwnd);

// Scale sizes appropriately
int scaledWidth = NUIDPIHelper::scaleToDPI(800, dpiScale);
int scaledHeight = NUIDPIHelper::scaleToDPI(600, dpiScale);
```

### 3. Update CMakeLists.txt

Add the new files to the build:

```cmake
# In NomadUI/CMakeLists.txt, add to Platform sources:
set(PLATFORM_SOURCES
    Platform/Windows/NUIWindowWin32.cpp
    Platform/Windows/NUIDPIHelper.cpp  # ADD THIS
)

set(PLATFORM_HEADERS
    Platform/Windows/NUIWindowWin32.h
    Platform/Windows/NUIDPIHelper.h    # ADD THIS
)
```

### 4. Update Examples

Each demo should call `initializeDPI()` at startup:

```cpp
// In SliderTextDemo.cpp, CustomWindowDemo.cpp, etc.
#include "../Platform/Windows/NUIDPIHelper.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // Initialize DPI awareness FIRST
    NomadUI::NUIDPIHelper::initializeDPI();
    
    // Then create window
    // ...
}
```

## Testing

### Before Fix:
- Window appears blurry on high-DPI displays
- Title bar buttons misaligned
- Mouse clicks don't match visual positions
- Text appears fuzzy

### After Fix:
- Crisp rendering at any DPI
- Correct window sizing
- Accurate mouse coordinates
- Sharp text rendering

## DPI Scaling Examples

| Display Setting | DPI | Scale Factor |
|----------------|-----|--------------|
| 100% (default) | 96  | 1.0 |
| 125%           | 120 | 1.25 |
| 150%           | 144 | 1.5 |
| 175%           | 168 | 1.75 |
| 200%           | 192 | 2.0 |

## Quick Fix for Existing Code

If you're seeing resolution issues right now:

1. Add `#include "NomadUI/Platform/Windows/NUIDPIHelper.h"` to your main file
2. Call `NUIDPIHelper::initializeDPI()` before creating any windows
3. Rebuild

That's it! The DPI helper will automatically detect and handle scaling.

## Advanced Usage

### Get Current DPI:

```cpp
HWND hwnd = /* your window handle */;
int dpi = NUIDPIHelper::getDPI(hwnd);
float scale = NUIDPIHelper::getDPIScale(hwnd);

std::cout << "DPI: " << dpi << " (scale: " << scale << "x)" << std::endl;
```

### Scale UI Elements:

```cpp
// Scale a button size
int buttonWidth = 100;
int buttonHeight = 30;

float dpiScale = NUIDPIHelper::getDPIScale(hwnd);
int scaledWidth = NUIDPIHelper::scaleToDPI(buttonWidth, dpiScale);
int scaledHeight = NUIDPIHelper::scaleToDPI(buttonHeight, dpiScale);
```

### Handle DPI Changes:

```cpp
// In your window procedure, handle WM_DPICHANGED:
case WM_DPICHANGED: {
    float newScale = NUIDPIHelper::getDPIScale(hwnd);
    // Update your UI layout with new scale
    updateLayout(newScale);
    break;
}
```

## Notes

- **Call `initializeDPI()` ONCE at startup, before any windows**
- DPI awareness is process-wide, not per-window
- Per-monitor DPI awareness requires Windows 8.1+
- The helper automatically falls back to older APIs on older Windows versions

## Troubleshooting

### Still Blurry?
- Make sure `initializeDPI()` is called BEFORE creating windows
- Check that it's called in `main()` or `WinMain()`, not in a window constructor

### Wrong Sizes?
- Use `getDPIScale()` to get the current scale factor
- Apply scaling to all pixel values (widths, heights, positions)

### Mouse Misalignment?
- Ensure mouse coordinates are scaled: `scaledX = mouseX / dpiScale`

---

**This fix should resolve all custom title bar resolution issues!** 🎉
