@echo off
setlocal
set "TARGET=%~1"
if not defined TARGET set "TARGET=main"
if /I "%TARGET%"=="main" goto setup
if /I "%TARGET%"=="tests" goto setup
echo Usage: build.cmd [main^|tests]
exit /b 1

:setup
where cl.exe >nul 2>nul
if not errorlevel 1 goto build
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missing
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "LAB_VS=%%I"
if not defined LAB_VS goto missing
call "%LAB_VS%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1

:build
pushd "%~dp0"
if errorlevel 1 exit /b 1
if not exist build mkdir build
if errorlevel 1 goto failed
cl /nologo /std:c++17 /W4 /Zi /EHsc /utf-8 main.cpp /Fe:lab1.exe /Fd:build\lab1-compiler.pdb /link /PDB:build\lab1-linker.pdb /INCREMENTAL:NO
if errorlevel 1 goto failed
if /I "%TARGET%"=="main" goto done
cl /nologo /std:c++17 /W4 /Zi /EHsc /utf-8 tests.cpp /Fe:tests.exe /Fd:build\tests-compiler.pdb /link /PDB:build\tests-linker.pdb /INCREMENTAL:NO
if errorlevel 1 goto failed

:done
popd
exit /b 0

:failed
popd
exit /b 1

:missing
echo MSVC was not found. Install Visual Studio with Desktop development with C++.
exit /b 1
