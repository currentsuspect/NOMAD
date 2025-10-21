# 🜂 NOMAD

**Built from scratch. Perfected with intention.**

A hand-engineered digital audio workstation and creative operating system — designed to feel human, timeless, and optimized for creation.

---

## Philosophy

> "Every pixel and buffer here exists because someone decided it should. There is no automation without understanding, no shortcut without intent."

NOMAD is a fully self-authored DAW with zero borrowed parts. Every layer — from windowing to audio I/O to rendering — is crafted by hand.

---

## Architecture

```
┌─────────────────────────────────────────┐
│           NOMAD Creative System         │
├─────────────────────────────────────────┤
│  NomadUI      │  Rendering & Components │
│  NomadPlat    │  Platform Abstraction   │
│  NomadAudio   │  RtAudio + DSP Engine   │
│  NomadCore    │  Base Utilities         │
│  NomadSDK     │  Plugin System          │
└─────────────────────────────────────────┘
```

### Current State: **v1.0 Foundation**

- ✅ **NomadUI** - Complete rendering engine with OpenGL
- ✅ **Build System** - Clean CMake, no JUCE dependency
- ⏳ **NomadCore** - In development
- ⏳ **NomadPlat** - Platform layer extraction
- ⏳ **NomadAudio** - RtAudio integration pending

---

## Quick Start

### Build
```bash
.\build.ps1
```

### Requirements
- CMake 3.22+
- Visual Studio 2022 (Windows)
- C++20 compiler

---

## Project Structure

```
Nomad/
├── NomadCore/      # Base utilities (math, I/O, threading)
├── NomadPlat/      # Platform abstraction (Win32, X11, Cocoa)
├── NomadUI/        # Rendering engine (OpenGL, Vulkan)
├── NomadAudio/     # Audio engine (RtAudio + DSP)
├── NomadSDK/       # Plugin system (v3.0)
├── NomadAssets/    # Fonts, icons, shaders, sounds
└── NomadDocs/      # Documentation and philosophy
```

## Documentation

- **[NomadDocs/NOMAD_AI_BRIEF.md](NomadDocs/NOMAD_AI_BRIEF.md)** - Architectural source of truth
- **[NomadDocs/BRANCHING_STRATEGY.md](NomadDocs/BRANCHING_STRATEGY.md)** - Git workflow and philosophy
- **[NomadDocs/BUILD_STATUS.md](NomadDocs/BUILD_STATUS.md)** - Current build state

Each subsystem has its own README:
- [NomadCore/README.md](NomadCore/README.md)
- [NomadPlat/README.md](NomadPlat/README.md)
- [NomadUI/README.md](NomadUI/README.md)
- [NomadAudio/README.md](NomadAudio/README.md)
- [NomadSDK/README.md](NomadSDK/README.md)
- [NomadAssets/README.md](NomadAssets/README.md)

---

## Design Principles

- **Intentional Minimalism** - Nothing extra, everything essential
- **Depth Through Motion** - Animation defines hierarchy
- **Performance as Aesthetic** - Fluidity = design quality
- **Analog Feel** - Subtle imperfections give realism
- **Clarity Over Decoration** - Spacing and light over gloss
- **Silence and Soul** - Design should evoke focus, not noise

> "Nomad should look like it was designed in silence, but feels loud inside."

---

## The Nomad Code

- Clarity before speed
- If it's slow, it's sacred
- No borrowed parts, no borrowed soul
- Simplify before optimizing
- Write for your future self
- Document as you build
- Elegance is correctness you can feel

---

## Roadmap

| Phase | Description | Status |
|-------|-------------|--------|
| **v1.0** | NomadUI + NomadPlat + RtAudio backend | 🟡 In Progress |
| **v1.5** | DSP and Mixer layer | ⚪ Planned |
| **v2.0** | Native audio drivers (ASIO, CoreAudio, PipeWire) | ⚪ Planned |
| **v3.0** | Plugin host and full modular DAW | ⚪ Planned |
| **v∞** | Self-contained creative operating system | ⚪ Vision |

---

## Contributing

Read **[NOMAD_AI_BRIEF.md](NOMAD_AI_BRIEF.md)** first. Understand the philosophy. Follow **[BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md)** for all commits.

When you write for Nomad, write like it will be read decades from now.

---

## License

Proprietary. All rights reserved.

---

**Welcome to NOMAD.**

*Build like silence is watching.*
