@echo off
rem Resolve this session's build slot; exports BUILDDIR and BUILDLOG.
rem
rem Several agents work this one source tree at the same time. Sharing a single
rem build\ makes them fight over ninja's lock and over each other's objects and
rem executables -- that is tech debt G-1, and it has cost real time: a LNK1104
rem on a locked fanren_tests.exe and a phantom compile error from a file that
rem was being saved mid-build, both inside ten minutes.
rem
rem Slot resolution, first match wins:
rem   1. first argument           build.bat myslot  -> build-myslot\
rem   2. FANREN_BUILD_SLOT        set once per shell
rem   3. CLAUDE_CODE_SESSION_ID   first 8 chars -- an agent session isolates
rem                               itself with no slot to claim. The older
rem                               build_a..d.bat scheme is unused (its four
rem                               directories went cold within hours) precisely
rem                               because it made a human pick a letter.
rem   4. nothing                  build\ -- unchanged default for humans
rem
rem Deliberately no setlocal: exporting those two variables IS this script's job.
rem Callers are expected to setlocal themselves.
set "SLOT=%~1"
if not defined SLOT set "SLOT=%FANREN_BUILD_SLOT%"
if not defined SLOT if defined CLAUDE_CODE_SESSION_ID set "SLOT=%CLAUDE_CODE_SESSION_ID:~0,8%"

set "BUILDDIR=build"
if defined SLOT set "BUILDDIR=build-%SLOT%"
set "BUILDLOG=%BUILDDIR%.log"
exit /b 0
