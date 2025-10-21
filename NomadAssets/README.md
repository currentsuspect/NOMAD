# NomadAssets

**All handmade and owned assets**

## Purpose

NomadAssets contains all visual and audio resources:
- Custom fonts (no Google Fonts, no borrowed typefaces)
- Hand-crafted icons (SVG format)
- GLSL shaders for rendering
- UI sounds and audio samples

## Structure

```
NomadAssets/
├── fonts/       # Custom typefaces
├── icons/       # SVG icons
├── shaders/     # GLSL vertex/fragment shaders
└── sounds/      # UI feedback sounds
```

## Design Principles

- **No borrowed parts** - Everything is original
- **Optimized** - Small file sizes, fast loading
- **Documented** - Each asset has purpose notes
- **Versioned** - Track changes like code

## Current Assets

- ✅ SVG icons (via nanosvg integration in NomadUI)
- ⏳ Custom fonts (pending)
- ⏳ Shaders (pending organization)
- ⏳ UI sounds (pending)

## Asset Guidelines

- **Icons:** SVG format, 24x24 base size, monochrome with tint support
- **Fonts:** TTF/OTF format, include license if custom
- **Shaders:** GLSL 330 core, documented uniforms
- **Sounds:** WAV format, 48kHz, mono, <100KB

---

*"No borrowed parts, no borrowed soul."*
