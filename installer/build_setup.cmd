@echo off
setlocal
set TMP=D:\work\_tools\tmp
set TEMP=D:\work\_tools\tmp
set CSC=C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe
set SRC=D:\work\PhaseDrift\build\PhaseDrift_artefacts\Release\VST3\Phase Drift.vst3\Contents\x86_64-win\Phase Drift.vst3
set OUTDIR=D:\work\PhaseDrift\dist

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

"%CSC%" /nologo /target:winexe /platform:x64 /optimize+ ^
  /win32manifest:"D:\work\PhaseDrift\installer\setup.manifest" ^
  /resource:"%SRC%",PhaseDrift.dll.bin ^
  /out:"%OUTDIR%\PhaseDrift-Setup.exe" ^
  "D:\work\PhaseDrift\installer\Setup.cs"

if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo OK: %OUTDIR%\PhaseDrift-Setup.exe
