# **NOMAD Phase II – Task Dependencies & Flow**

---

## **📊 Visual Dependency Graph**

```
┌─────────────────────────────────────────────────────────────┐
│                        PHASE II START                       │
└──────────────────────┬──────────────────────────────────────┘
                       │
          ┌────────────┴────────────┐
          ▼                         ▼
┌───────────────────┐     ┌───────────────────┐
│   TASK 3: GLAD    │     │  TASK 6: TIER     │
│   Installation    │     │    Detection      │
│   [1-2 days]      │     │   [2-3 days]      │
│   Priority: HIGH  │     │  Priority: MEDIUM │
└─────────┬─────────┘     └──────────┬────────┘
          │                          │
          │ ┌───────────────┐        │
          │ │  TASK 1:      │        │
          ├►│  Tracy Fix    │        │
          │ │  [3-4 days]   │        │
          │ │  Priority:    │        │
          │ │  HIGH         │        │
          │ └───────┬───────┘        │
          │         │                │
          │         │ If Fails       │
          │         ▼                │
          │ ┌───────────────┐        │
          │ │  TASK 2:      │        │
          │ │  Remotery     │        │
          │ │  Optimize     │        │
          │ │  [2-3 days]   │        │
          │ │  Priority:    │        │
          │ │  HIGH         │        │
          │ └───────┬───────┘        │
          │         │                │
          ▼         ▼                │
┌──────────────────────────┐         │
│   TASK 4: GPU SDF Text   │         │
│   Rendering              │         │
│   [5-7 days]             │         │
│   Priority: CRITICAL     │         │
│                          │         │
│   Sub-tasks:             │         │
│   • 4.1 Atlas Gen (1d)   │         │
│   • 4.2 Shaders (2d)     │         │
│   • 4.3 C++ Integ (2-3d) │         │
│   • 4.4 QA (1-2d)        │         │
│   • 4.5 Perf Val (1d)    │         │
└────────────┬─────────────┘         │
             │                       │
             ▼                       │
┌──────────────────────────┐         │
│   TASK 5: Debug Overlay  │         │
│   [3-4 days]             │◄────────┘
│   Priority: HIGH         │
│                          │
│   Sub-tasks:             │
│   • 5.1 Architecture (1d)│
│   • 5.2 Metrics (1d)     │
│   • 5.3 Graphs (1d)      │
│   • 5.4 Integration (1d) │
│   • 5.5 Styling (1d)     │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│  INTEGRATION TESTING     │
│  [2-3 days]              │
│  • All tasks together    │
│  • Performance validation│
│  • Quality checks        │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│   DOCUMENTATION UPDATE   │
│   [1-2 days]             │
│   • Architecture docs    │
│   • User guides          │
│   • API references       │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│     PHASE II COMPLETE    │
└──────────────────────────┘
```

---

## **🔀 Parallel vs Sequential Tasks**

### **Can Run in Parallel:**
- ✅ Task 3 (GLAD) + Task 6 (Tier Detection)
- ✅ Task 1 (Tracy) + Task 6 (Tier Detection)
- ✅ Task 2 (Remotery) + Task 6 (Tier Detection)

### **Must Run Sequentially:**
- ❌ Task 3 → Task 4 (GLAD required for GPU text)
- ❌ Task 1/2 → Task 5 (Profiler needed for metrics)
- ❌ Task 4 → Task 5 (GPU text can be used in overlay)

---

## **⚡ Critical Path Analysis**

**Longest Path (Worst Case):**
```
Task 3 (2d) → Task 1 (4d) → Task 4 (7d) → Task 5 (4d) → Testing (3d) = 20 days
```

**Shortest Path (Best Case):**
```
Task 3 (1d) → Task 1 (3d) → Task 4 (5d) → Task 5 (3d) → Testing (2d) = 14 days
```

**Realistic Path:**
```
Task 3 (1-2d) → Task 1 (3d) → Task 4 (6d) → Task 5 (3d) → Testing (2d) = 15-16 days
```

**If Tracy Fails:**
```
Task 3 (2d) → Task 1 attempt (2d) → Task 2 (3d) → Task 4 (6d) → Task 5 (3d) = 16 days
```

---

## **📅 Timeline Visualization**

```
Week 1    │ █████ T3 █████ | ████ T1 ████████ |
Week 2    │ ████ T1 ██ | ████████ T4 ████████ |
Week 3    │ ████████ T4 ████████ | ████ T4 ██ |
Week 4    │ ██ T4 ██ | ███████ T5 ███████ |    |
Week 5    │ ██ T5 ██ | ████ Testing ████ |     |
Week 6    │ ██ Docs ██ | █ Final █ |           |

Legend: T3=GLAD, T1=Tracy, T4=GPU Text, T5=Debug Overlay
Task 6 (Tier Detection) runs in parallel during Week 1-2
```

---

## **🎯 Milestone Markers**

### **Milestone 1: Foundation Complete**
**When:** End of Week 1  
**Criteria:**
- ✅ GLAD installed and verified
- ✅ Profiler decision made (Tracy or Remotery)
- ✅ OpenGL 4.3+ confirmed

**Deliverables:**
- Extension logs
- Profiler test results
- Go/No-Go decision document

---

### **Milestone 2: GPU Text Rendering Live**
**When:** End of Week 3  
**Criteria:**
- ✅ SDF atlases generated
- ✅ Text renders on GPU
- ✅ Quality matches CPU baseline
- ✅ Performance >2x improvement

**Deliverables:**
- SDF atlas files
- Shader code
- Performance benchmarks
- Quality comparison screenshots

---

### **Milestone 3: Diagnostics Complete**
**When:** End of Week 5  
**Criteria:**
- ✅ Debug overlay functional
- ✅ All metrics displaying
- ✅ Tier detection working
- ✅ Performance maintained

**Deliverables:**
- Debug overlay screenshots
- Metrics accuracy validation
- Tier detection test results

---

### **Milestone 4: Phase II Shipped**
**When:** End of Week 6  
**Criteria:**
- ✅ All tests passing
- ✅ Documentation complete
- ✅ Performance targets met
- ✅ Quality maintained

**Deliverables:**
- Final build
- Complete documentation
- Performance report
- Known issues log

---

## **⚠️ Blocking Relationships**

| Task | Blocks | Reason |
|------|--------|--------|
| **Task 3 (GLAD)** | Task 4 | GPU text needs OpenGL extensions |
| **Task 1/2 (Profiler)** | Task 5 | Debug overlay uses profiler metrics |
| **Task 4 (GPU Text)** | Task 5 | Overlay can use GPU text (optional) |
| **All Core Tasks** | Integration | Cannot test until features complete |

---

## **🔄 Fallback Paths**

### **If Tracy Fails:**
```
Task 1 (Attempt) → Task 2 (Remotery) → Continue
     [3 days]           [3 days]
```
**Time Lost:** ~1 day (overlap in attempt)

### **If SDF Quality Insufficient:**
```
Task 4.4 (QA Fail) → Increase Resolution → Re-test
                          [1 day]
```
**Time Lost:** 1-2 days

### **If VRAM Exceeds Budget:**
```
Task 4 (Complete) → Add LRU Eviction → Re-test
                         [2 days]
```
**Time Lost:** 2 days

---

## **💡 Optimization Opportunities**

### **Reduce Critical Path:**
1. **Start Task 4.1 (Atlas Gen) early** – Can generate atlases before shaders ready
2. **Parallelize Task 5 sub-tasks** – Metrics collection can start before graphs
3. **Overlap Testing with Task 6** – Tier detection can be tested independently

### **Accelerate Timeline:**
1. Use pre-built Tracy binaries (skip compilation)
2. Use existing SDF atlas tools (skip custom generator)
3. Reuse graph rendering from other widgets

---

## **📊 Resource Allocation**

**Single Developer Timeline:**
- Week 1-2: GLAD + Profiler (full focus)
- Week 2-4: GPU Text (full focus)
- Week 4-5: Debug Overlay (full focus)
- Week 5-6: Testing + Docs (full focus)

**Two Developer Timeline:**
- Dev 1: Tasks 3, 1/2, 4 (Critical path)
- Dev 2: Task 6, then assist with Task 5

**Could reduce to 3-4 weeks with two developers.**

---

## **✅ Daily Progress Tracking**

Use this format in daily logs:

```
Day X – [Date]
Current Task: [Task Number]
Progress: [Percentage or sub-task completed]
Blockers: [None / List issues]
Next Steps: [Tomorrow's plan]
```

---

**End of Dependencies Document**
