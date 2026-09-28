@echo off
rem Historical entry point from the P2 era, when four modules worked this tree
rem in parallel and each one needed its own build directory. The pipeline is no
rem longer duplicated here -- build.bat takes the slot as its first argument, so
rem this is a one line forward. Directory (build-b\) and log
rem (build-b.log) are unchanged.
rem
rem Kept rather than deleted because other sessions may still invoke this name.
rem New work does not need it: with no argument at all, build.bat gives each
rem agent session its own slot automatically.
call "%~dp0build_logged.bat" b
exit /b %ERRORLEVEL%
