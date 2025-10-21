# NomadCore

**Base utilities layer for NOMAD**

## Purpose

NomadCore provides fundamental utilities used across all NOMAD subsystems:
- Math operations (vectors, matrices, DSP math)
- File I/O and serialization
- Threading primitives and lock-free structures
- Logging and diagnostics
- Memory management utilities

## Structure

```
NomadCore/
├── include/
│   ├── Math/
│   ├── IO/
│   ├── Threading/
│   ├── Logging/
│   └── Memory/
└── src/
```

## Design Principles

- **Zero dependencies** - Pure C++20, no external libs
- **Header-only where possible** - Fast compilation
- **Performance critical** - Used by audio thread
- **Cross-platform** - No OS-specific code here

## Status

⏳ **In Development** - Not yet implemented

## Next Steps

1. Create math utilities (Vector2, Vector3, Matrix4x4)
2. Implement lock-free ring buffer for audio
3. Add file I/O abstractions
4. Build logging system

---

*"Clarity before speed."*
