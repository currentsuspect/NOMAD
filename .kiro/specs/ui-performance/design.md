# UI Performance Optimization Design Document

## Overview

This design addresses critical UI performance bottlenecks in NOMAD DAW that cause ghosting, frame drops, and laggy interactions during window dragging and control manipulation. The solution implements GPU-accelerated rendering, dirty region tracking, cached rendering, optimized event handling, and proper frame timing to achieve smooth 60 FPS performance comparable to FL Studio.

## Architecture

### Current Issues

1. **Inconsistent GPU Acceleration**: Only MixerComponent and PlaylistComponent use OpenGL. SequencerView, MainComponent, and other UI elements are CPU-rendered.

2. **Full Repaints on Every Change**: When a window moves, the entire parent canvas redraws everything, causing ghosting trails and frame drops.

3. **Blocking Event Loop**: Drag events are processed synchronously in the main UI thread, stalling the render queue.

4. **Expensive Visual Effects**: Drop shadows, rounded corners, and transparency are recalculated every frame during drag operations.

5. **No Frame Timing**: Render loop isn't synced to display refresh, causing jittery movement even at high FPS.

### Proposed Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Main Render Loop                       │
│              (VSync @ 60Hz, 16.67ms/frame)              │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
┌───────▼────────┐      ┌────────▼────────┐
│  Event Queue   │      │  Render Queue   │
│  (Async)       │      │  (GPU-Accel)    │
└───────┬────────┘      └────────┬────────┘
        │                        │
        │                        │
┌───────▼────────────────────────▼────────┐
│         Component Hierarchy              │
│  ┌────────────────────────────────┐     │
│  │  MainComponent (OpenGL)        │     │
│  │  ├─ PlaylistComponent (OpenGL) │     │
│  │  ├─ MixerComponent (OpenGL)    │     │
│  │  ├─ SequencerView (OpenGL)     │     │
│  │  └─ Other Components (OpenGL)  │     │
│  └────────────────────────────────┘     │
└──────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│      Cached Texture Manager             │
│  - Background textures                  │
│  - Static window content                │
│  - Pre-rendered shadows/effects         │
└─────────────────────────────────────────┘
```

## Components and Interfaces

### 1. OpenGL Context Management

**Add OpenGL to All Major Components:**

```cpp
// SequencerView.h
class SequencerView : public juce::Component, ...
{
private:
    juce::OpenGLContext openGLContext;
};

// SequencerView.cpp
SequencerView::SequencerView(...)
{
    openGLContext.attachTo(*this);
    openGLContext.setSwapInterval(1); // Enable VSync
}

SequencerView::~SequencerView()
{
    openGLContext.detach();
}
```

**Apply to:**
- SequencerView
- MainComponent (for background rendering)
- TransportComponent
- FileBrowserComponent
- Any other interactive components

### 2. Dirty Region Tracking

**Implement Component-Level Invalidation:**

```cpp
class OptimizedComponent : public juce::Component
{
public:
    void repaintRegion(juce::Rectangle<int> region)
    {
        // Only repaint the specific region
        repaint(region);
    }
    
    void setPosition(int x, int y) override
    {
        auto oldBounds = getBounds();
        Component::setPosition(x, y);
        auto newBounds = getBounds();
        
        // Invalidate only old and new positions
        if (auto* parent = getParentComponent())
        {
            parent->repaint(oldBounds);
            parent->repaint(newBounds);
        }
    }
    
private:
    juce::Rectangle<int> dirtyRegion;
    bool hasDirtyRegion = false;
};
```

### 3. Cached Rendering System

**Texture Cache Manager:**

```cpp
class TextureCacheManager
{
public:
    struct CachedTexture
    {
        juce::OpenGLTexture texture;
        juce::int64 lastUsed;
        bool isDirty;
    };
    
    juce::OpenGLTexture* getCachedTexture(const juce::String& key)
    {
        if (cache.contains(key))
        {
            auto& cached = cache[key];
            cached.lastUsed = juce::Time::currentTimeMillis();
            return cached.isDirty ? nullptr : &cached.texture;
        }
        return nullptr;
    }
    
    void cacheTexture(const juce::String& key, juce::Image& image)
    {
        CachedTexture cached;
        cached.texture.loadImage(image);
        cached.lastUsed = juce::Time::currentTimeMillis();
        cached.isDirty = false;
        cache[key] = cached;
    }
    
    void invalidate(const juce::String& key)
    {
        if (cache.contains(key))
            cache[key].isDirty = true;
    }
    
    void evictOldest(int maxCacheSize)
    {
        // Evict least recently used textures
        if (cache.size() > maxCacheSize)
        {
            // Implementation: sort by lastUsed and remove oldest
        }
    }
    
private:
    std::map<juce::String, CachedTexture> cache;
};
```

**Usage in Components:**

```cpp
void MainComponent::paint(juce::Graphics& g)
{
    // Try to use cached background
    auto* cachedBg = textureCache.getCachedTexture("main_background");
    if (cachedBg != nullptr)
    {
        // Draw cached texture (fast)
        g.drawImage(cachedBg->getImage(), getLocalBounds().toFloat());
    }
    else
    {
        // Render background and cache it
        juce::Image bgImage(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics bgGraphics(bgImage);
        
        // Draw background...
        bgGraphics.fillAll(juce::Colour(0xff0d0e0f));
        // ... other background elements
        
        textureCache.cacheTexture("main_background", bgImage);
        g.drawImage(bgImage, getLocalBounds().toFloat());
    }
}
```

### 4. Optimized Window Dragging

**Lightweight Drag Mode:**

```cpp
class FloatingWindow : public juce::Component
{
public:
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (titleBarArea.contains(event.getPosition()))
        {
            isDragging = true;
            enterLightweightMode();
            dragger.startDraggingComponent(this, event);
        }
    }
    
    void mouseUp(const juce::MouseEvent& event) override
    {
        if (isDragging)
        {
            isDragging = false;
            exitLightweightMode();
        }
    }
    
private:
    void enterLightweightMode()
    {
        // Disable expensive effects during drag
        shadowEnabled = false;
        blurEnabled = false;
        
        // Switch to solid background
        originalBackground = backgroundColor;
        backgroundColor = backgroundColor.withAlpha(1.0f);
        
        repaint();
    }
    
    void exitLightweightMode()
    {
        // Re-enable effects after drag
        shadowEnabled = true;
        blurEnabled = true;
        backgroundColor = originalBackground;
        
        // Smooth transition back
        juce::Timer::callAfterDelay(100, [this]() {
            repaint();
        });
    }
    
    bool isDragging = false;
    bool shadowEnabled = true;
    bool blurEnabled = true;
    juce::Colour originalBackground;
};
```

### 5. Async Event Processing

**Event Queue with Batching:**

```cpp
class AsyncEventProcessor
{
public:
    void queueDragEvent(juce::Component* component, juce::Point<int> newPosition)
    {
        juce::ScopedLock lock(queueMutex);
        
        // Coalesce multiple drag events for same component
        for (auto& event : eventQueue)
        {
            if (event.component == component && event.type == EventType::Drag)
            {
                event.position = newPosition;
                return;
            }
        }
        
        eventQueue.push_back({EventType::Drag, component, newPosition});
    }
    
    void processEvents()
    {
        std::vector<Event> eventsToProcess;
        
        {
            juce::ScopedLock lock(queueMutex);
            eventsToProcess.swap(eventQueue);
        }
        
        // Process all events in batch
        for (const auto& event : eventsToProcess)
        {
            if (event.type == EventType::Drag)
            {
                event.component->setTopLeftPosition(event.position);
            }
        }
    }
    
private:
    enum class EventType { Drag, Resize, Repaint };
    
    struct Event
    {
        EventType type;
        juce::Component* component;
        juce::Point<int> position;
    };
    
    std::vector<Event> eventQueue;
    juce::CriticalSection queueMutex;
};
```

### 6. Frame Timing and VSync

**Render Loop with Timing:**

```cpp
class PerformanceMonitor
{
public:
    void frameStart()
    {
        frameStartTime = juce::Time::getMillisecondCounterHiRes();
    }
    
    void frameEnd()
    {
        double frameTime = juce::Time::getMillisecondCounterHiRes() - frameStartTime;
        
        // Track frame times
        frameTimes.push_back(frameTime);
        if (frameTimes.size() > 60)
            frameTimes.erase(frameTimes.begin());
        
        // Calculate average
        double avgFrameTime = 0.0;
        for (double time : frameTimes)
            avgFrameTime += time;
        avgFrameTime /= frameTimes.size();
        
        // Warn if frame took too long
        if (frameTime > 16.67)
        {
            DBG("SLOW FRAME: " << frameTime << "ms (target: 16.67ms)");
        }
        
        // Update FPS counter
        currentFPS = 1000.0 / avgFrameTime;
    }
    
    double getCurrentFPS() const { return currentFPS; }
    double getAverageFrameTime() const
    {
        if (frameTimes.empty()) return 0.0;
        double sum = 0.0;
        for (double time : frameTimes) sum += time;
        return sum / frameTimes.size();
    }
    
private:
    double frameStartTime = 0.0;
    std::vector<double> frameTimes;
    double currentFPS = 60.0;
};
```

**Integration in Components:**

```cpp
void MainComponent::timerCallback()
{
    performanceMonitor.frameStart();
    
    // Update playhead position every frame for smooth movement
    double currentPosition = audioEngine.getTransportController().getPosition();
    playlistWindow.setPlayheadPosition(currentPosition);
    
    // Process async events
    eventProcessor.processEvents();
    
    performanceMonitor.frameEnd();
    
    // Display FPS in debug mode
    #if JUCE_DEBUG
    if (showPerformanceStats)
    {
        DBG("FPS: " << performanceMonitor.getCurrentFPS() 
            << " | Frame Time: " << performanceMonitor.getAverageFrameTime() << "ms");
    }
    #endif
}
```

### 7. Pre-rendered Effects

**Shadow and Effect Cache:**

```cpp
class EffectCache
{
public:
    juce::Image getDropShadow(int width, int height, float radius, juce::Colour color)
    {
        juce::String key = "shadow_" + juce::String(width) + "_" + juce::String(height) 
                         + "_" + juce::String(radius);
        
        if (shadowCache.contains(key))
            return shadowCache[key];
        
        // Render shadow once
        juce::Image shadow(juce::Image::ARGB, width + 20, height + 20, true);
        juce::Graphics g(shadow);
        
        juce::DropShadow dropShadow(color, (int)radius, juce::Point<int>(2, 3));
        juce::Rectangle<int> bounds(10, 10, width, height);
        dropShadow.drawForRectangle(g, bounds);
        
        shadowCache[key] = shadow;
        return shadow;
    }
    
private:
    std::map<juce::String, juce::Image> shadowCache;
};
```

**Usage:**

```cpp
void PlaylistComponent::paint(juce::Graphics& g)
{
    if (!isDocked && !isDragging)
    {
        // Use pre-rendered shadow
        auto shadow = effectCache.getDropShadow(getWidth(), getHeight(), 8.0f, 
                                                juce::Colours::black.withAlpha(0.6f));
        g.drawImageAt(shadow, -10, -10);
    }
    else if (isDragging)
    {
        // No shadow during drag for performance
    }
    
    // Rest of painting...
}
```

### 8. Optimized Repaint Strategy

**Smart Repaint Coalescing:**

```cpp
class SmartRepaintManager
{
public:
    void requestRepaint(juce::Component* component, juce::Rectangle<int> region)
    {
        juce::ScopedLock lock(repaintMutex);
        
        // Coalesce overlapping regions
        if (repaintRequests.contains(component))
        {
            repaintRequests[component] = repaintRequests[component].getUnion(region);
        }
        else
        {
            repaintRequests[component] = region;
        }
    }
    
    void processRepaints()
    {
        juce::ScopedLock lock(repaintMutex);
        
        for (auto& [component, region] : repaintRequests)
        {
            if (region.isEmpty())
                component->repaint();
            else
                component->repaint(region);
        }
        
        repaintRequests.clear();
    }
    
private:
    std::map<juce::Component*, juce::Rectangle<int>> repaintRequests;
    juce::CriticalSection repaintMutex;
};
```

## Data Models

### Performance Metrics

```cpp
struct PerformanceMetrics
{
    double currentFPS;
    double averageFrameTime;
    double peakFrameTime;
    int droppedFrames;
    int cachedTextures;
    size_t cacheMemoryUsage;
    
    juce::String toString() const
    {
        juce::String result;
        result << "FPS: " << juce::String(currentFPS, 1) << "\n";
        result << "Avg Frame Time: " << juce::String(averageFrameTime, 2) << "ms\n";
        result << "Peak Frame Time: " << juce::String(peakFrameTime, 2) << "ms\n";
        result << "Dropped Frames: " << droppedFrames << "\n";
        result << "Cached Textures: " << cachedTextures << "\n";
        result << "Cache Memory: " << (cacheMemoryUsage / 1024 / 1024) << "MB\n";
        return result;
    }
};
```

## Error Handling

1. **OpenGL Fallback**: If OpenGL context fails to attach, log warning and continue with software rendering
2. **Cache Overflow**: Implement LRU eviction when cache exceeds memory limit (default: 100MB)
3. **Frame Timing**: If frame consistently exceeds 16.67ms, log warning and suggest disabling effects
4. **VSync Failure**: If VSync unavailable, use manual frame limiting at 60 FPS

## Testing Strategy

### Performance Benchmarks

1. **Drag Test**: Measure FPS while dragging window continuously for 10 seconds
   - Target: 60 FPS sustained
   - Acceptable: 55+ FPS average

2. **Multi-Window Test**: Drag multiple overlapping windows
   - Target: 60 FPS with 3 windows
   - Acceptable: 50+ FPS with 3 windows

3. **Effect Overhead**: Compare FPS with/without shadows and blur
   - Target: <5% FPS difference

4. **Memory Usage**: Monitor cache memory over 5 minutes of use
   - Target: <100MB cache memory
   - Acceptable: <200MB cache memory

### Manual Testing

1. Drag windows rapidly - no ghosting trails
2. Adjust faders/knobs - smooth 60 FPS animation
3. Scroll playlist - no stuttering
4. Resize windows - smooth resize without tearing
5. Multiple simultaneous animations - no frame drops

### Profiling Tools

- JUCE's built-in performance profiler
- Windows: PIX, GPUView
- Visual Studio Performance Profiler
- Custom frame timing overlay (debug mode)

## Implementation Notes

1. **Incremental Rollout**: Enable OpenGL component-by-component, test each
2. **Debug Mode**: Add keyboard shortcut (Ctrl+Shift+P) to toggle performance overlay
3. **Configuration**: Add settings to disable GPU acceleration if issues occur
4. **Backward Compatibility**: Ensure software rendering fallback works on older systems
5. **Memory Management**: Implement cache size limits and LRU eviction
6. **Thread Safety**: All cache access must be mutex-protected

## Performance Targets

| Metric | Target | Acceptable | Current |
|--------|--------|------------|---------|
| Window Drag FPS | 60 | 55+ | ~20-30 |
| Frame Time | <16.67ms | <18ms | ~30-50ms |
| Dropped Frames | 0 | <5/min | Many |
| Cache Memory | <100MB | <200MB | 0 (no cache) |
| Startup Time | <2s | <3s | ~1s |

## Migration Path

1. **Phase 1**: Add OpenGL to SequencerView and MainComponent
2. **Phase 2**: Implement texture cache manager
3. **Phase 3**: Add lightweight drag mode to all floating windows
4. **Phase 4**: Implement async event processing
5. **Phase 5**: Add performance monitoring and profiling
6. **Phase 6**: Optimize repaint strategy with dirty regions
7. **Phase 7**: Pre-render and cache expensive effects
