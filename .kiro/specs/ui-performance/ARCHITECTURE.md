# UI Performance Architecture

## Overview
This document outlines the GPU-accelerated compositing architecture for NOMAD DAW, designed to achieve FL Studio-tier responsiveness with maintainable code.

## Core Systems

### 1. GPUContextManager (Single Shared OpenGL Context)
**Purpose**: Centralized GPU context management for the entire application.

**Benefits**:
- Perfect synchronization (one swap buffer)
- No redundant OpenGL setup/destruction
- Easier VSync and performance tracking
- Eliminates context switching overhead

**Implementation**:
- Single `juce::OpenGLContext` owned by MainComponent
- All child components register with the manager
- Components can be enabled/disabled for rendering without creating new contexts

**Usage**:
```cpp
// In MainComponent constructor
GPUContextManager::getInstance().attachToComponent(this);

// In child components
GPUContextManager::getInstance().registerComponent(this);
GPUContextManager::getInstance().setComponentRenderingActive(this, true);
```

### 2. DragStateManager (Global Lightweight Mode)
**Purpose**: Optimize UI performance during drag operations.

**Benefits**:
- Consistent visual behavior across all components
- Automatic shadow/blur disabling during drags
- FL Studio-style global UI optimization
- No repetitive toggling logic

**Implementation**:
- Singleton pattern with listener system
- Components subscribe to drag state changes
- Automatically reduces visual complexity during drags

**Usage**:
```cpp
// When starting drag
DragStateManager::getInstance().enterLightweightMode();

// When drag ends
DragStateManager::getInstance().exitLightweightMode();

// In components
if (DragStateManager::getInstance().isLightweight()) {
    // Skip expensive rendering
}
```

### 3. RepaintScheduler (Unified Dirty Region System)
**Purpose**: Batch and optimize repaint requests for single-pass rendering.

**Benefits**:
- Unions overlapping dirty regions
- Single repaint per frame per component
- Eliminates redundant redraws
- Stable CPU usage

**Implementation**:
- Collects repaint requests throughout the frame
- Unions overlapping regions
- Flushes all repaints in one batch

**Usage**:
```cpp
// Request repaint
RepaintScheduler::getInstance().requestRepaint(this, dirtyArea);

// In main timer (once per frame)
RepaintScheduler::getInstance().flushRepaints();
```

### 4. PerformanceMonitor (CPU/GPU Split Tracking)
**Purpose**: Identify rendering bottlenecks with detailed metrics.

**Metrics Tracked**:
- Frame time (total)
- CPU render time (paint calls, event handling)
- GPU render time (OpenGL swap buffer, compositing)
- Paint count per frame
- FPS (frames per second)

**Implementation**:
- Frame-based timing with high-resolution counters
- Exponential moving averages for smooth metrics
- Formatted stats output for overlay display

**Usage**:
```cpp
// At frame start
PerformanceMonitor::getInstance().beginFrame();

// After CPU work
PerformanceMonitor::getInstance().endCPUPhase();

// At frame end
PerformanceMonitor::getInstance().endFrame();

// Get stats
auto stats = PerformanceMonitor::getInstance().getStatsString();
```

## Implementation Stages

### Stage 1: Global OpenGL + VSync ✅
**Goal**: Establish 60 FPS baseline with unified GPU context.

**Tasks**:
1. Refactor MainComponent to use GPUContextManager
2. Remove individual OpenGL contexts from child components
3. Update all components to register with GPUContextManager
4. Verify VSync is working correctly

**Expected Gain**: Stable 60 FPS, reduced GPU overhead

### Stage 2: Lightweight Drag Mode
**Goal**: Fix ghost trails and visual artifacts during window dragging.

**Tasks**:
1. Integrate DragStateManager into floating windows
2. Implement drag state listeners in Playlist, Mixer, Sequencer
3. Disable shadows/blur during drags
4. Test drag performance

**Expected Gain**: Smooth dragging without visual artifacts

### Stage 3: Texture Caching + Pre-rendered Shadows
**Goal**: Massive repaint performance improvement.

**Tasks**:
1. Implement texture cache system
2. Pre-render shadows and effects to textures
3. Add lazy invalidation for cache entries
4. Benchmark performance improvement

**Expected Gain**: 50-70% reduction in paint time

### Stage 4: Dirty Region Repaint System
**Goal**: Stable CPU usage with minimal redraws.

**Tasks**:
1. Integrate RepaintScheduler into MainComponent
2. Update all components to use RepaintScheduler
3. Implement region union logic
4. Test with complex UI interactions

**Expected Gain**: Consistent CPU usage, no redundant redraws

### Stage 5: AsyncEventProcessor
**Goal**: Smooth input handling without lag.

**Tasks**:
1. Implement frame-locked event processing
2. Add event timestamping and coalescing
3. Process events once per frame
4. Test input responsiveness

**Expected Gain**: Zero input lag, smooth interactions

### Stage 6: PerformanceMonitor Overlay
**Goal**: Visibility into bottlenecks for optimization.

**Tasks**:
1. Integrate PerformanceMonitor into MainComponent
2. Create overlay UI for stats display
3. Add keyboard shortcut to toggle overlay
4. Benchmark and optimize based on metrics

**Expected Gain**: Data-driven optimization

### Stage 7: SmartRepaintManager Merge + Benchmarking
**Goal**: Final polish and optimization.

**Tasks**:
1. Merge RepaintScheduler with any remaining repaint logic
2. Run comprehensive benchmarks
3. Profile and optimize hotspots
4. Document performance characteristics

**Expected Gain**: Production-ready performance

## Future Enhancements (Optional)

### Frame Interpolation / Animation System
- Smooth transitions between states
- Adds perceived fluidity
- Useful for sliders, window movement

### Render Thread Scheduling
- Dedicated render thread separate from UI thread
- Total decoupling of rendering and interaction
- Advanced optimization

### UI Shader Effects
- GPU shaders for glow/blur effects
- Cached framebuffers for efficiency
- FL Studio-style visual polish

## Performance Targets

| Metric | Target | Current |
|--------|--------|---------|
| FPS | 60 | ~45-55 |
| Frame Time | <16.67ms | ~20-25ms |
| CPU Time | <10ms | ~15-20ms |
| GPU Time | <6ms | ~5-8ms |
| Paint Count | <10/frame | ~15-25/frame |

## Architecture Benefits

1. **Maintainability**: Clear separation of concerns, single responsibility
2. **Performance**: Optimized rendering pipeline, minimal overhead
3. **Scalability**: Easy to add new components and features
4. **Debuggability**: Detailed metrics and monitoring
5. **Consistency**: Unified behavior across all UI components

## Next Steps

1. Implement Stage 1 (Global OpenGL + VSync)
2. Test and verify 60 FPS baseline
3. Proceed to Stage 2 (Lightweight Drag Mode)
4. Continue through stages in order
5. Benchmark and optimize at each stage

## References

- JUCE OpenGL documentation
- FL Studio UI architecture (inspiration)
- GPU compositing best practices
- Real-time rendering optimization techniques
