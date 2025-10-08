# NOMAD DAW

A modern, GPU-accelerated Digital Audio Workstation built from the ground up for performance and workflow efficiency.

## Project Structure

```
/NOMAD
├── NomadUI/          # Custom UI framework (GPU-accelerated, OpenGL-based)
├── Source/           # Core DAW logic (audio engine, sequencer, models)
├── docs/             # Documentation and design philosophy
└── tests/            # Test suites
```

## Components

### NomadUI Framework
Custom UI framework designed for real-time audio applications:
- GPU-accelerated rendering with OpenGL 3.3+
- Component-based architecture
- FL Studio-inspired dark theme
- Smooth animations and interactions
- See `NomadUI/README.md` for details

### NOMAD Core
The DAW engine and logic:
- Real-time audio processing
- Pattern-based sequencer
- Playlist/arrangement view
- Audio clip management
- JUCE-based audio backend

## Building

### Prerequisites
- CMake 3.15+
- C++17 compiler (MSVC, GCC, or Clang)
- OpenGL 3.3+ support

### Build Instructions

```bash
# Build NomadUI
cd NomadUI
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
./bin/Debug/NomadUI_MinimalTest.exe

# Build NOMAD DAW
cd ../..
mkdir build && cd build
cmake ..
cmake --build .
```

## Development Status

- ✅ NomadUI Framework v0.1.0 - Core complete
- ✅ Sequencer engine with pattern/playlist modes
- ✅ Audio clip drag & drop
- ✅ Transport controls
- 🚧 Windows platform layer (in progress)
- 🚧 Additional UI widgets
- 📋 Plugin hosting (planned)
- 📋 Mixer view (planned)

## License

[Your License Here]

## Contributing

[Contributing guidelines]
