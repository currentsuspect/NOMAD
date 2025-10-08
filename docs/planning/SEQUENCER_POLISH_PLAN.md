# 🎨 Sequencer Visual Polish Plan

## Phase A: Visual Improvements (FL Studio Style)

### Current State:
- Basic grid with rectangles
- Simple track labels
- Large cells (32x32)
- Basic colors

### Target State (FL Studio Inspired):
- Compact cells (24x24)
- Step numbers above grid (1, 2, 3, 4...)
- Rounded step buttons that light up
- Darker, more professional colors
- Instrument names on tracks
- Mute/Solo buttons per track
- Better contrast and spacing
- Velocity indicators on notes

---

## Implementation Steps:

### Step 1: Update Grid Appearance ✅
- [x] Smaller cells (24x24)
- [x] Darker background (#0d0d0d)
- [ ] Rounded step buttons
- [ ] Better beat markers
- [ ] Subtle grid lines

### Step 2: Add Step Numbers
- [ ] Row above grid showing 1, 2, 3, 4...
- [ ] Highlight beat numbers (1, 5, 9, 13...)
- [ ] Small, compact font

### Step 3: Improve Track Labels
- [x] Instrument names (Kick, Snare, Hi-Hat...)
- [ ] Mute button (M)
- [ ] Solo button (S)
- [ ] Volume indicator
- [ ] Track color coding

### Step 4: Polish Note Display
- [ ] Rounded rectangles for notes
- [ ] Gradient fill
- [ ] Velocity brightness
- [ ] Glow effect when active
- [ ] Border highlight

### Step 5: Better Playhead
- [x] Small block indicator
- [x] Purple glow
- [ ] Smooth animation
- [ ] Step highlight

---

## Color Scheme (FL Studio Inspired):

```cpp
// Background colors
Background:      #0d0d0d  (very dark)
Grid Cell:       #1a1a1a  (dark gray)
Beat Cell:       #252525  (slightly lighter)
Grid Lines:      #2a2a2a  (subtle)

// Note colors
Note Fill:       #a855f7  (purple)
Note Glow:       #c084fc  (light purple)
Note Border:     #ffffff  (white)
Velocity Bright: #e9d5ff  (very light purple)

// UI elements
Track Label:     #888888  (medium gray)
Mute Button:     #ef4444  (red)
Solo Button:     #22c55e  (green)
Step Numbers:    #666666  (dark gray)
Beat Numbers:    #a855f7  (purple)
```

---

## Layout Changes:

### Before:
```
[Controls: Pattern, Length, Steps/Beat]
[Track 1] [Grid cells...]
[Track 2] [Grid cells...]
```

### After (FL Studio Style):
```
[Pattern Name] [Controls]
              [1][2][3][4][5][6][7][8]... (step numbers)
[Kick    ][M][S] [●][○][●][○][●][○][●][○]...
[Snare   ][M][S] [○][●][○][●][○][●][○][●]...
[Hi-Hat  ][M][S] [●][●][●][●][●][●][●][●]...
```

---

## Next Session Tasks:

1. Implement drawStepNumbers()
2. Update drawGrid() with rounded buttons
3. Update drawTrackLabels() with M/S buttons
4. Update drawNotes() with rounded, glowing notes
5. Add track mute/solo functionality
6. Polish colors and spacing

---

## Phase B Preview (Functional Features):

After visual polish, we'll add:
- Right-click context menu
- Velocity editing (drag up/down)
- Note length editing
- Copy/paste notes
- Clear pattern
- Randomize
- Shift notes
- Undo/redo

---

This will make NOMAD's sequencer look and feel professional! 🎵
