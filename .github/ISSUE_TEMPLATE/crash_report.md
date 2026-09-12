---
name: Crash Report
about: Aestra closed, froze, or lost work — what to attach so it can be fixed
title: '[CRASH] '
labels: 'bug, priority: critical'
assignees: ''
---

# Crash Report

<!--
A crash report is worth exactly as much as what is attached to it. Aestra
records more than most people realise, and almost none of it is anywhere
obvious, so this template is mostly a map to it.

If you can only manage one thing: attach the two log files. Everything else
below can be reconstructed from them or asked for later.
-->

## What happened

<!-- One or two sentences. "Closed instantly with no dialog while dragging a
clip" is more useful than "crashed". -->

## Did it close, freeze, or misbehave?

- [ ] Closed instantly (no dialog)
- [ ] Froze / stopped responding
- [ ] Showed an error and kept running
- [ ] Lost work without closing

## The logs — please attach both

Aestra writes **two** log files, and they are not copies of each other:

| File | Holds |
|---|---|
| `runtime_log.txt` | The first moments of startup — build id, working directory, font setup. Small. |
| `aestra_debug.log` | Everything after that — audio device, project load, the run itself. This is usually the one with the crash in it. |

**Where they are:** whatever directory Aestra was *launched from*, not a fixed
folder. Launched from a terminal, they are in that directory. Launched from a
desktop icon or app menu, they are wherever that launcher's working directory
points — most often your home folder.

If you cannot find them, search your home folder for `aestra_debug.log`. The
top of `runtime_log.txt` prints a line reading `Working Directory: ...`, which
tells you where that pair came from — useful if you find several.

> **Both files append and are never rotated**, so a file from months of use
> holds many runs. Please include the whole file rather than trimming it; if it
> is too large, the last few hundred lines plus the timestamp of the crash is
> enough.

- [ ] `aestra_debug.log` attached
- [ ] `runtime_log.txt` attached

## The project

- [ ] `.aes` project file attached (or: it was an unsaved new project)

**If work was lost, also look for the autosave.** It sits next to your project
rather than in a system folder:

```
YourProject.aes            <- the project
YourProject.autosave/      <- timestamped backups live here
```

- [ ] Autosave / backup files attached, or none existed

<!-- If the project is private, say so — a crash can usually be narrowed down
from the logs alone, and a redacted or cut-down project that still reproduces
it is just as good. -->

## Build

<!-- Copy the two lines from the top of runtime_log.txt. They look like:
     Aestra Starting - Aestra-2025-Core
     Working Directory: /home/you
     If you built it yourself, the git commit is more useful than a version. -->

- Build id / commit:
- Release or Debug:
- Installed, or built from source:

## System

- OS and version:
- CPU / RAM:
- Audio device and driver (WASAPI / ALSA / JACK / PipeWire):
- Sample rate and buffer size, if you know them:

<!-- Audio device details matter more than they look: a large share of crashes
and freezes come in through the device layer, and buffer size is the first
thing anyone will ask about a dropout or a hang. -->

## Reproducing it

**How often:**

- [ ] Every time, same steps
- [ ] Sometimes, same steps
- [ ] Once so far

**Steps:**

1.
2.
3.

<!-- If it only happened once and you cannot reproduce it, still file it. The
logs are the point; an unrepeatable crash with a full log is a real report. Say
what you were doing in the seconds before, even vaguely — that is often enough
to find it in the log. -->

## Anything else

<!-- Plugins loaded (and whether removing one stops it), whether it started
after a specific change, another app using the audio device at the time. -->

---

*"Clarity before speed."*
