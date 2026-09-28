@echo off
rem Historical inner half of build_d.bat, back when each P2 module carried its
rem own copy of the whole pipeline. build.bat now takes the slot directly, so
rem the split serves no purpose; this forwards unlogged, as it always did.
call "%~dp0build.bat" d
exit /b %ERRORLEVEL%
