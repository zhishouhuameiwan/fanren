@echo off
rem Build + unit tests only. NO content gates.
rem
rem build.bat runs four content gates first (validate.py, genmaps.py --check,
rem validate_selftest.py, artgen.py --check). The selftest alone can run for
rem more than an hour, and the project owner ruled on 2026-11-20 that the gates
rem are not run unless asked for. This script is the day-to-day loop instead:
rem bootstrap -> configure -> build -> ctest -j 4.
rem
rem Usage:   build_quick.bat [slot]        -> builds into build-<slot>\
rem          build_quick.bat [slot] notest -> build only, skip ctest
rem
rem Markers for log readers (cmd run from bash can report false success, so
rem judge by these lines, not by the exit code alone):
rem   BUILD_DONE        configure + build finished
rem   CTEST_EXIT=<n>    ctest finished with exit code n (0 = all passed)
rem
rem Visual Studio is located with vswhere, same as build.bat. If vswhere cannot
rem find it (some Build Tools installs), set VSPATH to the install root first:
rem   set "VSPATH=D:\Program Files\VS"
rem
rem Two Visual Studio installs on one machine: vswhere picks the latest, but a
rem build directory keeps the compiler it was first configured with. Mixing
rem them fails every file with "error STL1001: Unexpected compiler version".
rem Either set VSPATH to the install that directory was configured with, or
rem delete the build directory and let it configure afresh.
setlocal
pushd "%~dp0"

call "%~dp0build_slot.bat" %1

if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles%\CMake\bin;%PATH%"

python bootstrap.py
if errorlevel 1 goto failed

if defined VSPATH goto have_vs
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build_quick] vswhere.exe not found; set VSPATH to the Visual Studio install root.
    goto failed
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo [build_quick] No Visual Studio install with the C++ toolset was found; set VSPATH.
    goto failed
)

:have_vs
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto failed

cmake -S . -B "%BUILDDIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto failed

cmake --build "%BUILDDIR%" --parallel
if errorlevel 1 goto failed
echo BUILD_DONE

if /i "%~2"=="notest" (
    popd
    exit /b 0
)

ctest --test-dir "%BUILDDIR%" -j 4 --output-on-failure
set "RC=%ERRORLEVEL%"
echo CTEST_EXIT=%RC%
popd
exit /b %RC%

:failed
echo [build_quick] FAILED (%BUILDDIR%)
popd
exit /b 1
