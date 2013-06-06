# -*- mode: nsis; -*-

RequestExecutionLevel admin ; we want administrator rights, don't we ?

Name "QSoas"

OutFile "QSoasSetup.exe"

InstallDir "$PROGRAMFILES\QSoas"
XPStyle on

Section "-default"
  SetOutPath $INSTDIR
  !include "dependencies.nsi"
  File "..\release\QSoas.exe"
  CreateDirectory "$SMPROGRAMS\QSoas"
  CreateShortCut "$SMPROGRAMS\QSoas\QSoas.lnk" "$INSTDIR\QSoas.exe"
  CreateShortCut "$DESKTOP\QSoas.lnk" "$INSTDIR\QSoas.exe"
SectionEnd

Page directory
Page instfiles

