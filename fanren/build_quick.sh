#!/usr/bin/env bash
# macOS / Linux counterpart of build_quick.bat: build + unit tests only, NO
# content gates.
#
# build.sh runs four content gates first (validate.py, genmaps.py --check,
# validate_selftest.py, artgen.py --check). The selftest alone can run for
# more than an hour, and the project owner ruled on 2026-11-20 that the gates
# are not run unless asked for. This script is the day-to-day loop instead:
# bootstrap -> configure -> build -> ctest -j <cores>.
#
#   ./build_quick.sh                     build into build/ (or the session slot)
#   ./build_quick.sh myslot              build into build-myslot/
#   ./build_quick.sh myslot notest       build only, skip ctest
#   ./build_quick.sh --log myslot        output captured to build-myslot.log,
#                                        last line EXITCODE=n
#
# Slot resolution matches build.sh / build_slot.bat: first argument, then
# FANREN_BUILD_SLOT, then the first 8 characters of CLAUDE_CODE_SESSION_ID,
# else no slot.
#
# Markers for log readers (same as build_quick.bat):
#   BUILD_DONE                    configure + build finished
#   CTEST_EXIT=<n>                ctest finished with exit code n (0 = all passed)
#   [build_quick] FAILED (<dir>)  a step failed; the script exits non-zero
#
# cmake / ninja installed with `pip install --user` land in
# ~/Library/Python/3.x/bin on macOS, which is usually not on PATH. If they are
# not found, the first such directory that has them is added to PATH for this
# run only.
set -uo pipefail
cd "$(dirname "$0")"

LOGGED=0
POSITIONAL=()
for arg in "$@"; do
    if [[ "$arg" == "--log" ]]; then
        LOGGED=1
    else
        POSITIONAL+=("$arg")
    fi
done
SLOT_ARG="${POSITIONAL[0]:-}"
MODE_ARG="${POSITIONAL[1]:-}"

SLOT="${SLOT_ARG:-${FANREN_BUILD_SLOT:-}}"
if [[ -z "$SLOT" && -n "${CLAUDE_CODE_SESSION_ID:-}" ]]; then
    SLOT="${CLAUDE_CODE_SESSION_ID:0:8}"
fi
BUILDDIR="build"
[[ -n "$SLOT" ]] && BUILDDIR="build-$SLOT"
BUILDLOG="$BUILDDIR.log"

NOTEST=0
[[ "$(printf '%s' "$MODE_ARG" | tr '[:upper:]' '[:lower:]')" == "notest" ]] && NOTEST=1

PYTHON="${PYTHON:-python3}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

find_tools() {
    if command -v cmake >/dev/null 2>&1 && command -v ninja >/dev/null 2>&1; then
        return 0
    fi
    local dir
    # Newest Python first; take the first user scripts directory that has both.
    for dir in $(ls -d "$HOME"/Library/Python/3.*/bin 2>/dev/null | sort -t. -k2 -n -r); do
        if [[ -x "$dir/cmake" && -x "$dir/ninja" ]]; then
            export PATH="$dir:$PATH"
            echo "[build_quick] cmake/ninja not on PATH; using $dir"
            return 0
        fi
    done
    echo "[build_quick] cmake 或 ninja 找不到：PATH 上没有，~/Library/Python/3.*/bin 里也没有。" \
         "先装上（例如 pip3 install --user cmake ninja），或把它们所在目录加进 PATH。"
    return 1
}

run_build() {
    find_tools || return 1
    "$PYTHON" bootstrap.py || return 1
    cmake -S . -B "$BUILDDIR" -G Ninja -DCMAKE_BUILD_TYPE=Release || return 1
    cmake --build "$BUILDDIR" --parallel || return 1
    echo "BUILD_DONE"
    if [[ "$NOTEST" == 1 ]]; then
        return 0
    fi
    ctest --test-dir "$BUILDDIR" -j "$JOBS" --output-on-failure
    local rc=$?
    echo "CTEST_EXIT=$rc"
    return "$rc"
}

run_and_mark() {
    run_build
    local rc=$?
    if [[ "$rc" != 0 ]]; then
        echo "[build_quick] FAILED ($BUILDDIR)"
    fi
    return "$rc"
}

if [[ "$LOGGED" == 1 ]]; then
    ( run_and_mark ) > "$BUILDLOG" 2>&1
    RC=$?
    echo "EXITCODE=$RC" >> "$BUILDLOG"
    [[ "$RC" != 0 ]] && echo "[build_quick] FAILED ($BUILDDIR)，详见 $BUILDLOG" >&2
else
    ( run_and_mark )
    RC=$?
fi
exit "$RC"
