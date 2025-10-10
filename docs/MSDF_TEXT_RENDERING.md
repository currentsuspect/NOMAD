# MSDF Text Rendering System

This document describes the modern GPU-driven MSDF (Multi-channel Signed Distance Field) text rendering system that replaces the legacy text rendering subsystem.

## Overview

The MSDF text renderer provides:
- **Crisp text at any scale** with proper anti-aliasing
- **GPU-driven rendering** with minimal CPU overhead
- **Single packed atlas** containing all ASCII characters (32-126)
- **Support for outlines and glow effects** via shader parameters
- **Efficient batching** with single draw call per text string

## Architecture

### Core Components

1. **TextRenderer** (`src/text/TextRenderer.h/cpp`)
   - Main API for text rendering
   - Manages font loading, atlas generation, and rendering
   - Provides simple, clean interface

2. **MSDFGenerator** (`src/text/MSDFGenerator.h/cpp`)
   - Wrapper around MSDF generation
   - Supports both external msdfgen library and integrated implementation
   - Handles FreeType outline to MSDF conversion

3. **AtlasPacker** (`src/text/AtlasPacker.h/cpp`)
   - Skyline-based texture atlas packing
   - Efficiently packs glyph bitmaps into single texture
   - Optimized for text glyphs of varying sizes

4. **Shaders** (`shaders/text_msdf.vs/fs`)
   - Vertex shader: transforms positions and passes data
   - Fragment shader: reconstructs signed distance and applies anti-aliasing

## API Usage

### Basic Setup

```cpp
#include "src/text/TextRenderer.h"

// Initialize renderer
TextRenderer renderer;
if (!renderer.init("path/to/font.ttf", 48, 2048)) {
    // Handle error
}

// Set MSDF parameters for quality
renderer.setSDFParams(4.0f, 0.5f);  // pxRange, smoothing
```

### Rendering Text

```cpp
// Draw text at position
renderer.drawText(100.0f, 100.0f, "Hello World", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);

// Draw scaled text
renderer.drawText(100.0f, 150.0f, "Scaled Text", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 1.5f);

// Measure text dimensions
float width, height;
renderer.measureText("Test String", width, height, 1.0f);
```

### Effects

```cpp
// Outline effect (render black text behind white text)
renderer.drawText(22.0f, 100.0f, "Outline", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
renderer.drawText(20.0f, 100.0f, "Outline", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);

// Glow effect (multiple renders with different colors)
renderer.drawText(22.0f, 120.0f, "Glow", glm::vec4(0.0f, 0.0f, 0.0f, 0.5f), 1.0f);
renderer.drawText(21.0f, 120.0f, "Glow", glm::vec4(1.0f, 0.0f, 1.0f, 0.7f), 1.0f);
renderer.drawText(20.0f, 120.0f, "Glow", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
```

## Technical Details

### MSDF Generation

The system generates 3-channel MSDF bitmaps for each character:
- **R channel**: Distance to nearest edge (horizontal)
- **G channel**: Distance to nearest edge (vertical)  
- **B channel**: Distance to nearest edge (diagonal)

The fragment shader uses the median of these three values to reconstruct the signed distance field, providing better corner preservation than single-channel SDF.

### Atlas Packing

- Uses skyline algorithm for efficient packing
- Supports variable-sized glyphs
- Typical efficiency: 80-90% for ASCII character set
- Single 2048x2048 texture contains all characters

### Shader Parameters

- **uPxRange**: Distance field range in pixels (affects sharpness)
- **uSmoothing**: Additional smoothing factor
- **uThickness**: Outline thickness (0.5 = normal, <0.5 = outline, >0.5 = glow)
- **uScale**: Scale factor for adaptive smoothing

### Performance

- **Atlas generation**: Once at startup
- **Rendering**: Single draw call per text string
- **Memory**: ~12MB for 2048x2048 atlas (3 channels)
- **GPU**: Minimal state changes, efficient batching

## Dependencies

### Required
- **FreeType2**: Font loading and outline extraction
- **OpenGL 3.3+**: Rendering backend
- **GLM**: Mathematics library
- **GLAD**: OpenGL function loading

### Optional
- **msdfgen**: External MSDF generation library (falls back to integrated implementation)

## Building

### Windows
```batch
scripts\build_msdf_demo.bat
```

### Linux/macOS
```bash
scripts/build_msdf_demo.sh
```

### Manual Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make TEXT_DEMO -j$(nproc)
```

## Demo Program

The `TEXT_DEMO` executable demonstrates:
- Basic text rendering at different scales
- Outline and glow effects
- Multi-line text support
- Performance testing with long strings
- ASCII character set display

Run the demo to verify the MSDF text renderer is working correctly.

## Migration from Legacy System

The old text rendering system has been completely removed:
- `NUIFont.h/cpp` - Deleted
- `NUITextRenderer.h/cpp` - Deleted
- FreeType dependency moved to MSDF system
- OpenGL renderer updated with placeholder text methods

## Future Enhancements

- **Unicode support**: Extend beyond ASCII 32-126
- **Font fallbacks**: Multiple font support
- **Kerning**: Advanced typography features
- **Offline atlas generation**: Pre-generate atlases for common fonts
- **GPU memory management**: Dynamic atlas expansion
- **Text effects**: Drop shadows, gradients, etc.

## Troubleshooting

### Common Issues

1. **Font not found**: Ensure font path is correct and file exists
2. **FreeType not found**: Install libfreetype6-dev (Ubuntu) or freetype-devel (CentOS)
3. **GLFW not found**: Install libglfw3-dev (Ubuntu) or glfw-devel (CentOS)
4. **OpenGL context**: Ensure OpenGL 3.3+ context is available

### Debug Output

The system provides detailed logging:
- Font loading status
- Atlas generation progress
- Shader compilation results
- Packing efficiency statistics

Check console output for diagnostic information.