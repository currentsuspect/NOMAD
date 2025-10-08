# NOMAD DAW - Purple Glow Theme

## Overview

NOMAD now features a stunning purple glow theme that creates a modern, professional, and visually cohesive interface.

## Theme Color

**Primary Purple**: `#a855f7` (RGB: 168, 85, 247)
- Vibrant, energetic purple
- High visibility without being overwhelming
- Perfect for dark theme interfaces

## Implemented Features

### 1. NOMAD Title with Purple Glow ✓
- Multi-layer glow effect for depth
- Purple color with subtle outer glow
- Creates a premium, branded look

**Implementation:**
- 3 layers of decreasing alpha for glow effect
- Main text in full purple color
- Positioned in top-left of title bar

### 2. Purple Grid System ✓
- Compact, endless-feeling grid
- Purple glow on all grid lines
- More compact spacing for modern feel

**Grid Specifications:**
- Horizontal spacing: 48px (down from 60px)
- Vertical spacing: 64px (down from 80px)
- Grid lines have subtle glow effect
- Alpha values: 0.15 for horizontal, 0.12 for vertical

### 3. Time Counter Enhancement ✓
- Frosted glass background effect
- Purple glow outline
- Modern, clock-like font (24px monospace)
- Simplified format (no extra zeros)

**Format Examples:**
- `1:1:000` instead of `001:1:000`
- `12:3:480` instead of `012:3:480`
- Clean, absolute display

**Visual Effects:**
- Semi-transparent dark background (0.6 alpha)
- Double purple outline (inner and outer glow)
- Rounded corners (6px radius)

### 4. Transport Button Hover Glows ✓
- Purple glow on Play and Stop buttons
- Red glow maintained for Record button
- Smooth, radial gradient effect

**Glow Specifications:**
- Radius: 20px
- Alpha: 0.4 at center, fading to 0
- Activates on mouse hover

### 5. Window Control Button Glows ✓
- Purple glow on Minimize and Maximize
- Red glow maintained for Close button
- Radial gradient with 16px radius

### 6. Settings Button Purple Theme ✓
- Purple text color on hover
- Purple background tint on hover
- Consistent with overall theme

### 7. BPM Display Purple Theme ✓
- Purple text when editing
- Purple outline when focused
- Matches time counter aesthetic

### 8. Status Bar Purple Accent ✓
- Purple glow line at top of status bar
- Double-layer effect for depth
- Replaces previous green accent

### 9. Resizer Purple Tint ✓
- Purple-tinted grip lines
- Thicker lines (1.5px) for visibility
- Subtle but noticeable

## Visual Hierarchy

### Primary Elements (Full Purple)
- NOMAD title text
- Time counter text
- Active/editing states

### Secondary Elements (Purple Glow)
- Grid lines
- Button hover states
- Window control hovers
- Status bar accent

### Tertiary Elements (Purple Tint)
- Resizer grip
- Subtle backgrounds

## Color Palette

```
Primary Purple:   #a855f7 (Full intensity)
Purple Glow:      #a855f7 with 0.4 alpha
Purple Subtle:    #a855f7 with 0.15 alpha
Purple Tint:      #a855f7 with 0.08 alpha

Background Dark:  #0d0e0f
UI Dark:          #151618
Frosted Glass:    #1a1a1a with 0.6 alpha
```

## Design Philosophy

### 1. Consistency
- Purple used throughout for interactive elements
- Consistent glow radius and alpha values
- Unified visual language

### 2. Hierarchy
- Brighter purple for primary elements
- Subtle purple for secondary elements
- Creates clear visual flow

### 3. Modern Aesthetic
- Glow effects add depth
- Frosted glass for premium feel
- Clean, minimal design

### 4. Usability
- High contrast for readability
- Clear hover states
- Intuitive visual feedback

## Technical Implementation

### Glow Effect Pattern
```cpp
juce::Colour purpleGlow(0xffa855f7);

// Radial gradient glow
juce::ColourGradient glow(
    purpleGlow.withAlpha(0.4f), centerX, centerY,
    purpleGlow.withAlpha(0.0f), centerX + radius, centerY,
    true);

g.setGradientFill(glow);
g.fillEllipse(bounds.expanded(radius));
```

### Multi-Layer Glow Pattern
```cpp
// Outer glow layers
for (int i = 3; i > 0; --i)
{
    g.setColour(purpleGlow.withAlpha(0.15f * i));
    g.drawText(text, bounds.expanded(i, i), justification, true);
}

// Main text
g.setColour(purpleGlow);
g.drawText(text, bounds, justification, true);
```

### Frosted Glass Pattern
```cpp
// Background
g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.6f));
g.fillRoundedRectangle(bounds, cornerRadius);

// Glow outline
g.setColour(purpleGlow.withAlpha(0.3f));
g.drawRoundedRectangle(bounds.expanded(1), cornerRadius, 2.0f);

g.setColour(purpleGlow.withAlpha(0.5f));
g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
```

## Files Modified

### Core Theme Files
- `Source/MainComponent.cpp` - Grid, title, status bar
- `Source/UI/TransportComponent.cpp` - Time counter, buttons, BPM
- `Source/UI/TransportComponent.h` - Button glow method
- `Source/UI/WindowControlButton.h` - Window control glows
- `Source/UI/CustomResizer.h` - Resizer purple tint

## Future Enhancements

Potential additions to the purple theme:

1. **Animated Glow Pulse**
   - Subtle pulsing on active elements
   - Breathing effect on focused items

2. **Selection States**
   - Purple highlight for selected tracks
   - Purple outline for focused controls

3. **Progress Indicators**
   - Purple progress bars
   - Purple loading animations

4. **Waveform Display**
   - Purple waveform rendering
   - Purple peak indicators

5. **Plugin UI**
   - Purple accent colors in plugin windows
   - Consistent theme across all UI

6. **Scrollbars**
   - Purple thumb color
   - Purple track on hover

7. **Context Menus**
   - Purple highlight on hover
   - Purple selection background

8. **Tooltips**
   - Purple border
   - Purple accent text

## Performance Notes

- Glow effects use gradient fills (GPU accelerated)
- Multi-layer text rendering is minimal (3 layers max)
- Hover states only render when needed
- No performance impact on audio processing

## Accessibility

- High contrast maintained (purple on dark)
- Clear visual feedback for all interactions
- Consistent color usage aids navigation
- No reliance on color alone for information

## Conclusion

The purple glow theme transforms NOMAD into a modern, professional DAW with a unique visual identity. The consistent use of purple throughout creates a cohesive, premium experience while maintaining excellent usability.
