# NOMAD DAW — Bug Batch (2025-11-05)

## Triage Summary

* **P0 (must fix before next build):** B-01, B-02, B-03
* **P1 (next sprint UI/UX):** B-04, B-05
* **P3 (cosmetic/low risk):** B-06, B-07

*Default test rig:* Windows 10/11, WASAPI Shared & Exclusive, 44.1 k/48 k projects, buffer 128–512, Nomad build `develop` (commit TBD). Use low monitor volume for P0 tests.

---

## B-01: Transport spam leads to “buzz” that grows until app restart

**Severity:** P0 • **Area:** Transport / Mixer / Resampler
**Symptom:** Repeatedly pressing **Play** while audio is running slowly puts the mixer into a persistent buzzing state that gets louder over time; only closing the app clears it.

**Likely causes (hypotheses):**

* Resampler/IIR state not reset on rapid transport state changes → DC/NaN builds up.
* Double-start or stale ring-buffer pointers after rapid start/stop → reading garbage.
* Missing fade/crossfade on start/stop → discontinuities produce broadband buzz that feeds effects.

**How to reproduce**

1. Open any project with a looping clip.
2. While audio is playing, spam **Space/Play** 2–3× per second for ~10–20 s.
3. Observe buzz appearing and increasing in level; persists across transport states.

**Expected:** Audio stays clean; no cumulative buzz; stopping resets state.
**Actual:** Buzz accumulates and persists.

**Diagnostics to add**

* Guarded transport state machine (atomic): `Stopped → Starting → Playing → Stopping`.
* On `Start/Stop`: hard-reset resampler/filters/ring-buffers; clear denormals.
* Add 5 ms linear fade-out on stop, 5 ms fade-in on start.
* Log any mismatched device SR vs project SR transitions.

**Acceptance**

* 1 minute of aggressive Play/Stop toggling yields no buzz, no NaNs, no clipped peaks.
* Mixer peak stays within ±0.1 dB of baseline when idle after toggling.

**Tasks**

* [ ] Reset DSP state on transport transitions (`NomadAudio/Transport.cpp`, mixer graph nodes).
* [ ] Add start/stop de-click fades on master bus.
* [ ] Add `ASSERT(!isnan(sample))` in debug.
* [ ] Unit test: `Transport_Toggle_NoArtifacts`.

---

## B-02: Loud, distorted output when another app is using the device (Shared↔Exclusive auto-switch failure)

**Severity:** P0 • **Area:** Device I/O / WASAPI mode negotiation
**Symptom:** With **SoundID** (or any app) already using the audio device, launching Nomad and pressing Play produces very loud, distorted audio. Indicates failed or partial fallback between Exclusive and Shared modes.

**Likely causes**

* Attempting Exclusive, failing, then running with partially initialized format/latency.
* Project SR/bit-depth not negotiated to device mix format; clipping at the mixer.
* Host volume not reset / double-gain path when switching modes.

**How to reproduce**

1. Launch SoundID (or any player) and play a continuous tone/music.
2. Launch Nomad; open a project; press Play.
3. Distortion and level jump occur.

**Expected:** If Exclusive is unavailable, gracefully fall back to Shared; auto-resample cleanly.
**Actual:** Distortion, very loud output.

**Diagnostics / hardening**

* On init: probe `IAudioClient(3)::IsFormatSupported` for Exclusive; if denied, **explicitly** init Shared with the device’s **mix format**.
* Normalize Nomad’s graph to device format (SR/bit-depth/channels) with high-quality SRC.
* Add -6 dB safety headroom at output until calibration is complete.
* Emit telemetry: `mode={exclusive|shared}`, `device_sr`, `graph_sr`, `xrun_count`.

**Acceptance**

* With another app holding the device, Nomad starts clean in Shared (no distortion).
* Switching other app off allows re-init to Exclusive without pops.
* No peak exceeds 0 dBFS during the entire transition.

**Tasks**

* [ ] Robust mode negotiation & re-init path (`WASAPIDriver.cpp`).
* [ ] Single source of truth for graph format; resampler quality flag.
* [ ] UI toast: “Exclusive not available — running in Shared.”

---

## B-03: Moving a sample near the playhead crashes/loses audio

**Severity:** P0 • **Area:** Timeline model ↔ audio thread synchronization
**Symptom:** Drag a clip to the right while playing; when the playhead nears the clip edge, audio engine crashes or goes silent. Same behavior when moving the clip to another track.

**Likely causes**

* Timeline mutations touching buffers used by the audio thread (use-after-free).
* Rebuilding region indices while the RT thread iterates them (no copy-on-write).
* Missing safe point for graph change; invalid iterator on region vector.

**How to reproduce**

1. Play a loop.
2. Grab an audio clip and drag it right so the playhead approaches the clip boundary.
3. Observe dropout/crash; engine loses audio until restart.

**Expected:** Non-blocking edit; engine remains stable.
**Actual:** Engine drop/crash.

**Fix strategy**

* Double-buffer the timeline: edits enqueue commands; RT thread consumes at block boundaries.
* Clip handles become immutable IDs; moving a clip creates a new immutable view; old freed after swap.
* Add try-lock guard—if graph is mid-rebuild, schedule for next block.

**Acceptance**

* 100 consecutive drag moves during playback cause zero engine resets and ≤1 buffer xrun.
* Fuzz test: random clip moves for 60 s without silence.

**Tasks**

* [ ] Command queue + fence at audio block boundary.
* [ ] Copy-on-write clip registry.
* [ ] Regression test `EditDuringPlayback_NoCrash`.

---

## B-04: Ruler zoom is anchored to the left instead of the focused area

**Severity:** P1 • **Area:** Timeline UI / Zoom controller
**Symptom:** Zooming in/out always hugs the left edge instead of the focused cursor/selection.

**How to reproduce**

1. Place mouse over bar X (not near left edge).
2. Ctrl+Wheel (or UI zoom).
3. View jumps/grows from left, losing context.

**Expected:** Zoom should center on mouse (or selection) and preserve the time under the cursor.
**Actual:** Zoom anchored left.

**Proposed behavior (math)**

```cpp
// world = time in pixels, view = scroll offset in pixels
float ratio = (mouseX - viewOffset) / viewWidth;     // 0..1 under cursor
float newWidth = clamp(viewWidth * zoomFactor, minW, maxW);
float newOffset = mouseX - ratio * newWidth;
scrollTo(newOffset);
```

**Acceptance**

* Time under cursor changes by <1 pixel after ±5 zoom steps.
* Keyboard zoom centers on selection if present; else on playhead; else on mouse.

**Tasks**

* [ ] Implement cursor-anchored zoom.
* [ ] Add preference: `Zoom Anchor = Mouse | Playhead | Center`.

---

## B-05: Ruler “infinite mode” makes horizontal scrollbar shrink endlessly

**Severity:** P1 • **Area:** Timeline range/scrollbar policy
**Symptom:** Scrolling far right reaches a point where the ruler extends indefinitely; scrollbar thumb can’t reach the right and keeps shrinking.

**Likely causes**

* Content width bound to a virtual “∞” instead of project length (last event + margin).
* Thumb size tied to `contentWidth / viewWidth` without min clamp.

**Fix strategy**

* Define **Project Horizon** = max( lastEventEnd, loopEnd ) + safetyMargin (e.g., 8 bars).
* Scrollbar clamps to `[0, ProjectHorizon - viewWidth]`.
* Enforce minimum thumb size (e.g., ≥12 px).
* Lazy-extend horizon only when new content is placed beyond current horizon.

**Acceptance**

* Thumb remains stable size; can reach the true end; no “runaway” shrinkage.

**Tasks**

* [ ] Compute dynamic horizon each frame or on edits.
* [ ] Min thumb size; proper clamping.

---

## B-06: Remove shadow from dropdowns

**Severity:** P3 • **Area:** UI theme
**Change:** Drop the shadow on dropdown menus for a cleaner, flatter look.

**Acceptance**

* All dropdowns render without shadow; contrast and focus ring remain accessible (WCAG).

**Tasks**

* [ ] Update theme token (e.g., `DropDown.shadow = None`).
* [ ] Visual QA in light/dark themes.

---

## B-07: Re-enable animations on **Apply** / **Cancel** buttons (regressed with FBO caching)

**Severity:** P3 • **Area:** UI caching / animations
**Symptom:** Button hover/press animations disappeared after introducing FBO caching.

**Likely causes**

* Cached static FBO for the button’s layer ignores per-frame invalidations.
* `needsRedraw` not set during animation ticks.

**Fix strategy**

* Exclude animated widgets from static cache, or tag them with a small per-frame dirty rect.
* Ensure animation driver marks the widget dirty each frame.

**Acceptance**

* Hover/press animations are visible at 60 fps; caching remains effective elsewhere (≤1% FPS delta).

**Tasks**

* [ ] Cache policy: `Static unless Animated`.
* [ ] Dirty-rect propagation from animation timer.

---

## Test / Instrumentation Notes

* For now (Remotery): wrap critical zones with `rmt_BeginCPUSample("TransportStart", 0)` / `rmt_EndCPUSample();` and add counters for xruns, NaNs, gain overs.
* Add debug overlay meters:

  * `mode: shared/exclusive`, `device_sr`, `graph_sr`
  * `xrun_count`, `nan_count`, `max_peak_dBFS`, `resampler_state_reset_count`
* Safety: auto-attenuate master by -12 dB if peak > 0 dBFS for >50 ms during init/mode switch.

---

## GitHub Issue Templates (copy-paste)

**Title:** `[P0][Audio] Transport spam causes persistent buzz until restart`
**Labels:** `bug`, `audio`, `transport`, `P0`
**Body:** Use section **B-01** above.

**Title:** `[P0][I/O] Distorted output when other app holds device (Shared↔Exclusive fallback)`
**Labels:** `bug`, `audio-io`, `wasapi`, `P0`
**Body:** Use section **B-02** above.

**Title:** `[P0][RT] Moving clip near playhead crashes/loses audio`
**Labels:** `bug`, `rt-thread`, `timeline`, `P0`
**Body:** Use section **B-03** above.

**Title:** `[P1][UI] Zoom should anchor to cursor/selection`
**Labels:** `ui`, `timeline`, `usability`, `P1`
**Body:** Use section **B-04** above.

**Title:** `[P1][UI] Horizontal scrollbar shrinks in “infinite mode”`
**Labels:** `ui`, `timeline`, `scrollbar`, `P1`
**Body:** Use section **B-05** above.

**Title:** `[P3][Theme] Remove dropdown shadows`
**Labels:** `ui`, `theme`, `P3`
**Body:** Use section **B-06** above.

**Title:** `[P3][UI] Restore button animations with FBO caching`
**Labels:** `ui`, `animation`, `rendering`, `P3`
**Body:** Use section **B-07** above.
