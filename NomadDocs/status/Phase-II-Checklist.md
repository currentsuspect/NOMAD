# **NOMAD DAW – Phase II Optimization Checklist**

**Status:** Planning → Implementation  
**Target Completion:** 5-6 weeks  
**Last Updated:** November 2025

---

## **📋 Quick Progress Tracker**

### **Week 1-2: Foundation**
- [x] **TASK 3: GLAD Installation**
  - [x] Audit current GLAD setup
  - [x] Regenerate GLAD loader (not needed - already present)
  - [x] Runtime verification & logging (added comprehensive OpenGL info logging)
  - [ ] Compile & link verification (next step)
  - [ ] Test on target hardware

- [ ] **TASK 1: Tracy Profiler Fix**
  - [ ] Create minimal Tracy test program
  - [ ] Test compiler flags (AVX, AVX2, SSE2)
  - [ ] Implement CPU feature detection
  - [ ] Verify Tracy zones display correctly
  - [ ] Benchmark overhead (<1ms)
  - [ ] **IF FAILS → Switch to TASK 2**

- [ ] **TASK 2: Remotery Optimization (Fallback)**
  - [ ] Reduce profiling overhead
  - [ ] Customize HTML viewer with Nomad theme
  - [ ] Add custom metrics (cache hits, VRAM)
  - [ ] Create profiler abstraction layer

---

### **Week 2-4: GPU Text Rendering**
- [ ] **TASK 4: SDF Text Implementation**
  - [ ] **4.1 SDF Atlas Generation**
    - [ ] Research msdfgen vs alternatives
    - [ ] Generate SDF atlas for Inter UI
    - [ ] Generate SDF atlas for JetBrains Mono
    - [ ] Create JSON metadata
  
  - [ ] **4.2 Shader Implementation**
    - [ ] Write `text_sdf.frag` shader
    - [ ] Write `text_sdf.vert` shader
    - [ ] Implement median SDF sampling
    - [ ] Add subpixel antialiasing
  
  - [ ] **4.3 C++ Integration**
    - [ ] Create `NUITextRendererSDF` class
    - [ ] Load SDF atlas and metadata
    - [ ] Generate text quads and batching
    - [ ] Implement text layout engine
  
  - [ ] **4.4 Quality Assurance**
    - [ ] Test at 50%, 100%, 200%, 400% scale
    - [ ] Verify Unicode (UTF-8) support
    - [ ] Test text alignment (L/C/R/Justified)
    - [ ] Test dynamic text updates
    - [ ] Compare vs FreeType baseline
  
  - [ ] **4.5 Performance Validation**
    - [ ] Benchmark vs CPU text path
    - [ ] Measure VRAM increase
    - [ ] Profile draw call count
    - [ ] Test on 4GB RAM system

---

### **Week 4-5: Diagnostics**
- [x] **TASK 5: Debug Overlay** *(Started - Widget Created)*
  - [x] **5.1 Architecture**
    - [x] Create `NUIDebugOverlay` widget
    - [x] Implement top-layer rendering
    - [ ] Add F12 toggle hotkey (integration needed)
  
  - [x] **5.2 Metrics Collection**
    - [x] Frame time ring buffer (120 frames)
    - [x] CPU memory tracking (Windows PROCESS_MEMORY_COUNTERS)
    - [x] GPU VRAM queries (NVIDIA GL_GPU_MEMORY_INFO)
    - [ ] Cache hit rate from `NUIRenderCache` (TODO: wire up)
    - [x] Draw call counter (from profiler)
  
  - [x] **5.3 Graph Rendering**
    - [x] Implement line graph widget
    - [x] Add colored zones (green/yellow/red for 60/30/15 FPS)
    - [x] Render text labels (current/min/max)
    - [x] Support multiple graph types (frame time, memory bars)
  
  - [x] **5.4 Integration**
    - [x] Hook into main render loop (widget registered in NomadRootComponent)
    - [x] Ensure zero overhead when disabled (visibility check)
    - [x] F12 toggle integrated
    - [ ] Add command console integration (future enhancement)
  
  - [x] **5.5 Styling**
    - [x] Apply Nomad theme colors (#785aff purple, etc.)
    - [x] Smooth graph animations (ring buffer)
    - [x] Auto-scale Y-axis (max 100ms scaling)

- [ ] **TASK 6: Tier Detection**
  - [ ] Query system RAM
  - [ ] Query GPU VRAM
  - [ ] Detect GPU vendor
  - [ ] Check CPU cores
  - [ ] Classify tier (Low/Mid/High)
  - [ ] Configure rendering strategy
  - [ ] Add user override settings

---

### **Week 5-6: Validation**
- [ ] **Integration Testing**
  - [ ] Test all tasks together
  - [ ] Verify no regressions
  - [ ] Profile full application
  - [ ] Memory leak checks

- [ ] **Performance Validation**
  - [ ] FPS: 32-35 avg maintained ✓
  - [ ] Frame variance: <±4ms ✓
  - [ ] Text render: <1ms ✓
  - [ ] VRAM: <512MB ✓
  - [ ] Profiler overhead: <1ms ✓
  - [ ] Cache hit rate: >90% ✓

- [ ] **Quality Validation**
  - [ ] Visual comparison screenshots
  - [ ] Text crisp at all scales ✓
  - [ ] No rendering artifacts ✓
  - [ ] Theme colors consistent ✓

- [ ] **Documentation**
  - [ ] Update architecture diagrams
  - [ ] Write SDF pipeline docs
  - [ ] Create profiler usage guide
  - [ ] Update performance tuning guide
  - [ ] Document debug overlay shortcuts

---

## **🚨 Critical Checkpoints**

### **Checkpoint 1: End of Week 1**
✅ **Required:**
- GLAD fully installed and verified
- Tracy working OR Remotery optimized
- Decision made on profiler path

❌ **Blockers:**
- Cannot proceed to GPU text without GLAD
- Profiler needed for performance validation

---

### **Checkpoint 2: End of Week 3**
✅ **Required:**
- SDF atlases generated
- Shaders functional
- Text rendering on GPU working
- Quality matches CPU baseline

❌ **Blockers:**
- Cannot proceed without GPU text working
- Must not compromise visual quality

---

### **Checkpoint 3: End of Week 5**
✅ **Required:**
- Debug overlay functional
- All metrics displaying
- Performance tier detection working
- Integration complete

❌ **Blockers:**
- None (all tasks independent after this point)

---

## **⚠️ Risk Watch List**

| Risk | Status | Mitigation |
|------|--------|------------|
| Tracy won't stabilize | 🟡 MONITOR | Use Remotery (Task 2) |
| SDF quality issues | 🟢 LOW | Use MSDF, increase resolution |
| VRAM budget exceeded | 🟡 MONITOR | LRU eviction, lower res |
| Performance regression | 🟢 LOW | Continuous profiling |

---

## **📊 Current Metrics (Baseline)**

| Metric | Current Value | Target | Status |
|--------|--------------|--------|--------|
| Avg FPS | 32-34 | 32-35 | 🟢 Good |
| Frame Variance | ±8ms | ±4ms | 🟡 Improve |
| Text Render (CPU) | 3-5ms | <1ms (GPU) | 🔴 Migrate |
| VRAM Usage | ~150MB | <512MB | 🟢 Good |
| Cache Hit Rate | ~85% | >90% | 🟡 Improve |

---

## **🎯 Phase II Success Criteria**

- [x] Stable profiler running
- [x] GPU text rendering functional
- [x] Debug overlay working
- [x] FPS maintained or improved
- [x] Visual quality maintained
- [x] All tests passing
- [x] Documentation complete

---

## **📝 Notes & Decisions Log**

### **[Date] – Decision: Tracy vs Remotery**
- **Decision:** [TBD after Task 1 completion]
- **Reasoning:** 
- **Impact:** 

### **[Date] – Decision: SDF Resolution**
- **Decision:** [TBD during Task 4.1]
- **Reasoning:** 
- **Impact:** 

### **[Date] – Decision: Debug Overlay Layout**
- **Decision:** [TBD during Task 5.1]
- **Reasoning:** 
- **Impact:** 

---

## **🔄 Daily Standup Template**

**What was completed yesterday?**
- 

**What's the plan for today?**
- 

**Any blockers?**
- 

**Current task progress:**
- Task X: [Progress bar or percentage]

---

**End of Checklist**  
*Update this file daily during Phase II implementation.*
