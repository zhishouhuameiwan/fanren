@echo off
rem Configure and build the game, then run the unit tests.
rem
rem The MSVC environment is located with vswhere rather than a hardcoded path,
rem so this works across VS editions and install locations (the P0 probe
rem hardcoded "VS 18 Community" and would break on any other machine).
rem
rem The build directory is per-slot so parallel agents stop trampling each
rem other; build_slot.bat explains how the slot is chosen. No slot means
rem build\ exactly as before. Override explicitly with:  build.bat myslot
setlocal
pushd "%~dp0"

call "%~dp0build_slot.bat" %1

rem The content gates run FIRST. They need nothing but python and the repo --
rem no vendored deps, no toolchain, no network -- so a broken map costs seconds
rem instead of a full MSVC build plus 509 tests. They also used to sit last,
rem which meant an unrelated ctest flake skipped them entirely: the gate that
rem only runs when everything else already passed is not a gate.
rem Content gate: data / map / script / text reference integrity.
rem Rules live in docs/map_spec.md section 7; a broken map must fail the build,
rem not surface later as a player walking into an unreachable NPC.
python tools/validate.py
if errorlevel 1 goto failed

rem The generator is the single source of truth for maps/ch01_*.tmj. Drift is
rem silent -- wiping a gate property makes no tool complain -- so "would a
rem regeneration change maps/?" is itself a gate.
python tools/mapgen/genmaps.py --check
if errorlevel 1 goto failed

rem Negative self-test for the two gates above: deliberately break content in a
rem temp copy and require them to catch it. A check that has only ever seen
rem valid input is not a check; this project has already shipped one validator
rem whose regex was silently eaten and which passed everything for weeks.
python tools/validate_selftest.py
if errorlevel 1 goto failed

rem Art gate: assets/art/** are generator products (tools/artgen), exactly as
rem maps/*.tmj are products of tools/mapgen. Drift between the generator and
rem the committed PNGs is silent -- the game would simply render yesterday's
rem art -- so regenerate into a temp dir and compare pixel by pixel
rem (docs/octopath-overhaul.md section 1.2, docs/art-maps.md).
python tools/artgen/artgen.py --check
if errorlevel 1 goto failed

python bootstrap.py
if errorlevel 1 goto failed

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build] vswhere.exe not found; is Visual Studio installed?
    goto failed
)

set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo [build] No Visual Studio install with the C++ toolset was found.
    goto failed
)

call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto failed

cmake -S . -B "%BUILDDIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto failed

cmake --build "%BUILDDIR%" --parallel
if errorlevel 1 goto failed

ctest --test-dir "%BUILDDIR%" --output-on-failure
if errorlevel 1 goto failed

popd
exit /b 0

:failed
echo [build] FAILED (%BUILDDIR%)
popd
exit /b 1
