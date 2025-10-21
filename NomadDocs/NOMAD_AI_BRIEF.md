# 🜂 NOMAD CREATIVE SYSTEMS – DEVELOPER BRIEF

## 1. Purpose

This document defines how all AI systems, automation agents, and contributors (human or not) must operate within the NOMAD ecosystem. Its goal is to guarantee architectural purity, consistency, and the long-term vision of NOMAD as a fully self-authored digital audio workstation and creative operating system.

## 2. Identity

**Project Name:** NOMAD  
**Nature:** Independent DAW and creative framework  
**Philosophy:** "Built from scratch. Perfected with intention."  
**Mission:** To craft a hand-engineered, visually and sonically alive environment — one that feels human, timeless, and optimized for creation.

## 3. Architectural Overview

| Layer | Purpose | Language / Tech |
|-------|---------|----------------|
| **NomadCore** | Base utilities: math, file I/O, threading, logging, serialization | C++ |
| **NomadPlat** | Cross-platform OS layer: windowing, input, timing, platform abstraction | C++ / Win32 / X11 / Cocoa |
| **NomadUI** | Rendering engine, theming, layout, animation, and component system | C++ / OpenGL / Vulkan |
| **NomadAudio** | Real-time audio engine using RtAudio for cross-platform I/O | C++ |
| **NomadSDK** | Plugin/extension API for third-party or internal modules | C++ |
| **NomadAssets** | Fonts, icons, shaders, sounds — all handmade and owned | — |
| **NomadDocs** | Documentation, philosophy, and internal design bibles | Markdown |

## 4. Development State

### NOMAD v1 (current):
- Full NomadPlat for windowing and input
- NomadUI for rendering and user interaction
- NomadAudio powered by RtAudio for audio I/O
- No JUCE dependency

### NOMAD v2:
- Add NomadMixer and DSP modules
- Begin custom driver implementations (ASIO / CoreAudio / PipeWire backends)

### NOMAD v3:
- Introduce NomadPluginHost and NomadAudioGraph
- Fully self-contained DAW core

## 5. Technical Directives for AI Systems

### Respect architectural boundaries
- Do not mix UI, audio, and platform code
- No external frameworks beyond approved libs

### Language standards
- C++20+, RAII, smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- 4-space indentation, no tabs
- PascalCase for classes, camelCase for methods

### Performance priority
- Target 60 FPS UI and <10 ms audio latency on 4 GB RAM systems
- Avoid unnecessary heap allocations; use stack and pools

### Audio layer rules
- All real-time I/O through RtAudio
- No blocking calls in audio thread
- Use lock-free queues for UI ↔ Audio communication

### Rendering layer rules
- All draw calls go through NUIRendererGL or NUIRendererVK
- No third-party renderers or immediate-mode UIs

### Cross-platform discipline
- All OS-specific logic lives inside NomadPlat
- No Win32 calls outside the platform layer

### Documentation
- Doxygen headers for every class
- Each subsystem keeps a README.md describing purpose and design

## 6. Audio System Design

### 🔊 RtAudio Backend (v1)

**Purpose:** Cross-platform low-latency audio I/O

**Supported APIs:**
- Windows: WASAPI, ASIO
- macOS: CoreAudio
- Linux: ALSA, JACK, PipeWire-compatible

**License:** MIT — fully commercial safe

### 🎚 NomadAudio Structure

```
NomadAudio/
  ├── include/
  │   ├── NomadAudio.h
  │   ├── AudioDeviceManager.h
  │   ├── AudioDriver.h
  │   ├── Mixer.h
  │   ├── DSP/
  │   │   ├── Filters.h
  │   │   ├── Oscillator.h
  │   │   └── Envelope.h
  │   └── Utils/
  │       ├── RingBuffer.h
  │       └── AudioMath.h
  └── src/
      ├── RtAudioBackend.cpp
      ├── Mixer.cpp
      └── DSP/
```

**Flow:**
```
RtAudioCallback → NomadAudioEngine → MixerBus → OutputBuffer
```

Later, replace or extend RtAudioBackend with:
- `ASIODriver.cpp`
- `CoreAudioDriver.mm`
- `PipeWireDriver.cpp`

## 7. Platform Layer – NomadPlat

### Responsibilities
- Window creation / destruction
- OpenGL / Vulkan context management
- Keyboard, mouse, and input events
- File dialogs, timers, and clipboard
- OS abstraction macros and utilities

### Structure
```
NomadPlat/
  ├── include/
  │   ├── Platform.h
  │   ├── Window.h
  │   ├── Input.h
  │   └── Timer.h
  └── src/
      ├── Win32/
      ├── X11/
      ├── Cocoa/
      └── Shared/
```

## 8. Design Principles

- **Intentional Minimalism:** nothing extra, everything essential
- **Depth Through Motion:** animation defines hierarchy
- **Performance as Aesthetic:** fluidity = design quality
- **Analog Feel:** subtle imperfections give realism
- **Clarity Over Decoration:** spacing and light over gloss
- **Silence and Soul:** design should evoke focus, not noise

> "Nomad should look like it was designed in silence, but feels loud inside."

## 9. Workflow Philosophy

### Creation Ritual
1. **Concept:** define emotion + purpose first
2. **Prototype:** build fast, prove logic
3. **Refine:** optimize, polish visuals
4. **Ritualize:** document and lock into standard

### Daily Cycle
1. Read a paragraph from Nomad Bible
2. Code one subsystem with total presence
3. Write one Nomad Lesson reflection at day's end

## 10. The Nomad Code (Values for Every Contributor)

- Clarity before speed
- If it's slow, it's sacred
- No borrowed parts, no borrowed soul
- Simplify before optimizing
- Write for your future self
- Document as you build
- Elegance is correctness you can feel

## 11. Repository Layout (Reference)

```
Nomad/
├── NomadCore/
├── NomadPlat/
│   ├── src/Win32/
│   ├── src/X11/
│   ├── src/Cocoa/
│   └── Shared/
├── NomadUI/
│   ├── Core/
│   ├── Graphics/
│   └── Platform/
├── NomadAudio/
│   ├── include/
│   └── src/
├── NomadSDK/
├── NomadAssets/
├── NomadDocs/
│   ├── NOMAD_BIBLE.md
│   ├── NOMAD_AI_BRIEF.md
│   ├── ARCHITECTURE.md
│   └── STYLE_GUIDE.md
└── CMakeLists.txt
```

## 12. Long-Term Vision

| Phase | Description |
|-------|-------------|
| **v1.0** | Full NomadUI + NomadPlat + RtAudio backend |
| **v1.5** | Add DSP and Mixer layer |
| **v2.0** | Implement NomadAudio native drivers (ASIO, CoreAudio, PipeWire) |
| **v3.0** | Launch NomadPluginHost and full modular DAW |
| **v∞** | Self-contained creative operating system |

## 13. Preface / Manifesto

You are stepping into a system built by hand. Every pixel and buffer here exists because someone decided it should. There is no automation without understanding, no shortcut without intent.

When you write for Nomad, write like it will be read decades from now. Build like silence is watching.

**Welcome to NOMAD.**
