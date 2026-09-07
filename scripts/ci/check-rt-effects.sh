#!/usr/bin/env bash
# © 2026 Aestra Studios — All Rights Reserved. Licensed for personal & educational use only.
#
# Compile-time audio-thread constraint check. Implements the second half of
# B-005 (v0.7.1 trust sprint, objective TS-1, T-2).
#
# Usage:  check-rt-effects.sh [build-dir]
# Exit:   0 = every annotated translation unit is provably non-blocking.
#         1 = a violation, or the check could not be run.
#
# WHAT THIS ACTUALLY CHECKS
#
# Clang's -Wfunction-effects walks the call graph out of every function carrying
# [[clang::nonblocking]] (spelled AESTRA_RT_NONBLOCKING in this tree) and
# diagnoses allocation, deallocation, locks, throws, atomic waits, thread-local
# access, indirect calls it cannot resolve, and calls to any function it cannot
# prove non-blocking. That is the forbidden list from THREADING_MODEL.md, minus
# file I/O and sleeps, which arrive as "calls a function it cannot prove".
#
# It is a *type system* check, not a heuristic and not a grep: the obligation
# propagates through the call graph, so annotating one entry point pulls in
# everything it reaches.
#
# COVERAGE IS THE ANNOTATION SET, AND IT IS DERIVED, NOT DECLARED
#
# There is deliberately no allowlist file. The set of checked translation units
# is computed as "every .cpp under the audio tree that mentions
# AESTRA_RT_NONBLOCKING", so coverage cannot drift away from the annotations the
# way a hand-maintained list does. Add an annotation and its TU is gated on the
# next run; nobody has to remember a second edit.
#
# The honest boundary: a TU that merely *calls* annotated functions without
# annotating anything itself is not compiled here. It does not need to be — the
# diagnostic for an annotated function's body is emitted where that body is, and
# an unannotated caller is making no claim to violate.
#
# FAIL-CLOSED, EVERY PATH
#
# Like check-decision-citation.sh and unlike lane-runs.sh, everything that is not
# a positively verified pass exits 1: no compile_commands.json, no clang, an
# annotated TU with no compile command, a compiler that does not implement the
# attribute. A constraint check that reports success when it did not run is worse
# than no check, because it certifies a claim nobody tested.
set -uo pipefail

BUILD_DIR="${1:-build-clang}"
# Self-locating by default, so the tree under check is the tree this script was
# committed into and cannot be redirected by an unlucky working directory.
# AESTRA_RT_EFFECT_ROOT is a test seam and nothing else — check-rt-effects.test.sh
# sets it to point at a throwaway fixture. CI never sets it.
REPO_ROOT="${AESTRA_RT_EFFECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}"
DB="${BUILD_DIR}/compile_commands.json"

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "FAIL: build directory '${BUILD_DIR}' does not exist."
    echo "      Configure one with:"
    echo "        cmake -S . -B ${BUILD_DIR} -DCMAKE_CXX_COMPILER=clang++ \\"
    echo "              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAESTRA_RT_EFFECT_CHECK=ON"
    exit 1
fi

if [[ ! -f "$DB" ]]; then
    echo "FAIL: ${DB} not found — reconfigure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON."
    exit 1
fi

CXX="${CXX:-clang++}"
if ! command -v "$CXX" >/dev/null 2>&1; then
    echo "FAIL: '${CXX}' not on PATH. This check requires Clang; GCC has no"
    echo "      function-effects analysis and the annotations are inert there."
    exit 1
fi

# Prove the compiler actually implements the attribute before trusting a pass.
# A Clang too old to know [[clang::nonblocking]] parses the annotation as an
# unknown attribute and cheerfully compiles every violation in the tree.
probe="$(mktemp -t rt-effects-probe-XXXXXX.cpp)"
probe_out="$(mktemp -t rt-effects-probe-XXXXXX.log)"
trap 'rm -f "$probe" "$probe_out"' EXIT
cat >"$probe" <<'PROBE'
void violation() [[clang::nonblocking]] { delete new int(1); }
PROBE
if ! "$CXX" -std=c++20 -Wfunction-effects -fsyntax-only "$probe" 2>"$probe_out"; then
    echo "FAIL: probe translation unit did not compile at all:"
    sed 's/^/      /' "$probe_out"
    exit 1
fi
if ! grep -q "function-effects" "$probe_out"; then
    echo "FAIL: ${CXX} did not diagnose a deliberate allocation inside a"
    echo "      [[clang::nonblocking]] function. The attribute is not supported"
    echo "      here, so a pass would be meaningless. Clang 20+ is required."
    "$CXX" --version | sed 's/^/      /'
    exit 1
fi

# The annotated translation units, derived from the annotations themselves.
mapfile -t ANNOTATED < <(
    cd "$REPO_ROOT" &&
    grep -rl --include='*.cpp' 'AESTRA_RT_NONBLOCKING' \
        AestraAudio/src Source 2>/dev/null | sort
)

if [[ ${#ANNOTATED[@]} -eq 0 ]]; then
    echo "FAIL: no translation unit carries AESTRA_RT_NONBLOCKING."
    echo "      Either the annotations were removed or the macro was renamed."
    echo "      An empty check must not report success."
    exit 1
fi

echo "RT effect check — ${#ANNOTATED[@]} annotated translation unit(s)"
echo "compiler: $("$CXX" --version | head -1)"
echo

failed=0
checked=0
for rel in "${ANNOTATED[@]}"; do
    abs="${REPO_ROOT}/${rel}"
    # Pull this TU's real compile line out of the database, so the check sees
    # the same macros and include paths the build does. A hand-rolled command
    # line drifts, and a drifted one silently checks a different program.
    args="$(python3 - "$DB" "$abs" <<'PY'
import json, shlex, sys
db_path, target = sys.argv[1], sys.argv[2]
for entry in json.load(open(db_path)):
    if entry["file"] == target:
        argv = shlex.split(entry.get("command") or "")[1:] if entry.get("command") \
               else list(entry["arguments"])[1:]
        keep = []
        skip_next = False
        for i, a in enumerate(argv):
            if skip_next:
                skip_next = False
                continue
            # Drop output and compile-only flags; -fsyntax-only replaces them.
            if a in ("-o", "-c"):
                skip_next = a == "-o"
                continue
            if a == target or a.endswith(".o"):
                continue
            keep.append(a)
        print(shlex.join(keep))
        break
else:
    sys.exit(3)
PY
)"
    if [[ $? -ne 0 ]]; then
        echo "FAIL: ${rel} carries AESTRA_RT_NONBLOCKING but has no compile"
        echo "      command in ${DB}. It is annotated and unchecked, which is"
        echo "      exactly the gap this script exists to prevent."
        failed=1
        continue
    fi

    out="$(mktemp -t rt-effects-XXXXXX.log)"
    # shellcheck disable=SC2086
    if "$CXX" $args -Wfunction-effects -Werror=function-effects \
              -fsyntax-only "$abs" >"$out" 2>&1; then
        echo "  ok    ${rel}"
    else
        echo "  FAIL  ${rel}"
        sed 's/^/        /' "$out"
        failed=1
    fi
    rm -f "$out"
    checked=$((checked + 1))
done

echo
if [[ $failed -ne 0 ]]; then
    echo "RT effect check FAILED."
    echo
    echo "Each diagnostic above names a function that an audio-thread path"
    echo "reaches and the compiler cannot prove non-blocking. Three ways out,"
    echo "in order of preference:"
    echo
    echo "  1. Make it non-blocking. Usually the right answer on an RT path."
    echo "  2. Annotate it AESTRA_RT_NONBLOCKING at its DECLARATION, so callers"
    echo "     in other translation units see the contract too. 'no definition"
    echo "     in this translation unit' always means this."
    echo "  3. Move it off the RT path, and guard it with reportRealtimeMisuse."
    echo
    echo "Waiving the diagnostic is not on that list. If a waiver is genuinely"
    echo "correct, it belongs beside the argument that makes it correct — see"
    echo "the two in RealtimeThreadGuard.h for the standard that has to meet."
    exit 1
fi

echo "RT effect check passed — ${checked} translation unit(s) provably non-blocking."
exit 0
