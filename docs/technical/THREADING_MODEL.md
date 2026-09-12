# Aestra Threading Model (B-004)

This document describes the threading architecture of Aestra.

## Thread Overview

Aestra uses a multi-threaded architecture with three main thread types:

```
┌─────────────────────────────────────────────────────────────────────┐
│                          MAIN THREAD                                │
│  • Event loop (AestraApp::run)                                       │
│  • UI rendering (NUIRenderer)                                       │
│  • User input handling                                              │
│  • Non-real-time audio commands                                     │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │ Lock-free command queue
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         AUDIO THREAD                                │
│  • Real-time audio processing (AudioEngine::process)                │
│  • DSP operations (mixing, effects)                                 │
│  • ⚠️NO allocations, NO locks, NO I/O                               │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │ Atomic flags / counters
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        WORKER THREADS                               │
│  • Autosave (async file writes)                                     │
│  • Waveform decoding (background loading)                           │
│  • Plugin scanning                                                  │
│  • Export/render operations                                         │
└─────────────────────────────────────────────────────────────────────┘ 
```

## Thread Responsibilities

### 1. Main Thread (UI Thread)

**Entry Point:** `main()` → `AestraApp::initialize()` → `AestraApp::run()`

**Responsibilities:**
- Application lifecycle management
- Window creation and management
- OpenGL rendering context owner
- UI event handling (mouse, keyboard)
- Menu actions and dialogs
- Project save/load coordination
- Sending commands to audio thread

**Safe Operations:**
- ✅ Memory allocation
- ✅ Mutex locks
- ✅ File I/O
- ✅ Logging
- ✅ Exception handling

**Key Classes:**
- `AestraApp` - Application lifecycle
- `NUIRenderer` - UI rendering
- `NUIPlatformBridge` - Window/input abstraction
- `TrackManagerUI` - Track UI components

### 2. Audio Thread (Real-Time Thread)

**Entry Point:** OS audio callback → `AudioDeviceManager` → `AudioEngine::process()`

**Responsibilities:**
- Real-time audio processing
- Mixing tracks and buses
- Applying effects
- Metronome generation
- Recording input to buffers
- Transport position tracking

**⚠️ FORBIDDEN Operations:**
- ❌ Memory allocation (`new`, `malloc`, `std::vector::push_back`)
- ❌ Mutex locks (`std::mutex`, `std::lock_guard`)
- ❌ File I/O (`fopen`, `std::ifstream`)
- ❌ Logging (may allocate internally)
- ❌ System calls that block
- ❌ Throwing exceptions

**Safe Operations:**
- ✅ Atomic reads/writes
- ✅ Lock-free queue operations
- ✅ Pre-allocated buffer access
- ✅ Simple math and DSP

**Key Classes:**
- `AudioEngine` - Core audio processing
- `AudioCommandQueue` - Lock-free UI→Audio communication
- `TrackManager` - Track state management

### 3. Worker Threads (Background)

**Entry Point:** `std::async()` or thread pool

**Responsibilities:**
- Autosave (periodic project snapshots)
- Waveform decoding (loading audio files)
- Plugin scanning
- Offline rendering/export
- Large file operations

**Safe Operations:**
- ✅ Memory allocation
- ✅ Mutex locks (with care)
- ✅ File I/O
- ✅ Network operations

**Key Classes:**
- Autosave tasks in `AestraApp`
- `WaveformCache` background loading
- `PluginManager` scanning

## Communication Patterns

### UI → Audio: Command Queue

```cpp
// Main thread: send command
AudioQueueCommand cmd;
cmd.type = AudioQueueCommandType::SetTransportState;
cmd.value1 = 1.0f; // playing
m_commandQueue.push(cmd);

// Audio thread: process commands
AudioQueueCommand cmd;
while (m_commandQueue.pop(cmd)) {
    switch (cmd.type) {
        case SetTransportState: ...
    }
}
```

### Audio → UI: Atomic Flags

```cpp
// Audio thread: signal event
m_underrunFlag.store(true, std::memory_order_release);

// Main thread: check and clear
if (m_underrunFlag.exchange(false)) {
    showUnderrunWarning();
}
```

### Audio → UI: Atomic Counters

```cpp
// Audio thread: update position
m_globalSamplePos.store(pos, std::memory_order_relaxed);

// Main thread: read position
uint64_t pos = m_globalSamplePos.load(std::memory_order_relaxed);
```

## Lifecycle State Machine (B-002)

```
[Created] ──► [Initializing] ──► [Running] ──► [ShuttingDown] ──► [Terminated]
                    │                               ▲
                    ▼                               │
              [InitFailed] ─────────────────────────┘
```

Use `AppLifecycle::instance().getState()` to query current state.

## RT Thread Checks (B-005)

There is exactly one real-time state flag and one violation-reporting call, both
in `RealtimeThreadGuard.h`. Do not add a parallel flag or counter: a detection
surface that only some call sites consult reports "clean" for thread state it
never observed.

```cpp
// At the start of the audio callback (and in AudioEngine::processBlock).
// Depth counted, so the two nest correctly.
Aestra::Audio::ScopedRealtimeAudioThread guard;

// Query thread context.
if (Aestra::Audio::isRealtimeAudioThread()) {
    // We're on the audio thread
}

// Guard a non-real-time API. The return value is the refusal signal:
// true means "this was called from the audio thread" — bail out.
void MixerChannel::setMute(bool muted) {
    if (Aestra::Audio::reportRealtimeMisuse("MixerChannel::setMute")) return;
    // ...
}
```

Reports dispatch to the handler installed with `setRealtimeMisuseHandler()`;
`AudioEngine` installs one at startup. In debug builds with no handler
installed, a report asserts.

### The compile-time half

Everything above is detection: it tells you a violation happened, after it
happened, on a machine that ran the code. B-005 also asks for the violation to
be rejected before it exists.

Mark a function that runs on the audio thread with `AESTRA_RT_NONBLOCKING`,
declared in `RealtimeThreadGuard.h`:

```cpp
// In the header — the declaration is what callers in other translation units
// see, so annotating only the definition checks the body and tells them nothing.
void processStereo(const float* interleaved,
                   uint32_t numFrames) noexcept AESTRA_RT_NONBLOCKING;
```

Under Clang 20+ this expands to `[[clang::nonblocking]]`, and
`-Wfunction-effects` walks the call graph out of it: allocation, deallocation,
locks, throws, atomic waits, thread-local access, indirect calls it cannot
resolve, and any call it cannot prove non-blocking all become errors. Under any
other compiler the macro expands to nothing, so annotations are free to apply
everywhere and are simply unenforced there.

Two properties worth knowing before adopting it:

- **The obligation propagates.** Annotating an entry point pulls in everything it
  reaches, transitively. That is the value, and it is also why a large entry
  point is a project rather than an edit.
- **It is opt-in per function.** An unannotated function is never inspected, so
  the check cannot break code nobody has annotated. Coverage is exactly the set
  of functions carrying the macro — no more, and no less than that set.

Run it locally:

```bash
cmake -S . -B build-clang -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAESTRA_RT_EFFECT_CHECK=ON
./scripts/ci/check-rt-effects.sh build-clang
```

CI runs the same script in the `RT effect check` lane. The script refuses to
report success unless it has first confirmed the compiler diagnoses a deliberate
violation — an older Clang parses the attribute as unknown and checks nothing,
which would otherwise look identical to a clean tree.

## Best Practices

1. **Never block the audio thread** - Use lock-free queues for communication

2. **Pre-allocate everything** - Audio buffers, command queues, lookup tables

3. **Use atomic operations** - For simple values shared between threads

4. **Design for worst case** - Audio callback must complete in buffer period

5. **Test under load** - Run soak tests with stress on all threads

6. **Profile regularly** - Use Tracy or similar to identify bottlenecks

## Related Files

- `AppBootstrap.h` - Initialization modules (B-001)
- `AppLifecycle.h` - Lifecycle states (B-002)
- `ServiceLocator.h` - Service registry (B-003)
- `RealtimeThreadGuard.h` - RT thread state and misuse reporting (B-004, B-005)
