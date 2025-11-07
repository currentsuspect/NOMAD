# **NOMAD DAW – Optimization Phase II: Technical Specification**

**Author:** Dylan Makori / Nomad Studios  
**Date:** November 2025  
**Version:** 1.0  
**Status:** Planning Phase  
**Target Baseline:** 4 GB RAM, integrated GPU, maintaining 32–34 FPS with smoother pacing

---

## **📋 Executive Summary**

This document outlines the technical roadmap for Phase II optimization of Nomad DAW's rendering and debugging infrastructure. Following the successful implementation of framebuffer caching, this phase focuses on:

1. **Profiler Stability** – Resolve Tracy initialization issues or optimize Remotery
2. **GPU Text Rendering** – Migrate CPU text rendering to GPU using SDF
3. **GLAD Integration** – Complete OpenGL extension setup
4. **Debug Overlay Restoration** – Restore internal runtime diagnostics

**Core Principle:** Performance serves quality, not replaces it. Every optimization must maintain or improve visual fidelity.

---

## **🎯 Goals & Success Criteria**

| Goal | Measurable Outcome | Priority |
|------|-------------------|----------|
| **Profiler Stability** | Tracy working OR Remotery optimized with <1ms overhead | HIGH |
| **GPU Text Rendering** | All text rendered via GPU with crisp SDF at any scale | HIGH |
| **Debug Graphs Working** | Frame time, memory, cache hit graphs display correctly | HIGH |
| **GLAD Complete** | All OpenGL 4.3+ extensions loaded and verified | MEDIUM |
| **Performance Maintained** | FPS remains 32–34 avg, no quality degradation | CRITICAL |

---

## **🧩 Known Issues & Environment Context**

### **Issue 1: Tracy Profiler (AVX Initialization Error)**
- **Symptom:** Tracy fails to initialize with AVX-related crash
- **Current Workaround:** Remotery fallback active
- **Impact:** Suboptimal profiling graphs, no GPU zone tracking
- **Root Cause:** Likely AVX instruction set detection or alignment issue

### **Issue 2: GLAD Incomplete Installation**
- **Symptom:** Some OpenGL extensions may be missing
- **Impact:** Cannot implement advanced GPU text rendering safely
- **Required:** OpenGL 4.3+ with compute shader support verified

### **Issue 3: Debugger Graphs Non-Functional**
- **Symptom:** Runtime overlay graphs fail to render
- **Impact:** No real-time performance monitoring during development
- **Affected:** Frame timing, memory usage, cache hit rate displays

### **Issue 4: CPU Text Rendering Bottleneck**
- **Symptom:** Text rendering happens on CPU via FreeType rasterization
- **Impact:** Performance penalty for dynamic text (labels, values, timecodes)
- **Solution Required:** GPU-based SDF rendering with quality preservation

---

## **📦 TASK BREAKDOWN**

---

## **TASK 1 — Diagnose & Fix Tracy Profiler**

### **Purpose**
Restore Tracy profiler functionality to enable deep CPU+GPU zone profiling with superior visualization compared to Remotery.

### **Steps**

#### **1.1 – Isolate AVX Issue**
1. Create minimal Tracy test program separate from Nomad
2. Test with `/arch:AVX`, `/arch:AVX2`, and `/arch:SSE2` compiler flags
3. Check for alignment issues in Tracy's internal data structures
4. Verify CPU capabilities at runtime using `__cpuid` intrinsics
5. Check Tracy version compatibility (ensure using stable release)

**Code Location:** `NomadUI/External/tracy-source/`  
**Test File:** Create `NomadUI/External/tracy-test/main.cpp`

#### **1.2 – Compiler Flag Analysis**
1. Review CMakeLists.txt for Tracy compilation flags
2. Ensure consistency between Tracy build and Nomad build
3. Check for conflicting SIMD instruction sets
4. Verify data alignment requirements (16/32-byte alignment for AVX)

**Files to Check:**
- `CMakeLists.txt` (root)
- `NomadUI/CMakeLists.txt`
- Tracy's build configuration

#### **1.3 – Runtime Detection Fix**
1. Implement CPU feature detection guard
2. Conditionally enable Tracy only if AVX is supported
3. Add graceful fallback to Remotery if detection fails
4. Log detected CPU capabilities on startup

**Implementation Location:** `NomadUI/Core/NUIDebug.cpp` (or similar)

#### **1.4 – Verification**
1. Test on target hardware (4GB RAM, integrated GPU)
2. Verify Tracy connects and displays zones correctly
3. Confirm GPU zones work (if OpenGL backend enabled)
4. Benchmark profiling overhead (must be <1ms per frame)

### **Dependencies**
- None (foundational task)

### **Complexity**
⭐⭐⭐ **Medium-High** (3/5)  
Requires low-level CPU intrinsics knowledge and Tracy internals understanding.

### **Deliverables**
- ✅ Tracy initializes without crashes on target hardware
- ✅ CPU zones display correctly in Tracy profiler
- ✅ GPU zones functional (if GPU support enabled)
- ✅ Fallback to Remotery if Tracy unavailable
- ✅ Documentation of fix in `NomadDocs/Bug Reports/`

### **Notes & Edge Cases**
- **Fallback Strategy:** If Tracy cannot be stabilized within 2 days of focused work, proceed with Remotery optimization (Task 2)
- **Version Pinning:** Consider pinning to last known stable Tracy version (v0.9.1 or v0.10.0)
- **Cross-Platform:** Ensure fix works on Windows; defer Linux/macOS to later phase

---

## **TASK 2 — Optimize Remotery (Fallback Path)**

### **Purpose**
If Tracy cannot be stabilized, optimize Remotery to provide adequate profiling with improved graph readability and lower overhead.

### **Steps**

#### **2.1 – Reduce Profiling Overhead**
1. Adjust Remotery sample rate (balance detail vs. performance)
2. Disable unnecessary features (network profiling if unused)
3. Optimize string handling in zone names
4. Use scoped macros consistently to avoid manual cleanup

**Config Location:** `NomadUI/External/Remotery/lib/Remotery.h`

#### **2.2 – Improve Graph Visualization**
1. Customize Remotery's HTML viewer with Nomad theme colors
2. Add frame time markers and vertical guides
3. Implement cache hit rate custom metrics
4. Add VRAM usage tracking (via custom OpenGL queries)

**HTML Viewer:** `Remotery/vis/index.html`

#### **2.3 – Integration Improvements**
1. Wrap Remotery in abstraction layer for easy switching
2. Add compile-time flags to toggle Tracy/Remotery
3. Ensure zero overhead when profiling disabled
4. Add runtime enable/disable via debug console

**Abstraction:** Create `NomadUI/Core/NUIProfiler.h` wrapper

#### **2.4 – Custom Metrics**
1. Expose custom Remotery properties for:
   - Cache hit rate percentage
   - VRAM usage (via `glGetIntegerv`)
   - Draw call count per frame
   - Widget invalidation count
2. Display in Remotery's property panel

### **Dependencies**
- Task 1 must fail or be deemed unviable

### **Complexity**
⭐⭐ **Medium** (2/5)  
Remotery is simpler than Tracy but requires HTML/JS customization.

### **Deliverables**
- ✅ Remotery overhead <1ms per frame
- ✅ Themed HTML viewer matching Nomad's UI
- ✅ Custom metrics visible in property panel
- ✅ Profiler abstraction layer for future flexibility

### **Notes & Edge Cases**
- **Remotery Limitations:** No GPU zones, no GPU memory tracking
- **Workaround:** Use RenderDoc for GPU-specific profiling sessions
- **Long-term:** Plan migration to Tracy once stable

---

## **TASK 3 — Complete GLAD Installation & Verification**

### **Purpose**
Ensure all required OpenGL extensions and headers are properly loaded for GPU text rendering and future graphics features.

### **Steps**

#### **3.1 – Audit Current GLAD Setup**
1. Check `NomadUI/External/glad/include/glad/glad.h`
2. Verify generated loader includes OpenGL 4.3+ core profile
3. List all extensions currently loaded
4. Compare against requirements for SDF text rendering

**Required Extensions:**
- `GL_ARB_compute_shader` (optional, for advanced effects)
- `GL_ARB_texture_storage` (for immutable texture storage)
- `GL_ARB_seamless_cube_map` (if 3D graphics planned)
- `GL_ARB_debug_output` (already used for debug callbacks)

#### **3.2 – Regenerate GLAD Loader (if needed)**
1. Go to https://glad.dav1d.de/
2. Select OpenGL 4.3 Core Profile
3. Select extensions:
   - `GL_ARB_compute_shader`
   - `GL_ARB_texture_storage`
   - `GL_ARB_buffer_storage`
   - `GL_ARB_multi_draw_indirect`
4. Generate and download
5. Replace files in `NomadUI/External/glad/`

#### **3.3 – Runtime Verification**
1. Add debug output on startup listing loaded extensions
2. Check for required extensions and log warnings if missing
3. Implement fallback paths for optional extensions
4. Test on target hardware (Intel integrated GPU)

**Implementation:** `NomadUI/Graphics/OpenGL/NUIRendererGL.cpp`

```cpp
// Add to renderer initialization
void NUIRendererGL::LogExtensions() {
    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    
    NOM_LOG_INFO("OpenGL Extensions (%d total):", numExtensions);
    for (int i = 0; i < numExtensions; ++i) {
        const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
        NOM_LOG_INFO("  - %s", ext);
    }
}
```

#### **3.4 – Compile & Link Verification**
1. Ensure `glad.c` is compiled into NomadUI library
2. Verify no duplicate symbol errors
3. Check that `gladLoadGL()` is called after GL context creation
4. Confirm all function pointers are non-null

### **Dependencies**
- None (can run in parallel with Task 1/2)

### **Complexity**
⭐ **Low** (1/5)  
GLAD setup is well-documented and straightforward.

### **Deliverables**
- ✅ OpenGL 4.3+ core profile loaded
- ✅ All required extensions verified and logged
- ✅ No linkage or compilation errors
- ✅ Documentation of supported extensions

### **Notes & Edge Cases**
- **Intel GPU Limitations:** Some compute shader features may be limited
- **Fallback:** If compute shaders unavailable, use fragment shader SDF path
- **Testing:** Test on both integrated and dedicated GPUs if possible

---

## **TASK 4 — Implement GPU Text Rendering (SDF)**

### **Purpose**
Migrate text rendering from CPU-based FreeType rasterization to GPU-based Signed Distance Field (SDF) rendering for superior performance and scalability.

### **Steps**

#### **4.1 – SDF Font Atlas Generation**
1. Research SDF generation libraries:
   - `msdfgen` (multi-channel SDF, highest quality)
   - `sdf-gen` (single-channel, simpler)
   - FreeType + custom SDF generator
2. Generate SDF atlas for all UI fonts at build time
3. Store as PNG/KTX texture with distance values
4. Include metadata: glyph metrics, UV coordinates, advance widths

**Tool:** Use `msdfgen` command-line tool  
**Location:** `NomadAssets/fonts/`  
**Output:** `NomadAssets/fonts/[fontname]-sdf.png` + JSON metadata

#### **4.2 – SDF Shader Implementation**
1. Create fragment shader for SDF text rendering
2. Implement subpixel antialiasing
3. Add outline and shadow effects (optional)
4. Optimize for performance (minimize texture fetches)

**Shader File:** `NomadAssets/shaders/text_sdf.frag`

```glsl
#version 430 core

in vec2 v_TexCoord;
in vec4 v_Color;

out vec4 fragColor;

uniform sampler2D u_SDFAtlas;
uniform float u_PxRange; // Distance field pixel range

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 sdfSample = texture(u_SDFAtlas, v_TexCoord).rgb;
    float sigDist = median(sdfSample.r, sdfSample.g, sdfSample.b);
    
    // Convert to screen-space distance
    float screenPxDistance = u_PxRange * (sigDist - 0.5);
    float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    
    fragColor = vec4(v_Color.rgb, v_Color.a * alpha);
}
```

#### **4.3 – C++ Integration**
1. Create `NUITextRendererSDF` class
2. Load SDF atlas and metadata on startup
3. Generate vertex buffers for text quads
4. Batch text rendering per font (minimize state changes)
5. Implement text layout and line breaking

**Files to Create/Modify:**
- `NomadUI/Graphics/NUITextRendererSDF.h`
- `NomadUI/Graphics/NUITextRendererSDF.cpp`
- Update `NomadUI/Graphics/NUITextRenderer.cpp` to use SDF path

#### **4.4 – Quality Assurance**
1. Test text rendering at multiple scales (50%, 100%, 200%, 400%)
2. Verify Unicode support (UTF-8 decoding)
3. Check text alignment (left, center, right, justified)
4. Test dynamic text updates (labels, timecode, numeric values)
5. Compare quality against original FreeType rendering

**Test Fonts:**
- Inter UI (UI labels)
- JetBrains Mono (code/debug)
- Any custom DAW fonts

#### **4.5 – Performance Validation**
1. Benchmark text rendering vs. old CPU path
2. Measure VRAM usage increase from atlas textures
3. Profile draw call count (should not increase significantly)
4. Test on target hardware (4GB RAM, integrated GPU)

### **Dependencies**
- **Task 3 (GLAD)** – Must complete before shader implementation
- **Rendering Cache** – Already implemented (Phase I)

### **Complexity**
⭐⭐⭐⭐ **High** (4/5)  
Requires shader programming, font metrics understanding, and careful quality tuning.

### **Deliverables**
- ✅ SDF atlas generated for all UI fonts
- ✅ GPU text rendering fully functional
- ✅ Quality matches or exceeds CPU rendering
- ✅ Performance improvement of >2x for dynamic text
- ✅ Unicode support verified
- ✅ Documentation of SDF pipeline

### **Notes & Edge Cases**
- **Fallback Path:** Keep CPU FreeType rendering as fallback if GPU fails
- **Font Hinting:** SDF loses hinting; compensate with higher-resolution generation
- **Small Text:** Test at 8px and 10px sizes; may need multi-resolution atlases
- **Memory:** Monitor VRAM increase; each font atlas ~2–8 MB depending on coverage

---

## **TASK 5 — Restore Internal Debug Graphs**

### **Purpose**
Re-enable runtime debug overlay displaying frame time, memory usage, cache hits, and other performance metrics.

### **Steps**

#### **5.1 – Debug Overlay Architecture**
1. Create `NUIDebugOverlay` widget class
2. Render as top-layer overlay (not part of widget tree)
3. Use immediate-mode rendering (no caching needed)
4. Toggle visibility via keyboard shortcut (e.g., F12)

**Location:** `NomadUI/Widgets/NUIDebugOverlay.h/cpp`

#### **5.2 – Metrics Collection**
1. Implement frame time ring buffer (last 120 frames)
2. Track CPU memory usage via OS APIs
3. Query VRAM usage via `glGetIntegerv(GL_GPU_MEMORY_INFO_*)`
4. Expose cache hit rate from `NUIRenderCache`
5. Count draw calls per frame

**Metrics Class:** `NomadUI/Core/NUIPerformanceMetrics.h/cpp`

```cpp
struct NUIPerformanceMetrics {
    // Frame timing
    float frameTimeMs[120]; // Ring buffer
    int frameIndex;
    float avgFrameTime;
    float minFrameTime;
    float maxFrameTime;
    
    // Memory
    size_t cpuMemoryMB;
    size_t gpuMemoryMB;
    
    // Rendering
    float cacheHitRate;      // 0.0 - 1.0
    int drawCallCount;
    int triangleCount;
    
    void Update();
    void Reset();
};
```

#### **5.3 – Graph Rendering**
1. Implement line graph widget for frame time
2. Use colored zones (green <16ms, yellow 16-33ms, red >33ms)
3. Add horizontal guides (16ms, 33ms, 66ms)
4. Render text labels with current/min/max values
5. Support multiple graphs (stacked or tabbed)

**Graph Types:**
- Frame time (line graph)
- Memory usage (stacked area)
- Cache hit rate (percentage bar)
- Draw calls (histogram)

#### **5.4 – Integration**
1. Hook metrics update into main render loop
2. Render debug overlay after all widgets
3. Ensure zero performance impact when disabled
4. Add command console integration (type `debug show fps`)

**Integration Point:** `NomadUI/Core/NUIApplication.cpp::Render()`

#### **5.5 – Styling**
1. Use semi-transparent dark background
2. Use Nomad theme colors (purple accent for active states)
3. Smooth graph animations (interpolate between frames)
4. Auto-scale Y-axis based on data range

### **Dependencies**
- **Task 4 (GPU Text)** – Recommended but not required (can use CPU text for debug)
- **Profiler (Task 1 or 2)** – Can share some metric collection logic

### **Complexity**
⭐⭐⭐ **Medium** (3/5)  
Requires UI programming and metric collection, but no complex algorithms.

### **Deliverables**
- ✅ Debug overlay toggleable via F12
- ✅ Frame time graph displaying last 120 frames
- ✅ Memory usage display (CPU + GPU)
- ✅ Cache hit rate percentage
- ✅ Draw call count per frame
- ✅ Zero overhead when disabled

### **Notes & Edge Cases**
- **Thread Safety:** Metrics collection may happen on render thread; use lock-free structures
- **Graph Resolution:** Adapt to window DPI and size
- **Export:** Add ability to export metrics to CSV for analysis
- **Hotkeys:** Document all debug overlay shortcuts

---

## **TASK 6 — Performance Tier Auto-Detection**

### **Purpose**
Automatically detect hardware capabilities and select optimal rendering strategy (cached vs. immediate-mode).

### **Steps**

#### **6.1 – Hardware Detection**
1. Query system RAM via OS APIs
2. Query VRAM via OpenGL extensions
3. Detect GPU vendor (Intel, NVIDIA, AMD)
4. Check CPU core count and frequency
5. Measure memory bandwidth (optional benchmark)

**Implementation:** `NomadCore/Hardware/NHardwareInfo.h/cpp`

#### **6.2 – Tier Classification**
1. Define tier thresholds:
   - **Low-End:** ≤4GB RAM, integrated GPU, <2GB VRAM
   - **Mid-Tier:** 8-16GB RAM, dedicated GPU, 2-6GB VRAM
   - **High-End:** ≥16GB RAM, high-end GPU, >6GB VRAM
2. Classify system on startup
3. Log detected tier and reasoning

#### **6.3 – Rendering Strategy Selection**
1. Low-End: Enable full caching, dirty-rect, aggressive batching
2. Mid-Tier: Hybrid caching for static elements only
3. High-End: Immediate-mode rendering (simpler, faster on powerful GPUs)

**Config Location:** `NomadUI/Config/NUIRenderConfig.h`

```cpp
enum class RenderTier {
    LowEnd,
    MidTier,
    HighEnd
};

struct RenderConfig {
    RenderTier tier;
    bool enableCaching;
    bool enableDirtyRect;
    bool enableBatching;
    int maxCacheMemoryMB;
};
```

#### **6.4 – User Override**
1. Add settings panel for manual tier selection
2. Allow disabling auto-detection
3. Provide presets (Performance, Balanced, Quality)

### **Dependencies**
- None (can run in parallel)

### **Complexity**
⭐⭐ **Medium** (2/5)  
Straightforward system queries, but requires OS-specific code.

### **Deliverables**
- ✅ Hardware detection on startup
- ✅ Automatic tier classification
- ✅ Rendering strategy auto-configured
- ✅ User override in settings

### **Notes & Edge Cases**
- **Laptop Mode:** Detect battery vs. AC power, throttle accordingly
- **Multi-GPU:** Handle systems with integrated + dedicated GPU
- **Virtualization:** Detect VMs and classify as low-end

---

## **🔁 Execution Order & Timeline**

### **Phase II.A – Foundation (Weeks 1-2)**
1. ✅ Task 3 – Complete GLAD installation *(1-2 days)*
2. ✅ Task 1 – Fix Tracy profiler *(3-4 days)*
   - If fails → Task 2 – Optimize Remotery *(2-3 days)*

### **Phase II.B – Core Rendering (Weeks 2-4)**
3. ✅ Task 4 – Implement GPU Text Rendering *(5-7 days)*
   - Sub-task 4.1 – SDF generation *(1 day)*
   - Sub-task 4.2 – Shader implementation *(2 days)*
   - Sub-task 4.3 – C++ integration *(2-3 days)*
   - Sub-task 4.4 – Quality assurance *(1-2 days)*

### **Phase II.C – Diagnostics (Week 4-5)**
4. ✅ Task 5 – Restore debug graphs *(3-4 days)*
5. ✅ Task 6 – Performance tier detection *(2-3 days)*

### **Phase II.D – Validation (Week 5-6)**
6. ✅ Integration testing across all tasks
7. ✅ Performance validation on target hardware
8. ✅ Documentation updates

**Total Estimated Duration:** 5-6 weeks

---

## **🧩 Dependency Graph**

```
Task 3 (GLAD)
    ↓
Task 4 (GPU Text) ──→ Task 5 (Debug Graphs)
    ↑                      ↑
Task 1 (Tracy) ──→ Task 2 (Remotery) ──┘
                            
Task 6 (Tier Detection) ─→ [Independent, can run anytime]
```

**Critical Path:** Task 3 → Task 4 → Task 5

---

## **💬 Implementation Remarks & Warnings**

### **⚠️ Quality Must Not Regress**
- Every change must be visually compared against baseline
- Use side-by-side screenshots for verification
- If quality degrades, pause and investigate root cause

### **⚠️ Performance Baseline**
- Continuously measure FPS during implementation
- If FPS drops below 30 average, roll back and investigate
- Use profiler to identify bottlenecks before optimizing

### **⚠️ Thread Safety**
- Metrics collection may happen on multiple threads
- Use lock-free data structures or careful mutex usage
- Avoid blocking render thread

### **⚠️ Fallback Paths**
- Every GPU feature must have CPU fallback
- Gracefully handle extension unavailability
- Test fallback paths on older hardware

### **⚠️ Memory Constraints**
- Target hardware has 4GB total RAM
- Monitor VRAM usage carefully (< 512MB for UI)
- Implement LRU cache eviction if needed

### **⚠️ Cross-Platform Considerations**
- Keep Windows as primary target for Phase II
- Avoid platform-specific code in core rendering
- Plan Linux/macOS support for Phase III

---

## **🧠 Optimization Considerations**

### **Balancing Quality & Performance**

| Technique | Quality Impact | Performance Gain | Decision |
|-----------|----------------|------------------|----------|
| **SDF Text** | ✅ Better (scalable) | ⭐⭐⭐⭐ High | IMPLEMENT |
| **Texture Atlasing** | ⚪ Neutral | ⭐⭐⭐ Medium | Phase III |
| **Dirty-Rect 2.0** | ⚪ Neutral | ⭐⭐⭐⭐ High | Phase III |
| **Draw Call Batching** | ⚪ Neutral | ⭐⭐⭐ Medium | Phase III |
| **Lower Texture Resolution** | ❌ Worse | ⭐⭐ Low | REJECT |
| **Disable Antialiasing** | ❌ Worse | ⭐⭐ Low | REJECT |

### **Memory vs. Speed Trade-offs**
- **SDF Atlases:** +8MB VRAM, saves 5-10ms CPU time per frame → **Worth it**
- **Frame Caching:** +50-100MB VRAM, saves 10-15ms GPU time → **Worth it** (already implemented)
- **Profiler Overhead:** +2MB RAM, +0.5ms per frame → **Worth it** (critical for development)

### **Future-Proofing**
- Design abstractions for easy Vulkan/DX12 backend addition
- Keep rendering code modular and testable
- Document all magic numbers and thresholds
- Plan for 4K and HiDPI displays

---

## **📦 Final Deliverables**

### **Code Deliverables**
- ✅ Stable profiler (Tracy or optimized Remotery)
- ✅ Complete GLAD setup with all extensions
- ✅ GPU SDF text rendering system
- ✅ Functional debug overlay with graphs
- ✅ Hardware tier detection system

### **Documentation Deliverables**
- ✅ Updated architecture diagrams
- ✅ SDF pipeline documentation
- ✅ Profiler usage guide
- ✅ Performance tuning guide
- ✅ Debug overlay reference

### **Testing Deliverables**
- ✅ Unit tests for metrics collection
- ✅ Visual regression tests for text rendering
- ✅ Performance benchmarks on target hardware
- ✅ Integration test suite

### **Asset Deliverables**
- ✅ SDF font atlases for all UI fonts
- ✅ Updated shaders (text_sdf.frag, text_sdf.vert)
- ✅ Debug overlay themes

---

## **🔧 Risk Mitigation**

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Tracy won't stabilize** | MEDIUM | LOW | Use Task 2 (Remotery) as fallback |
| **SDF quality insufficient** | LOW | HIGH | Increase atlas resolution, use MSDF |
| **VRAM exceeds budget** | MEDIUM | MEDIUM | Implement LRU eviction, lower atlas res |
| **Performance regresses** | LOW | HIGH | Continuous profiling, rollback capability |
| **Integration GPU unavailable** | LOW | MEDIUM | CPU fallback paths for all GPU features |

---

## **✳️ Developer's Closing Note**

> **"Performance must serve quality, never replace it."**

Every optimization in Phase II is designed to enhance the developer and user experience without compromising Nomad's visual excellence. The rendering system already demonstrates production-grade performance; this phase elevates diagnostics, scalability, and future-readiness.

**Focus areas:**
- Crisp, scalable text at all zoom levels
- Real-time performance insights during development
- Stable profiling infrastructure for future optimization
- Hardware-aware rendering strategies

**Non-negotiables:**
- Visual quality must match or exceed current state
- FPS must remain ≥30 average on target hardware
- All GPU features must have CPU fallbacks
- Code must remain maintainable and documented

---

## **📊 Success Metrics**

At the end of Phase II, the following must be true:

| Metric | Current | Target | Measurement |
|--------|---------|--------|-------------|
| **Avg FPS** | 32-34 | 32-35 | Maintain or improve |
| **Frame Time Variance** | ±8ms | ±4ms | Smoother pacing |
| **Text Render Cost** | 3-5ms | <1ms | GPU profiler |
| **VRAM Usage** | 150MB | <512MB | GL queries |
| **Profiler Overhead** | N/A | <1ms | Frame time delta |
| **Cache Hit Rate** | 85% | >90% | Runtime metrics |

---

## **🎯 Next Steps After Phase II**

1. **Phase III: Draw Call Batching** – Group by texture/shader
2. **Phase IV: Dirty-Rect 2.0** – Sub-millisecond invalidation
3. **Phase V: Texture Atlasing** – Reduce texture binds
4. **Phase VI: Multi-threaded Rendering** – Command buffer recording
5. **Phase VII: Vulkan Backend** – Ultimate performance

---

**End of Specification**

*For questions or clarifications, see `SUPPORT.md` or contact Dylan Makori / Nomad Studios.*
