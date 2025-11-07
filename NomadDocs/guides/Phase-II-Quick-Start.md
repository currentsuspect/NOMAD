# **NOMAD Phase II – Quick Start Guide**

**Purpose:** Fast implementation reference with code snippets and commands.

---

## **🚀 Task 3: GLAD Setup (Day 1)**

### **Verify Current Installation**
```bash
cd NomadUI\External\glad
dir include\glad\glad.h
dir src\glad.c
```

### **Log OpenGL Info on Startup**
Add to `NUIRendererGL::Initialize()`:

```cpp
void LogOpenGLInfo() {
    NOM_LOG_INFO("OpenGL: %s", glGetString(GL_VERSION));
    NOM_LOG_INFO("Vendor: %s", glGetString(GL_VENDOR));
    NOM_LOG_INFO("Renderer: %s", glGetString(GL_RENDERER));
    
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    NOM_LOG_INFO("Version: %d.%d", major, minor);
}
```

---

## **🚀 Task 1: Tracy Test (Day 2-3)**

### **Minimal Test Program**
Create `tracy-test/main.cpp`:

```cpp
#define TRACY_ENABLE
#include "tracy/Tracy.hpp"
#include <thread>
#include <chrono>

void TestWork() {
    ZoneScoped;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

int main() {
    for (int i = 0; i < 100; ++i) {
        FrameMark;
        TestWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}
```

**Test with different compiler flags:**
- `/arch:SSE2` (safest)
- `/arch:AVX` (if supported)
- `/arch:AVX2` (fastest)

---

## **🚀 Task 4: SDF Text (Week 2-3)**

### **Generate SDF Atlas (using msdfgen)**
```bash
# Install msdfgen
git clone https://github.com/Chlumsky/msdfgen
cd msdfgen
mkdir build && cd build
cmake ..
cmake --build .

# Generate SDF for font
msdfgen.exe msdf -font "C:\path\to\Inter-Regular.ttf" ^
    -charset charset.txt ^
    -size 48 -pxrange 4 ^
    -imageout inter-sdf.png ^
    -json inter-sdf.json
```

### **SDF Fragment Shader**
Create `NomadAssets\shaders\text_sdf.frag`:

```glsl
#version 430 core
in vec2 v_TexCoord;
in vec4 v_Color;
out vec4 fragColor;
uniform sampler2D u_Atlas;
uniform float u_PxRange;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(u_Atlas, v_TexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = u_PxRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    fragColor = vec4(v_Color.rgb, v_Color.a * opacity);
}
```

---

## **🚀 Task 5: Debug Overlay (Week 4)**

### **Performance Metrics Structure**
Create `NUIPerformanceMetrics.h`:

```cpp
#pragma once
#include <array>

struct NUIPerformanceMetrics {
    static constexpr int FRAME_HISTORY = 120;
    
    std::array<float, FRAME_HISTORY> frameTimesMs;
    int frameIndex = 0;
    
    float avgFrameTime = 0.0f;
    float minFrameTime = 999.0f;
    float maxFrameTime = 0.0f;
    
    size_t cpuMemoryMB = 0;
    size_t gpuMemoryMB = 0;
    float cacheHitRate = 0.0f;
    int drawCallCount = 0;
    
    void Update(float deltaMs);
    void Reset();
};
```

### **Debug Overlay Widget**
```cpp
class NUIDebugOverlay : public NUIWidget {
public:
    void Render() override;
    void Toggle() { visible = !visible; }
    
private:
    bool visible = false;
    NUIPerformanceMetrics metrics;
    
    void RenderGraph(const std::array<float, 120>& data);
};
```

---

## **📊 Testing Commands**

### **Build & Run**
```bash
cd build
cmake --build . --config Release
bin\Release\NomadDAW.exe
```

### **Profile with Remotery**
```bash
# Start Nomad
bin\Release\NomadDAW.exe

# Open browser to Remotery viewer
start http://localhost:17815
```

### **Capture with RenderDoc**
1. Launch RenderDoc
2. File → Attach to Running Instance
3. Select NomadDAW.exe
4. Press F12 to capture frame

---

## **✅ Quick Validation Checklist**

**After GLAD (Task 3):**
- [ ] OpenGL 4.3+ reported in logs
- [ ] No GLAD-related compile errors
- [ ] Extensions loaded successfully

**After Tracy/Remotery (Task 1/2):**
- [ ] Profiler connects without crash
- [ ] Frame zones visible
- [ ] Overhead <1ms per frame

**After SDF Text (Task 4):**
- [ ] Text renders on screen
- [ ] Crisp at 50%, 100%, 200% scale
- [ ] Performance >2x faster than CPU

**After Debug Overlay (Task 5):**
- [ ] F12 toggles overlay
- [ ] Frame graph displays
- [ ] Metrics update in real-time

---

**See full spec:** `Optimization Phase II - Technical Specification.md`
