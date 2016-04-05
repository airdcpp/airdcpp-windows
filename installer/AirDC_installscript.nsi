; Install script for AirDC++
; This script is based on the DC++ install script, (c) 2001-2010 Jacek Sieka
; You need the UNICODE version of NSIS to be able to compile this script
; Its available from https://code.google.com/p/unsis/downloads/list
;
; The following NSIS plugins are required for compiling this script:
; http://nsis.sourceforge.net/MoreInfo_plug-in
; http://nsis.sourceforge.net/ShellExecAsUser_plug-in

!include "MUI2.nsh"
!include "Sections.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"   ; for ${GetSize}
!include "WinVer.nsh"

!ifndef NSIS_UNICODE
  !error "An Unicode version of NSIS is required, see <http://code.google.com/p/unsis/>"
!endif

Var VERSION ; will be filled in onInit
Var TargetArch64 ; target architecture depending on the system and user's choice
Var OldInstDir ; existing install dir (in case of reinstall)
Var DOCBACKUPDIR ; Replaces the backup path for the profile folder
Var INSTBACKUPDIR ; Replaces the backup path for the program folder

!macro GetAirDCVersion
; Get the AirDC++ version and store it in $VERSION
  GetDllVersionLocal "..\compiled\x64\AirDC.exe" $R0 $R1
  IntOp $R2 $R0 / 0x00010000
  IntOp $R3 $R0 & 0x0000FFFF
  IntOp $R4 $R1 / 0x00010000
  IntOp $R5 $R1 & 0x0000FFFF
  StrCpy $VERSION "$R2.$R3"
!macroend

; To install in userprofile
RequestExecutionLevel admin

SetCompressor /SOLID "lzma"

; The name of the installer
Name "AirDC++ $VERSION"

; The file to write
!ifdef GIT_VERSION
OutFile '..\releases\${GIT_VERSION}\AirDC_Installer_${GIT_VERSION}.exe'
!else
OutFile AirDC_Installer_XXX.exe
!endif


ShowInstDetails show
ShowUninstDetails show

!define MUI_ICON "Install.ico"
!define MUI_UNICON "Uninstall.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "airheader.bmp" ; optional
!define MUI_ABORTWARNING
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
!define MUI_LANGDLL_REGISTRY_KEY "SOFTWARE\AirDC++"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Install_Language"
!define MUI_FINISHPAGE_NOAUTOCLOSE ; Pause after installation

;!define MUI_COMPONENTSPAGE_NODESC ; Added desc so this is not in use

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "$(RunAtFinish)"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

;Page components
;Page directory
;Page instfiles
;UninstPage uninstConfirm
;UninstPage instfiles
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

  !insertmacro MUI_LANGUAGE "English"

!include "all_language.nsh"

; Beginning of English strings
LangString RunAtFinish ${LANG_ENGLISH} "Launch AirDC++"
LangString ^ComponentsText ${LANG_ENGLISH} "Welcome to the AirDC++ installer."
LangString ^DirText ${LANG_ENGLISH} "If you are upgrading from an older version, select the same directory where your old version resides and your existing settings will be imported."
LangString SecAirDC ${LANG_ENGLISH} "AirDC++ (required)"
LangString ProfileDir ${LANG_ENGLISH} "Settings of a previous installation of AirDC++ has been found in the user profile, do you want to backup settings? (You can find them in $DOCBACKUPDIR later)"
LangString ProgramDir ${LANG_ENGLISH} "A previous installation of AirDC++ has been found in the target folder, do you want to backup settings? (You can find them in $INSTBACKUPDIR later)"
LangString Dcpp ${LANG_ENGLISH} "Settings of an existing DC++ installation has been found in the user profile, do you want to import settings and queue?"
LangString Apex ${LANG_ENGLISH} "Settings of an existing ApexDC++ installation has been found in the user profile, do you want to import settings and queue?"
LangString Strong ${LANG_ENGLISH} "Settings of an existing StrongDC++ installation has been found in the user profile, do you want to import settings and queue?"
LangString SecStartMenu ${LANG_ENGLISH} "Start menu shortcuts"
LangString SM_AIRDCPP_DESC ${LANG_ENGLISH} "AirDC++ File Sharing Application"
LangString SM_AIRDCPP_DESCUN ${LANG_ENGLISH} "Uninstall AirDC++"
LangString SM_UNINSTALL ${LANG_ENGLISH} "Uninstall"
LangString WR_REMOVEONLY ${LANG_ENGLISH} "(remove only)"
LangString SecRadox ${LANG_ENGLISH} "Radox Emoticon Pack"
LangString SecThemes ${LANG_ENGLISH} "Themes"
LangString SecWebResources ${LANG_ENGLISH} "Web user interface"
LangString SecStore ${LANG_ENGLISH} "Store settings in the user profile directory"
LangString DeskShort ${LANG_ENGLISH} "Create AirDC++ desktop shourtcut"
LangString SecInstall32 ${LANG_ENGLISH} "Install the 32-bit version"
LangString XPOrOlder ${LANG_ENGLISH} "If you want to keep your settings in the program directory, make sure that you DO NOT install AirDC++ to the 'Program files' folder!!! This can lead to abnormal behaviour like loss of settings or downloads!"
LangString XPOrBelow ${LANG_ENGLISH} "This version is only compatible with Windows Vista or newer operating system. You will be forwarded to www.airdcpp.net for downloading a version that is compatible with your Windows version."
LangString ^UninstallText ${LANG_ENGLISH} "This will uninstall AirDC++. Hit the Uninstall button to continue."
LangString RemoveQueue ${LANG_ENGLISH} "Do you also want to remove queue, themes and all settings?"
LangString NotEmpty ${LANG_ENGLISH} "Installation directory is NOT empty. Do you still want to remove it?"
LangString DESC_dcpp ${LANG_ENGLISH} "AirDC++ main program."
LangString DESC_StartMenu ${LANG_ENGLISH} "A shortcut of AirDC++ will be placed in your start menu."
LangString DESC_Radox ${LANG_ENGLISH} "RadoX emoticon pack will be installed to be used in chats."
LangString DESC_Themes ${LANG_ENGLISH} "If you don't like the default theme you can try these two extra themes, Dark Skull and Zoolution."
LangString DESC_WebResorces ${LANG_ENGLISH} "Allows accessing the client from other devices via a web browser."
LangString DESC_loc ${LANG_ENGLISH} "Normally you should not change this because it can lead to abnormal behaviour like loss of settings or downloads. If you unselect it, read the warning message carefully."
LangString DESC_desk ${LANG_ENGLISH} "Do you often run AirDC++? Then this is definitely something for you!"
LangString DESC_arch ${LANG_ENGLISH} "If you want to use the 32-bits version of AirDC++ on your 64-bits operating system you need to select this. If this is selected and grayed out you can not unselect it because your operation system is 32-bits."
;End of English strings

; Registry key to check for directory (so if you install again, it will overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\AirDC++ "Install_Dir"

; The stuff to install
Section "$(SecAirDC)" dcpp

; Set output path to the installation directory.
  SetOutPath $INSTDIR

; Check for existing settings in the profile first
  StrCpy $DOCBACKUPDIR "$DOCUMENTS\AirDC++\BACKUP"
  IfFileExists "$DOCUMENTS\AirDC++\*.xml" 0 check_programdir
  MessageBox MB_YESNO|MB_ICONQUESTION $(ProfileDir) IDNO no_backup
  CreateDirectory "$DOCUMENTS\AirDC++\BACKUP\"
  CopyFiles "$DOCUMENTS\AirDC++\*.xml" "$DOCUMENTS\AirDC++\BACKUP\"
  IfFileExists "$DOCUMENTS\AirDC++\*.dat" 0 +2
  CopyFiles "$DOCUMENTS\AirDC++\*.dat" "$DOCUMENTS\AirDC++\BACKUP\"
  IfFileExists "$DOCUMENTS\AirDC++\*.txt" 0 no_backup
  CopyFiles "$DOCUMENTS\AirDC++\*.txt" "$DOCUMENTS\AirDC++\BACKUP\"
  goto no_backup

check_programdir:
; Maybe we're upgrading from an older version so lets check the old Settings directory
  StrCpy $INSTBACKUPDIR "$INSTDIR\BACKUP"
  IfFileExists "$INSTDIR\Settings\*.xml" 0 check_dcpp
  MessageBox MB_YESNO|MB_ICONQUESTION $(ProgramDir) IDNO no_backup
  CreateDirectory "$INSTDIR\BACKUP\"
  CopyFiles "$INSTDIR\Settings\*.xml" "$INSTDIR\BACKUP\"
  IfFileExists "$INSTDIR\Settings\*.dat" 0 +2
  CopyFiles "$INSTDIR\Settings\*.dat" "$INSTDIR\BACKUP\"
  IfFileExists "$INSTDIR\Settings\*.txt" 0 no_backup
  CopyFiles "$INSTDIR\Settings\*.txt" "$INSTDIR\BACKUP\"
  goto no_backup

check_dcpp:
; Lets check the profile for possible DC++ settings to import
  IfFileExists "$APPDATA\DC++\*.xml" 0 check_apex
  MessageBox MB_YESNO|MB_ICONQUESTION $(Dcpp) IDNO check_apex
  CreateDirectory "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\DC++\*.xml" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\DC++\*.dat" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\DC++\*.txt" "$DOCUMENTS\AirDC++\"
  goto no_backup

check_apex:
  IfFileExists "$APPDATA\ApexDC++\*.xml" 0 check_strong
  MessageBox MB_YESNO|MB_ICONQUESTION $(Apex) IDNO check_strong
  CreateDirectory "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\ApexDC++\*.xml" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\ApexDC++\*.dat" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\ApexDC++\*.txt" "$DOCUMENTS\AirDC++\"
  goto no_backup

check_strong:
  IfFileExists "$DOCUMENTS\StrongDC++\*.xml" 0 no_backup
  MessageBox MB_YESNO|MB_ICONQUESTION $(Strong) IDNO no_backup
  CreateDirectory "$DOCUMENTS\AirDC++\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.xml" "$DOCUMENTS\AirDC++\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.dat" "$DOCUMENTS\AirDC++\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.txt" "$DOCUMENTS\AirDC++\"
  goto no_backup

no_backup:
; Put the files there
  File "dcppboot.xml"
  File "popup.bmp"

  ${If} $TargetArch64 == "1"
    File "..\compiled\x64\AirDC.pdb"
    File "..\compiled\x64\AirDC.exe"
  ${Else}
  File "..\compiled\Win32\AirDC.pdb"
  File "..\compiled\Win32\AirDC.exe"
  ${EndIf}
  ; Uncomment to get DCPP core version from AirDC.exe we just installed and store in $1
  ;Function GetDCPlusPlusVersion
  ;        Exch $0;
  ;	GetDllVersion "$INSTDIR\$0" $R0 $R1
  ;	IntOp $R2 $R0 / 0x00010000
  ;	IntOp $R3 $R0 & 0x0000FFFF
  ;	IntOp $R4 $R1 / 0x00010000
  ;	IntOp $R5 $R1 & 0x0000FFFF
  ;        StrCpy $1 "$R2.$R3$R4$R5"
  ;        Exch $1
  ;FunctionEnd

  ; Push "AirDC.exe"
  ; Call "GetDCPlusPlusVersion"
  ; Pop $1

; Get AirDC version we just installed and store in $2
  MoreInfo::GetProductVersion "$INSTDIR\AirDC.exe"
  Pop $2

; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\AirDC++ "Install_Dir" "$INSTDIR"
; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayIcon" '"$INSTDIR\AirDC.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayName" "AirDC++ $2 $(WR_REMOVEONLY)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayVersion" "$2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "Publisher" "AirDC++ Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "URLInfoAbout" "http://www.airdcpp.net"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "URLUpdateInfo" "http://www.airdcpp.net/download"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "HelpLink" "http://www.airdcpp.net/guides"
   ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
   IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "EstimatedSize" "$0"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "NoRepair" "1"

  WriteUninstaller "uninstall.exe"

; Add AirDC.exe to registry 'App Path'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\AirDC.exe" "" "$INSTDIR\AirDC.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\AirDC.exe" "Path" "$INSTDIR"

SectionEnd

; optional section
Section "$(SecStartMenu)" DescStartMenu
  CreateDirectory "$SMPROGRAMS\AirDC++\"
  CreateShortCut "$SMPROGRAMS\AirDC++\AirDC++.lnk" "$INSTDIR\AirDC.exe" "" "$INSTDIR\AirDC.exe" 0 "" "" "$(SM_AIRDCPP_DESC)"
  CreateShortCut "$SMPROGRAMS\AirDC++\$(SM_UNINSTALL).lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0 "" "" "$(SM_AIRDCPP_DESCUN)"
SectionEnd

Section "$(SecRadox)" DescRadox
  SetOutPath $INSTDIR
  File /r EmoPacks
SectionEnd

Section "$(SecThemes)" DescThemes
  SetOutPath $INSTDIR
  File /r Themes
SectionEnd

Section "$(SecWebResources)" DescWebResorces
  SetOutPath $INSTDIR
  File /r Web-resources
SectionEnd

Section "$(SecStore)" loc
; Change to nonlocal dcppboot if the checkbox left checked
  SetOutPath $INSTDIR
  File /oname=dcppboot.xml "dcppboot.nonlocal.xml"
SectionEnd

Section "$(DeskShort)" desk
  SetShellVarContext current
  CreateShortCut "$DESKTOP\AirDC++.lnk" "$INSTDIR\AirDC.exe" "" "$INSTDIR\AirDC.exe" 0 "" "" "$(SM_AIRDCPP_DESC)"
SectionEnd

Section "$(SecInstall32)" arch
; Force to install 32 bit version on an x64 system
SectionEnd

;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${dcpp} $(DESC_dcpp)
    !insertmacro MUI_DESCRIPTION_TEXT ${DescStartMenu} $(DESC_StartMenu)
    !insertmacro MUI_DESCRIPTION_TEXT ${DescRadox} $(DESC_Radox)
    !insertmacro MUI_DESCRIPTION_TEXT ${DescThemes} $(DESC_Themes)
    !insertmacro MUI_DESCRIPTION_TEXT ${DescWebResorces} $(DESC_WebResorces)
    !insertmacro MUI_DESCRIPTION_TEXT ${loc} $(DESC_loc)
    !insertmacro MUI_DESCRIPTION_TEXT ${desk} $(DESC_desk)
    !insertmacro MUI_DESCRIPTION_TEXT ${arch} $(DESC_arch)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

Function .onSelChange
; Alter default value for $INSTDIR according to the arch selection
; but only if it's the first install and we're on an x64 system
${If} ${RunningX64}
  SectionGetFlags ${arch} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} x86
  ${If} $OldInstDir == ""
    StrCpy $INSTDIR "$PROGRAMFILES64\AirDC++"
  ${EndIf}
  StrCpy $TargetArch64 "1"
  goto end
x86:
  ${If} $OldInstDir == ""
    StrCpy $INSTDIR "$PROGRAMFILES\AirDC++"
  ${EndIf}
  StrCpy $TargetArch64 "0"
end:
${EndIf}

; Show the warning only once
  StrCmp $R9 "1" skip
  SectionGetFlags ${loc} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} skip
  StrCpy $R9 "1"
  MessageBox MB_OK|MB_ICONEXCLAMATION $(XPOrOlder)
skip:
FunctionEnd

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY

; Set the system arch as default
  ${If} ${RunningX64}
    StrCpy $TargetArch64 "1"
  ${Else}
    StrCpy $TargetArch64 "0"
  ${EndIf}

  IfFileExists "$EXEDIR\AirDC.exe" 0 checkos
  StrCpy $INSTDIR $EXEDIR
checkos:
  ${IfNot} ${AtLeastWinVista}
    MessageBox MB_OK|MB_ICONEXCLAMATION $(XPOrBelow)
    Exec '"$WINDIR\explorer.exe" "http://www.airdcpp.net/download"'
    Quit
  ${EndIf}

; Set the program component really required (read only)
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${dcpp} $0

; Require the 32 bit arch option on x86 systems, deselect on x64
  ${If} $TargetArch64 == "0"
    SectionSetFlags ${arch} $0
  ${Else}
    SectionGetFlags ${arch} $0
    IntOp $0 $0 & ${SECTION_OFF}
    SectionSetFlags ${arch} $0
  ${EndIf}

; Save existing install dir to be able to compare later
  StrCpy $OldInstDir $INSTDIR

; When there is no install dir (ie when this is the first install), default to
; "Program Files/AirDC++".
  ${If} $INSTDIR == ""
    ${If} $TargetArch64 == "1"
      StrCpy $INSTDIR "$PROGRAMFILES64\AirDC++"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\AirDC++"
    ${EndIf}
  ${EndIf}

  !insertmacro GetAirDCVersion
FunctionEnd

Function LaunchLink
Exec '"$WINDIR\explorer.exe" "$INSTDIR\AirDC.exe"'
FunctionEnd

; uninstall stuff
Function un.isEmptyDir
  # Stack ->                    # Stack: <directory>
  Exch $0                       # Stack: $0
  Push $1                       # Stack: $1, $0
  FindFirst $0 $1 "$0\*.*"
  strcmp $1 "." 0 _notempty
    FindNext $0 $1
    strcmp $1 ".." 0 _notempty
      ClearErrors
      FindNext $0 $1
      IfErrors 0 _notempty
        FindClose $0
        Pop $1                  # Stack: $0
        StrCpy $0 1
        Exch $0                 # Stack: 1 (true)
        goto _end
     _notempty:
       FindClose $0
       ClearErrors
       Pop $1                   # Stack: $0
       StrCpy $0 0
       Exch $0                  # Stack: 0 (false)
  _end:
FunctionEnd

Function un.onInit

!insertmacro MUI_UNGETLANGUAGE

FunctionEnd

; special uninstall section.
Section "un.Uninstall"

; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++"
  DeleteRegKey HKLM SOFTWARE\AirDC++

; remove files
  Delete "$INSTDIR\popup.bmp"
  Delete "$INSTDIR\AirDC.pdb"
  Delete "$INSTDIR\AirDC.exe"
  Delete "$INSTDIR\dcppboot.xml"

; remove EmoPacks dir
  RMDir /r "$INSTDIR\EmoPacks"
  RMDir /r "$INSTDIR\icons"

; remove Web-resources dir
  RMDir /r "$INSTDIR\Web-resources"

; Remove registry entries
; Assume they are all registered to us
  DeleteRegKey HKCU "Software\Classes\dchub"
  DeleteRegKey HKCU "Software\Classes\adc"
  DeleteRegKey HKCU "Software\Classes\adcs"
  DeleteRegKey HKCU "Software\Classes\magnet"

; Remove registry keys 'App Path'
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\AirDC.exe"

; Start Menu Shortcuts
  Delete "$SMPROGRAMS\AirDC++\AirDC++.lnk" 
  Delete "$SMPROGRAMS\AirDC++\$(SM_UNINSTALL).lnk" 
  RMDir /r "$SMPROGRAMS\AirDC++"

; Desktop Shortcut
  Delete "$DESKTOP\AirDC++.lnk"

; Must Remove Uninstaller, too
  Delete $INSTDIR\uninstall.exe

; remove shortcuts, if any.
  Delete "$SMPROGRAMS\AirDC++\*.*"

; remove directories used.
  RMDir "$SMPROGRAMS\AirDC++"

; IfFileExists "$INSTDIR\Settings\*.xml" 0 check_if_empty
  MessageBox MB_YESNO|MB_ICONQUESTION $(RemoveQueue) IDNO end_uninstall

; delete settings directory
  RMDir /r $INSTDIR\Settings
  RMDir /r "$LOCALAPPDATA\AirDC++\"
  RMDir /r "$DOCUMENTS\AirDC++\"
  RMDir /r $INSTDIR\Themes
  Push "$INSTDIR"
  Call un.isEmptyDir
  Pop $0
  StrCmp $0 0 0 +2
    MessageBox MB_YESNO|MB_ICONQUESTION  $(NotEmpty) IDYES kill_whole_dir
  goto end_uninstall

kill_whole_dir:
  RMDir /r "$INSTDIR"

end_uninstall:
  RMDir "$INSTDIR"
SectionEnd

; eof
