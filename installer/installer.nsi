; Phase Drift - standard Windows installer (NSIS / Modern UI 2)
; Build: installer\build_setup.cmd

Unicode true
SetCompressor /SOLID lzma
SetCompressorDictSize 32

!include "MUI2.nsh"
!include "x64.nsh"

!define APPNAME      "Phase Drift"
!define APPVERSION  "0.9.0"
!define PUBLISHER   "Seater Saturn"
!define BUNDLE      "Phase Drift.vst3"
!define STAGEDIR    "D:\work\PhaseDrift\build\PhaseDrift_artefacts\Release\VST3"
!define LICFILE     "D:\work\PhaseDrift\LICENSE"
!define REGKEY      "Software\Microsoft\Windows\CurrentVersion\Uninstall\PhaseDrift"

Name "${APPNAME} ${APPVERSION}"
Caption "${APPNAME} ${APPVERSION} Setup"
BrandingText "${PUBLISHER}"
OutFile "D:\work\PhaseDrift\dist\PhaseDrift-Setup.exe"
InstallDir "$COMMONFILES64\VST3"
InstallDirRegKey HKLM "Software\${PUBLISHER}\PhaseDrift" "InstallDir"
RequestExecutionLevel admin
ShowInstDetails show
ShowUninstDetails show
XPStyle on

VIProductVersion "0.9.0.0"
VIAddVersionKey /LANG=1033 "ProductName"     "${APPNAME}"
VIAddVersionKey /LANG=1033 "CompanyName"     "${PUBLISHER}"
VIAddVersionKey /LANG=1033 "FileDescription" "${APPNAME} ${APPVERSION} Setup"
VIAddVersionKey /LANG=1033 "FileVersion"     "0.9.0.0"
VIAddVersionKey /LANG=1033 "LegalCopyright"  "AGPL-3.0 - Seater Saturn"
VIAddVersionKey /LANG=1033 "Comments"        "Experimental per-band phase + independent stereo echo, VST3 64-bit"

!define MUI_ABORTWARNING
!define MUI_ICON   "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT

; ---- pages --------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${LICFILE}"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

; ---- install ------------------------------------------------------------
Section "!Phase Drift VST3" SEC_MAIN
  SectionIn RO

  SetOutPath "$INSTDIR\${BUNDLE}\Contents\x86_64-win"
  File "${STAGEDIR}\${BUNDLE}\Contents\x86_64-win\${BUNDLE}"

  SetOutPath "$INSTDIR\${BUNDLE}\Contents"
  WriteUninstaller "$INSTDIR\${BUNDLE}\Contents\uninstall.exe"

  WriteRegStr HKLM "Software\${PUBLISHER}\PhaseDrift" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\${PUBLISHER}\PhaseDrift" "Version" "${APPVERSION}"

  WriteRegStr   HKLM "${REGKEY}" "DisplayName"     "${APPNAME} (VST3)"
  WriteRegStr   HKLM "${REGKEY}" "Publisher"       "${PUBLISHER}"
  WriteRegStr   HKLM "${REGKEY}" "DisplayVersion"  "${APPVERSION}"
  WriteRegStr   HKLM "${REGKEY}" "InstallLocation" "$INSTDIR\${BUNDLE}"
  WriteRegStr   HKLM "${REGKEY}" "DisplayIcon"     "$INSTDIR\${BUNDLE}\Contents\uninstall.exe"
  WriteRegStr   HKLM "${REGKEY}" "UninstallString" '"$INSTDIR\${BUNDLE}\Contents\uninstall.exe"'
  WriteRegStr   HKLM "${REGKEY}" "QuietUninstallString" '"$INSTDIR\${BUNDLE}\Contents\uninstall.exe" /S'
  WriteRegDWORD HKLM "${REGKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REGKEY}" "NoRepair" 1
SectionEnd

; ---- uninstall ----------------------------------------------------------
Section "Uninstall"
  RMDir /r "$INSTDIR\${BUNDLE}"

  DeleteRegKey HKLM "${REGKEY}"
  DeleteRegKey HKLM "Software\${PUBLISHER}\PhaseDrift"
  DeleteRegKey HKLM "Software\${PUBLISHER}"
SectionEnd
