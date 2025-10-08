# Audio Clip Drag & Drop Implementation

## Overview
Implemented drag-and-drop functionality to add audio samples from the file browser to the playlist, with visual waveform display.

## Features Implemented

### 1. AudioClip Data Model (`Source/Models/AudioClip.h`)
Created a new data structure to represent audio clips in the playlist:

**Properties:**
- `audioFile`: Reference to the source audio file
- `trackIndex`: Which track the clip is on (0-19)
- `startTime`: Position in the timeline (in beats)
- `duration`: Length of the audio in seconds
- `color`: Visual color (purple glow by default)
- `name`: Display name (filename without extension)
- `audioData`: Loaded audio samples for playback and waveform display
- `sampleRate`: Original sample rate of the audio

**Key Methods:**
- `loadAudioData()`: Loads the entire audio file into memory
- `getEndTime()`: Calculates the end position
- `getBounds()`: Calculates screen position for rendering

### 2. Drag & Drop Support
PlaylistComponent now implements `juce::FileDragAndDropTarget`:

**Supported Formats:**
- WAV (.wav)
- MP3 (.mp3)
- FLAC (.flac)
- OGG (.ogg)
- AIFF (.aiff, .aif)

**Drag & Drop Behavior:**
1. **Drag Enter**: Purple overlay appears to indicate drop zone
2. **Drop**: Audio file is loaded and placed at the drop position
3. **Position Calculation**: 
   - Track determined by Y position
   - Time determined by X position (snapped to grid)

### 3. Visual Representation

**Clip Rendering:**
- Semi-transparent purple background matching the theme
- Bright purple border (2px)
- Clip name displayed on the left
- Waveform visualization when clip is wide enough (>40px)

**Waveform Display:**
- Real-time waveform drawn from loaded audio data
- White semi-transparent color
- Downsampled for performance
- Shows amplitude variations

### 4. Helper Methods

**Position Conversion:**
```cpp
int getTrackAtPosition(int y)      // Converts Y coordinate to track index
double getTimeAtPosition(int x)     // Converts X coordinate to timeline position
```

**Rendering:**
```cpp
void drawAudioClip(Graphics& g, const AudioClip& clip)  // Draws clip with waveform
```

## Technical Details

### Audio Loading
- Uses JUCE's `AudioFormatManager` to support multiple formats
- Registers basic formats (WAV, AIFF, FLAC, OGG, MP3)
- Loads entire file into memory for instant playback
- Stores original sample rate for accurate playback

### Coordinate System
- **Pixels Per Beat**: 20 pixels = 1 beat (configurable)
- **Track Height**: 48 pixels per track
- **Scrolling**: Accounts for both horizontal and vertical scroll offsets
- **Grid Alignment**: Clips snap to the timeline grid

### Memory Management
- Audio data stored in `AudioSampleBuffer`
- Uses `std::vector` for clip storage
- Move semantics for efficient clip management

## Usage

### Dragging Files
1. Open the file browser (left sidebar)
2. Navigate to a folder with audio files
3. Drag an audio file (.wav, .mp3, etc.) to the playlist grid
4. Drop it on the desired track and time position
5. The clip appears with its waveform

### Visual Feedback
- **Idle**: Clips show with semi-transparent purple background
- **Drag Over**: Entire grid area highlights with purple overlay
- **Waveform**: Automatically displays when clip is wide enough

## Next Steps

### Playback Integration (To Be Implemented)
1. Connect clips to AudioEngine for playback
2. Implement playhead tracking
3. Add scrubbing functionality
4. Support multiple clips playing simultaneously

### Additional Features (Future)
- Clip selection and editing
- Clip trimming/resizing
- Fade in/out
- Volume/pan per clip
- Clip duplication
- Delete functionality
- Undo/redo support

## Files Modified

### New Files:
- `Source/Models/AudioClip.h` - Audio clip data structure

### Modified Files:
- `Source/UI/PlaylistComponent.h` - Added drag-drop interface and clip storage
- `Source/UI/PlaylistComponent.cpp` - Implemented drag-drop handlers and rendering
- `CMakeLists.txt` - Added AudioClip.h to build

## Testing
- Build successful with no errors
- Ready for testing with actual audio files
- Waveform rendering implemented and ready to display

## Known Limitations
- Clips are not yet connected to audio playback
- No clip editing/deletion UI yet
- No playhead/scrubbing yet (next step)
- All audio loaded into memory (may need streaming for large files)
