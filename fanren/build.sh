#!/usr/bin/env bash
# macOS / Linux counterpart of build.bat (+ build_slot.bat, build_logged.bat).
#
# Same four steps in the same order: content gates first (python only, seconds
# instead of a full build), bootstrap.py (vendored deps; on non-Windows it also
# builds SDL3 and its satellites from their pinned source releases), CMake +
# Ninja, then ctest. The reasons behind that order live in build.bat.
#
#   ./build.sh                 build into build/
#   ./build.sh myslot          build into build-myslot/
#   ./build.sh --log [slot]    same, output captured to build[-slot].log, last line EXITCODE=n
#
# Slot resolution matches build_slot.bat: argument, then FANREN_BUILD_SLOT,
# then the first 8 characters of CLAUDE_CODE_SESSION_ID, else no slot.
#
# Needs python3 (numpy, Pillow, scipy), cmake >= 3.25, ninja and a C++20
# compiler. If cmake/ninja came from `pip install --user`, put the user
# scripts directory on PATH (macOS: ~/Library/Python/3.x/bin).
set -uo pipefail
cd "$(dirname "$0")"

LOGGED=0
SLOT_ARG=""
for arg in "$@"; do
    if [[ "$arg" == "--log" ]]; then
        LOGGED=1
    else
        SLOT_ARG="$arg"
    fi
done

SLOT="${SLOT_ARG:-${FANREN_BUILD_SLOT:-}}"
if [[ -z "$SLOT" && -n "${CLAUDE_CODE_SESSION_ID:-}" ]]; then
    SLOT="${CLAUDE_CODE_SESSION_ID:0:8}"
fi
BUILDDIR="build"
[[ -n "$SLOT" ]] && BUILDDIR="build-$SLOT"
BUILDLOG="$BUILDDIR.log"

PYTHON="${PYTHON:-python3}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

run_build() {
    set -e
    "$PYTHON" tools/validate.py
    "$PYTHON" tools/mapgen/genmaps.py --check
    "$PYTHON" tools/validate_selftest.py
    "$PYTHON" tools/artgen/artgen.py --check
    "$PYTHON" bootstrap.py
    cmake -S . -B "$BUILDDIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILDDIR" --parallel
    # Unlike build.bat this runs tests in parallel (about 75 s instead of
    # several minutes); every test that touches disk uses its own temp dir.
    ctest --test-dir "$BUILDDIR" --output-on-failure -j "$JOBS"
}

if [[ "$LOGGED" == 1 ]]; then
    ( run_build ) > "$BUILDLOG" 2>&1
    RC=$?
    echo "EXITCODE=$RC" >> "$BUILDLOG"
else
    ( run_build )
    RC=$?
fi

if [[ "$RC" != 0 ]]; then
    echo "[build] FAILED ($BUILDDIR)" >&2
fi
exit "$RC"
