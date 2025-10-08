# Design Document - NOMAD DAW

## Overview

NOMAD is architected as a modular, high-performance DAW built on the JUCE framework. The design emphasizes separation of concerns with distinct layers for audio processing, data management, and user interface. The architecture supports real-time audio processing with lock-free communication between the audio thread and UI thread, ensuring glitch-free performance even under heavy load.

### Technology Stack

- **Framework:** JUCE 7.x (cross-platform C++ framework for audio applications)
- **Language:** C++17/20
- **Build System:** CMake
- **Audio APIs:** ASIO (Windows), WASAPI (Windows), ALSA/JACK (Linux)
- **Plugin Formats:** VST2, VST3, AU (via JUCE plugin hosting)
- **Graphics:** JUCE Graphics with OpenGL acceleration for complex visualizations
- **File Formats:** Custom XML-based project format, standard MIDI, WAV/FLAC/OGG export

### Design Principles

1. **Real-time Safety:** Audio processing code is lock-free and allocation-free
2. **Modularity:** Each major component (sequencer, mixer, piano roll) is independently testable
3. **Scalability:** Architecture supports hundreds of tracks and plugins
4. **Extensibility:** Plugin-based architecture allows future expansion
5. **Cross-platform:** Platform-specific code isolated in abstraction layers

## Architecture

### High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        UI Layer (JUCE)                       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │Sequencer │ │Piano Roll│ │  Mixer   │ │ Playlist │       │
│  │   View   │ │   View   │ │   View   │ │   View   │       │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘       │
└───────┼────────────┼────────────┼────────────┼─────────────┘
        │            │            │            │
┌───────┼────────────┼────────────┼────────────┼─────────────┐
│       │      Application Controller Layer    │             │
│  ┌────▼──────────────────────────────────────▼─────┐       │
│  │         Command Manager & State Manager         │       │
│  └────┬──────────────────────────────────────┬─────┘       │
└───────┼──────────────────────────────────────┼─────────────┘
        │                                      │
┌───────┼──────────────────────────────────────┼─────────────┐
│       │          Data Model Layer            │             │
│  ┌────▼─────┐ ┌──────────┐ ┌──────────┐ ┌───▼──────┐      │
│  │ Pattern  │ │  Track   │ │Automation│ │  Mixer   │      │
│  │  Model   │ │  Model   │ │  Model   │ │  Model   │      │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘      │
└───────┼────────────┼────────────┼────────────┼─────────────┘
        │            │            │            │
┌───────┼────────────┼────────────┼────────────┼─────────────┐
│       │         Audio Engine Layer           │             │
│  ┌────▼──────────────────────────────────────▼─────┐       │
│  │              Audio Graph Manager                │       │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐        │       │
│  │  │Sequencer │ │  Plugin  │ │  Mixer   │        │       │
│  │  │ Engine   │ │  Host    │ │  Engine  │        │       │
│  │  └──────────┘ └──────────┘ └──────────┘        │       │
│  └────────────────────┬──────────────────────────┘       │
└────────────────────────┼────────────────────────────────────┘
                         │
                    ┌────▼─────┐
                    │  Audio   │
                    │  Device  │
                    └──────────┘
```

### Threading Model

1. **Audio Thread:** High-priority real-time thread for audio processing
   - Lock-free communication with other threads
   - No memory allocation or blocking operations
   - Fixed buffer size processing

2. **UI Thread:** Main thread for user interface and event handling
   - Handles user input and renders graphics
   - Communicates with audio thread via lock-free queues

3. **Background Threads:** For non-real-time operations
   - File I/O (loading/saving projects)
   - Plugin scanning
   - Audio file loading and caching

### Data Flow

```
User Input → UI Component → Command → State Manager → Data Model
                                                           ↓
                                                    Lock-free Queue
                                                           ↓
                                                    Audio Engine
                                                           ↓
                                                    Audio Output
```

## Components and Interfaces

### 1. Audio Engine Core

#### AudioEngine Class
```cpp
class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    void audioDeviceIOCallback(const float** inputChannelData,
                              int numInputChannels,
                              float** outputChannelData,
                              int numOutputChannels,
                              int numSamples) override;
    
    void setTransportState(TransportState state);
    void setPlayheadPosition(double positionInSeconds);
    double getSampleRate() const;
    int getBlockSize() const;
    
private:
    AudioGraphManager graphManager;
    TransportController transport;
    std::atomic<double> playheadPosition;
};
```

#### AudioGraphManager
Manages the audio processing graph with nodes for instruments, effects, and mixer channels.

```cpp
class AudioGraphManager
{
public:
    void addNode(std::unique_ptr<AudioNode> node);
    void removeNode(NodeID id);
    void connectNodes(NodeID source, NodeID destination);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    
private:
    std::vector<std::unique_ptr<AudioNode>> nodes;
    std::vector<Connection> connections;
    void topologicalSort(); // For efficient processing order
};
```

### 2. Pattern-Based Sequencer

#### Pattern Class
```cpp
class Pattern
{
public:
    struct Note {
        int step;           // Step position in pattern
        int track;          // Track index
        int pitch;          // MIDI note number
        float velocity;     // 0.0 to 1.0
        int duration;       // Duration in steps
    };
    
    void addNote(const Note& note);
    void removeNote(int step, int track);
    std::vector<Note> getNotesInRange(int startStep, int endStep) const;
    void setLength(int steps);
    void setStepsPerBeat(int steps);
    
private:
    std::vector<Note> notes;
    int lengthInSteps;
    int stepsPerBeat;
    juce::String name;
};
```

#### SequencerEngine
Processes patterns and generates MIDI events for the audio engine.

```cpp
class SequencerEngine
{
public:
    void processBlock(juce::MidiBuffer& midiMessages, 
                     double startTime, 
                     double endTime,
                     double sampleRate);
    
    void setActivePattern(PatternID id);
    void setLoopEnabled(bool enabled);
    
private:
    std::map<PatternID, Pattern> patterns;
    PatternID activePattern;
    double currentPosition;
};
```

### 3. Piano Roll

#### PianoRollModel
```cpp
class PianoRollModel
{
public:
    struct MidiNote {
        double startTime;   // In beats
        double duration;    // In beats
        int pitch;          // MIDI note number (0-127)
        int velocity;       // MIDI velocity (0-127)
        int channel;        // MIDI channel
    };
    
    void addNote(const MidiNote& note);
    void removeNote(NoteID id);
    void moveNote(NoteID id, double newStartTime, int newPitch);
    void resizeNote(NoteID id, double newDuration);
    void setNoteVelocity(NoteID id, int velocity);
    
    std::vector<MidiNote> getNotesInRange(double startTime, double endTime,
                                          int minPitch, int maxPitch) const;
    
    void quantize(double gridSize);
    void importMidi(const juce::File& file);
    void exportMidi(const juce::File& file);
    
private:
    std::map<NoteID, MidiNote> notes;
    std::vector<CCEvent> ccData; // Control Change data
    NoteID nextNoteID;
};
```

#### PianoRollView
```cpp
class PianoRollView : public juce::Component
{
public:
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    void setZoom(float horizontal, float vertical);
    void setSnapGrid(double gridSize);
    
private:
    PianoRollModel& model;
    float horizontalZoom;
    float verticalZoom;
    double snapGridSize;
    
    void drawPianoKeys(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
};
```

### 4. Mixer

#### MixerChannel
```cpp
class MixerChannel
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    void setGain(float gainDb);
    void setPan(float pan); // -1.0 (left) to 1.0 (right)
    void setSolo(bool solo);
    void setMute(bool mute);
    
    void addInsertEffect(std::unique_ptr<PluginInstance> plugin);
    void addSendEffect(SendID id, float level);
    
    float getCurrentLevel() const; // For metering
    
private:
    std::atomic<float> gain;
    std::atomic<float> pan;
    std::atomic<bool> isSolo;
    std::atomic<bool> isMuted;
    
    std::vector<std::unique_ptr<PluginInstance>> insertEffects;
    std::map<SendID, float> sends;
    
    juce::dsp::Gain<float> gainProcessor;
    juce::dsp::Panner<float> panProcessor;
    LevelMeter meter;
};
```

#### MixerEngine
```cpp
class MixerEngine
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    ChannelID addChannel(const juce::String& name);
    void removeChannel(ChannelID id);
    MixerChannel& getChannel(ChannelID id);
    
    void routeChannel(ChannelID source, ChannelID destination);
    void createGroup(const std::vector<ChannelID>& channels);
    
private:
    std::map<ChannelID, std::unique_ptr<MixerChannel>> channels;
    std::vector<ChannelRoute> routing;
    MixerChannel masterChannel;
};
```

### 5. Playlist/Arrangement

#### PlaylistClip
```cpp
struct PlaylistClip
{
    ClipID id;
    double startTime;       // In beats
    double duration;        // In beats
    PatternID patternRef;   // Reference to pattern
    int trackIndex;
    juce::Colour color;
    
    double getEndTime() const { return startTime + duration; }
};
```

#### PlaylistModel
```cpp
class PlaylistModel
{
public:
    ClipID addClip(const PlaylistClip& clip);
    void removeClip(ClipID id);
    void moveClip(ClipID id, double newStartTime, int newTrack);
    void resizeClip(ClipID id, double newDuration);
    
    std::vector<PlaylistClip> getClipsInRange(double startTime, double endTime) const;
    
    void setLoopRegion(double start, double end);
    void setTimeSignature(int numerator, int denominator);
    
private:
    std::map<ClipID, PlaylistClip> clips;
    std::optional<std::pair<double, double>> loopRegion;
    TimeSignature timeSignature;
};
```

### 6. Automation

#### AutomationClip
```cpp
class AutomationClip
{
public:
    struct Point {
        double time;        // In beats
        float value;        // Normalized 0.0 to 1.0
        CurveType curve;    // Linear, Exponential, Bezier
    };
    
    void addPoint(const Point& point);
    void removePoint(int index);
    void movePoint(int index, double newTime, float newValue);
    
    float getValueAtTime(double time) const;
    
    void setTargetParameter(ParameterID param);
    ParameterID getTargetParameter() const;
    
private:
    std::vector<Point> points;
    ParameterID targetParameter;
    
    float interpolate(const Point& p1, const Point& p2, double time) const;
};
```

#### AutomationEngine
```cpp
class AutomationEngine
{
public:
    void processBlock(double startTime, double endTime);
    
    void addAutomationClip(std::unique_ptr<AutomationClip> clip);
    void removeAutomationClip(ClipID id);
    
    // Called from audio thread
    float getParameterValue(ParameterID param, double time) const;
    
private:
    std::map<ClipID, std::unique_ptr<AutomationClip>> clips;
    std::map<ParameterID, std::vector<ClipID>> parameterToClips;
};
```

### 7. Plugin Hosting

#### PluginManager
```cpp
class PluginManager
{
public:
    void scanForPlugins(const std::vector<juce::File>& directories);
    std::vector<PluginDescription> getAvailablePlugins() const;
    
    std::unique_ptr<PluginInstance> loadPlugin(const PluginDescription& desc);
    
private:
    juce::KnownPluginList knownPlugins;
    juce::AudioPluginFormatManager formatManager;
};
```

#### PluginInstance
```cpp
class PluginInstance
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    
    void setParameter(int index, float value);
    float getParameter(int index) const;
    int getNumParameters() const;
    juce::String getParameterName(int index) const;
    
    void showEditor();
    void hideEditor();
    
    juce::MemoryBlock getState() const;
    void setState(const juce::MemoryBlock& state);
    
private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
};
```

### 8. Transport and Timing

#### TransportController
```cpp
class TransportController
{
public:
    enum State { Stopped, Playing, Recording };
    
    void play();
    void stop();
    void record();
    void setPosition(double timeInBeats);
    
    double getPosition() const;
    State getState() const;
    
    void setTempo(double bpm);
    double getTempo() const;
    
    void setLoopEnabled(bool enabled);
    void setLoopPoints(double start, double end);
    
private:
    std::atomic<State> state;
    std::atomic<double> position;
    std::atomic<double> tempo;
    std::atomic<bool> loopEnabled;
    double loopStart, loopEnd;
};
```

## Data Models

### Project File Structure

Projects are saved as XML with the following structure:

```xml
<NomadProject version="1.0">
    <Settings>
        <Tempo>120.0</Tempo>
        <TimeSignature numerator="4" denominator="4"/>
        <SampleRate>44100</SampleRate>
    </Settings>
    
    <Patterns>
        <Pattern id="1" name="Kick Pattern" length="16" stepsPerBeat="4">
            <Note step="0" track="0" pitch="36" velocity="1.0" duration="1"/>
            <Note step="4" track="0" pitch="36" velocity="0.8" duration="1"/>
        </Pattern>
    </Patterns>
    
    <Tracks>
        <Track id="1" name="Drums" color="#FF5733">
            <MidiNotes>
                <Note start="0.0" duration="0.25" pitch="60" velocity="100"/>
            </MidiNotes>
        </Track>
    </Tracks>
    
    <Playlist>
        <Clip id="1" pattern="1" start="0.0" duration="4.0" track="0"/>
    </Playlist>
    
    <Mixer>
        <Channel id="1" name="Channel 1" gain="0.0" pan="0.0">
            <InsertEffect>
                <Plugin path="..." state="..."/>
            </InsertEffect>
        </Channel>
    </Mixer>
    
    <Automation>
        <Clip parameter="mixer.channel1.gain">
            <Point time="0.0" value="0.5" curve="linear"/>
            <Point time="4.0" value="1.0" curve="exponential"/>
        </Automation>
    </Automation>
</NomadProject>
```

### Memory Management

- **Smart Pointers:** Use `std::unique_ptr` for ownership, `std::shared_ptr` sparingly
- **Object Pools:** Reuse objects in audio thread to avoid allocation
- **Lock-free Structures:** Use `juce::AbstractFifo` for thread-safe communication
- **Copy-on-Write:** For data shared between threads

## Error Handling

### Audio Thread Error Handling

```cpp
// Never throw exceptions in audio thread
void AudioEngine::audioDeviceIOCallback(...)
{
    try {
        // Process audio
    } catch (...) {
        // Log error to lock-free queue
        errorQueue.push(ErrorInfo{...});
        // Output silence
        for (int i = 0; i < numOutputChannels; ++i)
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
}
```

### Plugin Error Handling

- Sandbox plugin processing to prevent crashes
- Timeout mechanism for unresponsive plugins
- Fallback to bypass mode on plugin failure
- User notification of plugin issues

### File I/O Error Handling

- Validate file format before parsing
- Graceful degradation for missing plugins
- Auto-save and crash recovery
- Backup previous project version before saving

## Testing Strategy

### Unit Tests

- Test individual components in isolation
- Mock dependencies (e.g., audio device, file system)
- Test edge cases (empty patterns, extreme parameter values)
- Use JUCE's `UnitTest` framework

```cpp
class PatternTests : public juce::UnitTest
{
public:
    PatternTests() : juce::UnitTest("Pattern Tests") {}
    
    void runTest() override
    {
        beginTest("Add and retrieve notes");
        Pattern pattern;
        Pattern::Note note{0, 0, 60, 1.0f, 1};
        pattern.addNote(note);
        expect(pattern.getNotesInRange(0, 1).size() == 1);
    }
};
```

### Integration Tests

- Test component interactions (e.g., sequencer → audio engine)
- Test audio processing pipeline end-to-end
- Test plugin loading and processing
- Test project save/load roundtrip

### Performance Tests

- Measure audio processing latency
- Test CPU usage with many tracks/plugins
- Test memory usage and leak detection
- Stress test with large projects

### Manual Testing

- Test UI responsiveness and visual feedback
- Test with real audio hardware and various buffer sizes
- Test with popular third-party plugins
- User acceptance testing for workflow

## UI/UX Design

### Component Hierarchy

```
MainWindow
├── MenuBar
├── Toolbar (transport controls, tempo, time signature)
├── MainContentArea
│   ├── SequencerView
│   ├── PianoRollView
│   ├── PlaylistView
│   └── MixerView
└── StatusBar
```

### Theme System

```cpp
class ThemeManager
{
public:
    struct Theme {
        juce::Colour background;
        juce::Colour foreground;
        juce::Colour accent;
        juce::Colour highlight;
        // ... more colors
    };
    
    void setTheme(const Theme& theme);
    Theme getCurrentTheme() const;
    void saveTheme(const juce::String& name);
    std::vector<juce::String> getAvailableThemes() const;
    
private:
    Theme currentTheme;
    std::map<juce::String, Theme> themes;
};
```

### Layout Management

- Use JUCE's `FlexBox` and `Grid` for responsive layouts
- Support docking and undocking of panels
- Save and restore layout preferences
- Support multiple monitor setups

### Graphics Optimization

- Use OpenGL for waveform rendering and complex visualizations
- Implement dirty rectangle optimization for repainting
- Cache rendered graphics where appropriate
- Use hardware acceleration for effects

## Build System and Dependencies

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.15)
project(NOMAD VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# JUCE
add_subdirectory(JUCE)

# Main application
juce_add_gui_app(NOMAD
    PRODUCT_NAME "NOMAD"
    COMPANY_NAME "YourCompany"
    BUNDLE_ID "com.yourcompany.nomad"
)

target_sources(NOMAD PRIVATE
    Source/Main.cpp
    Source/AudioEngine.cpp
    Source/Pattern.cpp
    # ... more sources
)

target_link_libraries(NOMAD PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(NOMAD PRIVATE JUCE_ASIO=1)
endif()
```

### Dependencies

- **JUCE 7.x:** Core framework (included as submodule)
- **VST3 SDK:** For VST3 plugin hosting (optional, JUCE can download)
- **ASIO SDK:** For ASIO support on Windows (optional)

### Build Instructions

**Windows:**
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

**Linux:**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Cross-Platform Considerations

### Platform Abstraction

- Use JUCE's cross-platform APIs wherever possible
- Isolate platform-specific code in separate files
- Use preprocessor directives for platform-specific features

```cpp
#if JUCE_WINDOWS
    // Windows-specific code
#elif JUCE_LINUX
    // Linux-specific code
#endif
```

### Audio Driver Support

- **Windows:** ASIO (low latency), WASAPI (modern), DirectSound (fallback)
- **Linux:** JACK (professional), ALSA (standard)
- Provide driver selection UI with latency information

### File Paths

- Use JUCE's `File` class for cross-platform path handling
- Store user data in platform-appropriate locations
  - Windows: `%APPDATA%/NOMAD`
  - Linux: `~/.config/NOMAD`

### Plugin Paths

- **Windows:** `C:/Program Files/VSTPlugins`, `C:/Program Files/Common Files/VST3`
- **Linux:** `~/.vst`, `/usr/lib/vst`, `~/.vst3`, `/usr/lib/vst3`

## Performance Optimization

### Audio Thread Optimization

- Pre-allocate all buffers
- Use SIMD operations (via JUCE's `FloatVectorOperations`)
- Minimize branching in hot paths
- Profile with tools like Intel VTune or perf

### Memory Optimization

- Use object pools for frequently allocated objects
- Implement custom allocators for audio thread
- Monitor memory usage and implement limits

### Graphics Optimization

- Use dirty rectangle repainting
- Implement level-of-detail rendering for zoomed-out views
- Cache complex graphics operations
- Use OpenGL for heavy rendering tasks

## Security Considerations

- Validate all file inputs to prevent malformed data crashes
- Sandbox plugin processing to prevent malicious plugins
- Implement plugin whitelist/blacklist functionality
- Secure handling of user data and preferences

## Future Extensibility

### Plugin Architecture

Design allows for future internal plugins:
- Built-in synthesizers
- Built-in effects (EQ, compressor, reverb)
- Analysis tools (spectrum analyzer, oscilloscope)

### Scripting Support

Consider adding scripting for:
- Custom MIDI processing
- Automation generation
- Batch operations

### Collaboration Features

Architecture supports future addition of:
- Cloud project storage
- Real-time collaboration
- Version control integration

## Linux Porting Roadmap

1. **Phase 1:** Ensure all code uses JUCE cross-platform APIs
2. **Phase 2:** Set up Linux build environment and CI
3. **Phase 3:** Test with ALSA and JACK audio systems
4. **Phase 4:** Test with Linux VST plugins
5. **Phase 5:** Package for major distributions (deb, rpm, AppImage)
6. **Phase 6:** Optimize for Linux-specific workflows

## Conclusion

This design provides a solid foundation for building NOMAD as a professional DAW. The modular architecture, real-time safety considerations, and cross-platform design ensure that the application can scale from a prototype to a full-featured production tool. The use of JUCE provides a robust framework for audio processing and UI development, while the careful separation of concerns allows for independent development and testing of each component.
