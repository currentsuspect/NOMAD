# NomadPlat

**Cross-platform OS abstraction layer**

## Purpose

NomadPlat handles all platform-specific code:
- Window creation and management
- OpenGL/Vulkan context setup
- Input events (keyboard, mouse, touch)
- File dialogs and system integration
- Timers and high-resolution clocks

## Structure

```
NomadPlat/
├── include/
│   ├── Platform.h
│   ├── Window.h
│   ├── Input.h
│   └── Timer.h
└── src/
    ├── Win32/      # Windows implementation
    ├── X11/        # Linux implementation
    ├── Cocoa/      # macOS implementation
    └── Shared/     # Common utilities
```

## Design Principles

- **Single responsibility** - Only platform abstraction
- **No rendering** - That's NomadUI's job
- **No audio** - That's NomadAudio's job
- **Clean interfaces** - Platform details hidden

## Current State

⏳ **Extraction Needed** - Some Win32 code exists in NomadUI/Platform/

## Next Steps

1. Extract Win32 window code from NomadUI
2. Create platform abstraction interface
3. Implement X11 backend for Linux
4. Implement Cocoa backend for macOS

---

*"All OS-specific logic lives inside NomadPlat."*
