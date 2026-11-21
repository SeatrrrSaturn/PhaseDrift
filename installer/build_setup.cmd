@echo off
setlocal
set TMP=D:\work\_tools\tmp
set TEMP=D:\work\_tools\tmp
set MAKENSIS=D:\work\_tools\nsis\nsis-3.11\makensis.exe
set OUTDIR=D:\work\PhaseDrift\dist

if not exist "%OUTDIR%" mkdir "%OUTDIR%"
if not exist "%MAKENSIS%" (
  echo NSIS not found: %MAKENSIS%
  echo Download nsis-3.11.zip from sourceforge and extract it to D:\work\_tools\nsis
  exit /b 1
)

"%MAKENSIS%" /V2 /INPUTCHARSET UTF8 "D:\work\PhaseDrift\installer\installer.nsi"
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo OK: %OUTDIR%\PhaseDrift-Setup.exe
