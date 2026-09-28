@echo off
rem Wrapper: runs build.bat with all output captured to this slot's log so the
rem agent harness can tail progress. Not part of the normal workflow.
rem
rem The log name follows the build directory: build.log for the default slot,
rem build-<slot>.log otherwise. build_slot.bat is called here purely to learn
rem that name; build.bat calls it again to resolve the same slot for itself.
setlocal
call "%~dp0build_slot.bat" %1
call "%~dp0build.bat" %* > "%~dp0%BUILDLOG%" 2>&1
set "RC=%ERRORLEVEL%"
echo EXITCODE=%RC% >> "%~dp0%BUILDLOG%"
exit /b %RC%
