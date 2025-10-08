# Drag & Drop Debugging Guide

## Testing Steps

### Test 1: External File Drop (from Windows Explorer)
1. Open Windows Explorer
2. Navigate to a folder with a .wav file
3. Drag the .wav file from Windows Explorer to the Playlist window
4. Check the console for log messages

**Expected logs:**
```
isInterestedInFileDrag called with 1 files
Checking file: C:\path\to\file.wav
File is an audio file - accepting drag
fileDragEnter at x=..., y=...
Files dropped at track X, time Y
Processing file: C:\path\to\file.wav
Loading audio: filename (X.XXs, 2 channels)
Successfully loaded XXXXX samples
Successfully loaded audio clip: filename
```

### Test 2: Internal Drag (from File Browser)
JUCE's FileTreeComponent doesn't support drag-out by default. We need to implement a custom solution.

## Current Status

### What's Implemented:
- ✅ PlaylistComponent implements FileDragAndDropTarget
- ✅ isInterestedInFileDrag() checks for audio file extensions
- ✅ filesDropped() creates AudioClip and loads audio data
- ✅ AudioClip.cpp properly loads audio files
- ✅ Extensive debug logging

### What's NOT Working:
- ❌ FileTreeComponent doesn't initiate drags
- ❌ setDragAndDropDescription() doesn't enable external dragging

## Solutions

### Solution 1: Use External Drag (Windows Explorer)
**Pros:** Simple, works immediately
**Cons:** Less convenient than internal browser

### Solution 2: Implement Custom File List with Drag Support
Replace FileTreeComponent with a custom ListBox that supports dragging:

```cpp
class DraggableFileListBox : public juce::ListBox,
                              public juce::ListBoxModel
{
    // Implement mouseDrag to start drag operation
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            juce::StringArray files;
            files.add(selectedFile.getFullPathName());
            container->startDragging(files, this);
        }
    }
};
```

### Solution 3: Add "Add to Playlist" Button
Simple button in file browser that adds selected file to playlist.

## Recommended Next Steps

1. **Test external drag first** - Drag from Windows Explorer to verify the drop target works
2. **If external drag works**, implement Solution 2 or 3
3. **If external drag doesn't work**, check if PlaylistComponent is properly visible and in front

## Troubleshooting

### No log messages at all?
- PlaylistComponent might not be visible
- PlaylistComponent might be behind other components
- FileDragAndDropTarget interface might not be properly registered

### "isInterestedInFileDrag" called but "filesDropped" not called?
- Check if isInterestedInFileDrag returns true
- Check file extension matching

### Files dropped but not loading?
- Check AudioClip::loadAudioData() logs
- Verify file path is correct
- Check if AudioFormatManager is properly initialized
