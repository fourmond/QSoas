# -*- mode: nsis; -*-

Name "QSoas"

OutFile "QSoasSetup.exe"

Section "-default"
  SetOutPath $INSTDIR
  !include "dependencies.nsi"
  File "..\release\QSoas.exe"
SectionEnd

Page directory
Page instfiles
