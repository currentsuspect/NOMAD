# **NOMAD Phase II – Reference Card**

**Quick reference for thresholds, parameters, and critical values used in optimization.**

---

## **🎯 Performance Targets**

| Metric | Current | Target | Critical Threshold |
|--------|---------|--------|-------------------|
| **Average FPS** | 32-34 | 32-35 | ≥30 FPS (minimum) |
| **Frame Time** | ~30-31ms | <33ms | <66ms (never drop below 15 FPS) |
| **Frame Variance** | ±8ms | ±4ms | ±10ms (acceptable) |
| **Text Render Time** | 3-5ms (CPU) | <1ms (GPU) | <2ms (acceptable) |
| **VRAM Usage** | ~150MB | <512MB | <768MB (hard limit) |
| **System RAM** | Variable | N/A | 4GB baseline |
| **Profiler Overhead** | N/A | <1ms | <2ms (acceptable) |
| **Cache Hit Rate** | ~85% | >90% | >80% (minimum) |
| **Draw Calls/Frame** | Variable | <150 | <200 (acceptable) |

---

## **🔧 OpenGL Configuration**

### **Required Version**
- **Minimum:** OpenGL 4.3 Core Profile
- **Recommended:** OpenGL 4.5+
- **Fallback:** OpenGL 3.3 (limited features)

### **Required Extensions**
```cpp
// Critical
GL_ARB_texture_storage         // Immutable texture storage
GL_ARB_debug_output           // Debug callbacks (already used)

// Recommended
GL_ARB_compute_shader         // Compute shader support
GL_ARB_buffer_storage         // Persistent mapped buffers
GL_ARB_multi_draw_indirect    // Efficient batching

// Optional
GL_ARB_seamless_cube_map      // Future 3D graphics
GL_NV_gpu_shader5             // NVIDIA-specific optimizations
```

### **Texture Formats**
- **UI Textures:** `GL_RGBA8` (standard)
- **SDF Atlases:** `GL_RGB8` (multi-channel SDF) or `GL_R8` (single-channel)
- **HDR (future):** `GL_RGBA16F`

---

## **📊 SDF Text Rendering Parameters**

### **Atlas Generation**
| Parameter | Value | Notes |
|-----------|-------|-------|
| **Atlas Size** | 2048×2048 | Per font, adjust based on glyph count |
| **Glyph Size** | 48px | Base size for generation |
| **Pixel Range** | 4.0 | Distance field spread |
| **Format** | MSDF (RGB) | Multi-channel for better quality |
| **Padding** | 2px | Between glyphs |

### **Runtime Rendering**
| Parameter | Value | Notes |
|-----------|-------|-------|
| **Min Font Size** | 8px | Smallest readable size |
| **Max Font Size** | 128px | Largest UI text |
| **Antialiasing** | Subpixel | Crisp text at all scales |
| **Mipmap Levels** | Auto-generate | For scaled rendering |

### **Shader Uniforms**
```glsl
uniform sampler2D u_SDFAtlas;      // SDF texture
uniform float u_PxRange = 4.0;     // Distance field range
uniform vec2 u_AtlasSize = vec2(2048.0, 2048.0);
uniform float u_OutlineWidth = 0.0; // Optional outline
uniform vec4 u_OutlineColor = vec4(0.0); // Optional outline color
```

---

## **📈 Performance Metrics Collection**

### **Frame Time Tracking**
```cpp
constexpr int FRAME_HISTORY = 120;  // 2 seconds at 60fps
constexpr int UPDATE_INTERVAL = 30; // Update avg every 30 frames
```

### **Memory Tracking**
```cpp
// Query intervals
constexpr int CPU_MEMORY_QUERY_FRAMES = 60;  // Once per second
constexpr int GPU_MEMORY_QUERY_FRAMES = 120; // Once per 2 seconds (slower)
```

### **VRAM Query (NVIDIA)**
```cpp
GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX       // Total VRAM
GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX     // Available VRAM
```

### **VRAM Query (AMD)**
```cpp
GL_TEXTURE_FREE_MEMORY_ATI  // Free texture memory
GL_VBO_FREE_MEMORY_ATI      // Free VBO memory
```

---

## **🖥️ Hardware Tier Classification**

### **Tier Thresholds**

| Tier | RAM | VRAM | GPU Type | CPU Cores |
|------|-----|------|----------|-----------|
| **Low-End** | ≤4GB | ≤1GB | Integrated | ≤4 |
| **Mid-Tier** | 8-16GB | 2-6GB | Dedicated | 4-8 |
| **High-End** | ≥16GB | ≥6GB | High-end | ≥8 |

### **Rendering Strategy by Tier**

| Tier | Caching | Dirty Rect | Batching | Mode |
|------|---------|------------|----------|------|
| **Low-End** | ✅ Full | ✅ Yes | ✅ Aggressive | Cached |
| **Mid-Tier** | 🟡 Partial | 🟡 Optional | ✅ Yes | Hybrid |
| **High-End** | ❌ Minimal | ❌ No | 🟡 Optional | Immediate |

### **Detection Code**
```cpp
struct HardwareTier {
    static RenderTier Detect() {
        size_t ram = GetSystemRAM_MB();
        size_t vram = GetGPUMemory_MB();
        int cores = GetCPUCoreCount();
        
        if (ram <= 4096 || vram <= 1024) return RenderTier::LowEnd;
        if (ram >= 16384 && vram >= 6144) return RenderTier::HighEnd;
        return RenderTier::MidTier;
    }
};
```

---

## **🎨 Debug Overlay Configuration**

### **Graph Display Parameters**
```cpp
constexpr int GRAPH_WIDTH = 400;     // Pixels
constexpr int GRAPH_HEIGHT = 120;    // Pixels
constexpr float GRAPH_MIN_TIME = 0.0f;   // Milliseconds
constexpr float GRAPH_MAX_TIME = 50.0f;  // Auto-scale to max value

// Color zones
constexpr float GOOD_THRESHOLD = 16.67f;  // 60 FPS (green)
constexpr float OK_THRESHOLD = 33.33f;    // 30 FPS (yellow)
// Above OK_THRESHOLD = red
```

### **Update Rates**
```cpp
constexpr int OVERLAY_UPDATE_FRAMES = 1;  // Update every frame
constexpr int METRICS_UPDATE_FRAMES = 30; // Update averages every 30 frames
```

### **Hotkeys**
| Key | Action |
|-----|--------|
| **F12** | Toggle debug overlay |
| **Ctrl+F12** | Toggle profiler |
| **Shift+F12** | Export metrics to CSV |

---

## **🧪 Tracy Profiler Configuration**

### **Compiler Flags**
```cmake
# Safe (SSE2 only)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

# Moderate (AVX if supported)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")

# Aggressive (AVX2 if supported)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
```

### **Zone Macros**
```cpp
// Frame boundary
FrameMark;

// Named zone
ZoneScopedN("RenderWidgets");

// Conditional zone (only in debug)
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

// GPU zone (OpenGL)
TracyGpuZone("GPU Draw");
```

### **Performance Budget**
```cpp
constexpr float TRACY_MAX_OVERHEAD_MS = 1.0f;  // Per frame
constexpr int TRACY_MAX_ZONES = 1000;          // Per frame
```

---

## **🎨 Nomad Theme Colors**

### **Primary Colors**
```cpp
constexpr uint32_t COLOR_PRIMARY = 0x785aff;    // Vibrant purple
constexpr uint32_t COLOR_SECONDARY = 0x00d4bc;  // Teal
constexpr uint32_t COLOR_SUCCESS = 0x00d49e;    // Teal green
constexpr uint32_t COLOR_WARNING = 0xffb400;    // Amber
constexpr uint32_t COLOR_ERROR = 0xff4465;      // Vibrant red
```

### **Backgrounds**
```cpp
constexpr uint32_t BG_PRIMARY = 0x19191c;    // Deep slate
constexpr uint32_t BG_SECONDARY = 0x1e1e22;  // Panels
constexpr uint32_t BG_TERTIARY = 0x28282d;   // Transport bar
constexpr uint32_t BG_RAISED = 0x323238;     // Cards, hover
```

### **Text**
```cpp
constexpr uint32_t TEXT_PRIMARY = 0xeeeef2;   // Crisp white
constexpr uint32_t TEXT_SECONDARY = 0xaaaab2; // Labels
constexpr uint32_t TEXT_DISABLED = 0x808088;  // Disabled
```

---

## **⚠️ Critical Thresholds & Limits**

### **Memory Limits**
```cpp
constexpr size_t MAX_CACHE_MEMORY_MB = 256;      // FBO cache limit
constexpr size_t MAX_TEXTURE_MEMORY_MB = 512;    // All textures
constexpr size_t MAX_VERTEX_BUFFER_MB = 64;      // Dynamic VBOs
```

### **Rendering Limits**
```cpp
constexpr int MAX_DRAW_CALLS_PER_FRAME = 200;    // Hard limit
constexpr int TARGET_DRAW_CALLS = 150;           // Target
constexpr int MAX_TEXTURE_BINDS = 100;           // Per frame
constexpr int MAX_WIDGETS_CACHED = 500;          // FBO cache
```

### **Frame Time Thresholds**
```cpp
constexpr float FRAME_TIME_60FPS = 16.67f;   // 60 FPS
constexpr float FRAME_TIME_30FPS = 33.33f;   // 30 FPS
constexpr float FRAME_TIME_15FPS = 66.67f;   // 15 FPS (critical)
constexpr float MAX_FRAME_TIME = 100.0f;     // Emergency throttle
```

---

## **🔍 Debug Logging Levels**

### **Log Categories**
```cpp
enum class LogCategory {
    Core,       // Core system
    Renderer,   // Rendering pipeline
    Audio,      // Audio engine
    UI,         // UI widgets
    Platform,   // Platform layer
    Profiler,   // Performance profiling
};
```

### **Verbosity Levels**
```cpp
enum class LogLevel {
    Trace,      // Most verbose
    Debug,      // Debug info
    Info,       // General info
    Warning,    // Warnings
    Error,      // Errors
    Critical,   // Critical errors
};
```

---

## **📁 File Paths & Organization**

### **Asset Paths**
```
NomadAssets/
  ├── fonts/
  │   ├── Inter-Regular.ttf
  │   ├── Inter-Regular-sdf.png
  │   └── Inter-Regular-sdf.json
  ├── shaders/
  │   ├── text_sdf.vert
  │   └── text_sdf.frag
  └── icons/
```

### **Debug Output Paths**
```
NomadDocs/
  ├── logs/
  │   ├── nomad-[date].log
  │   └── performance-[date].csv
  └── profiler/
      └── tracy-[date].tracy
```

---

## **🛠️ Useful Commands**

### **Build Commands**
```bash
# Debug build
cmake --build build --config Debug

# Release build
cmake --build build --config Release

# Clean rebuild
cmake --build build --target clean
cmake --build build
```

### **Testing Commands**
```bash
# Run with profiler
bin\Release\NomadDAW.exe --profile

# Run with debug overlay
bin\Release\NomadDAW.exe --debug

# Export metrics
bin\Release\NomadDAW.exe --export-metrics metrics.csv
```

---

## **📞 Quick Troubleshooting**

| Problem | Check | Solution |
|---------|-------|----------|
| **Tracy crashes** | CPU flags | Use `/arch:SSE2` |
| **Text not rendering** | GLAD loaded? | Call `gladLoadGL()` |
| **Low FPS** | Cache enabled? | Check tier detection |
| **High VRAM** | Atlas size? | Reduce to 1024×1024 |
| **Graphs not showing** | Metrics enabled? | Toggle with F12 |

---

**End of Reference Card**  
*Keep this document open during Phase II implementation.*
