; Install script for AirDC++
; This script is based on the DC++ install script, (c) 2001-2010 Jacek Sieka
; You need the ANSI version of the MoreInfo plugin installed in NSIS to be able to compile this script
; Its available from http://nsis.sourceforge.net/MoreInfo_plug-in

!include "MUI2.nsh"
!include "Sections.nsh"
!include "x64.nsh"
 

Var VERSION ; will be filled in onInit
Var TargetArch64 ; target architecture depending on the system and user's choice
Var OldInstDir ; existing istall dir (in case of reinstall)

!macro GetAirDCVersion
	; Get the DC++ version and store it in $VERSION
	GetDllVersionLocal "..\compiled\x64\AirDC.exe" $R0 $R1
	IntOp $R2 $R0 / 0x00010000
	IntOp $R3 $R0 & 0x0000FFFF
	IntOp $R4 $R1 / 0x00010000
	IntOp $R5 $R1 & 0x0000FFFF
	StrCpy $VERSION "$R2.$R3"
!macroend

SetCompressor "lzma"

; The name of the installer
Name "AirDC++ $VERSION"

ShowInstDetails show
ShowUninstDetails show

  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "airheader.bmp" ; optional
  !define MUI_ABORTWARNING

  !define MUI_COMPONENTSPAGE_NODESC
  
  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_TEXT "Launch AirDC++"
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
   
   
; The file to write
  OutFile "AirDC_Installer_2.60_r1746.exe"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\AirDC++ "Install_Dir"


; The text to prompt the user to enter a directory
ComponentText "Welcome to the AirDC++ installer."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install AirDC++ into. If you upgrade select the same directory where your old version resides and your existing settings will be imported."

; The stuff to install
Section "AirDC++ (required)" dcpp

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Check for existing settings in the profile first
  IfFileExists "$DOCUMENTS\AirDC++\*.xml" 0 check_programdir
  MessageBox MB_YESNO|MB_ICONQUESTION "Settings of a previous installation of AirDC++ has been found in the user profile, do you want to backup settings and queue? (You can find them in $DOCUMENTS\AirDC++\BACKUP later)" IDNO no_backup
  CreateDirectory "$DOCUMENTS\AirDC++\BACKUP\"
  CopyFiles "$DOCUMENTS\AirDC++\*.xml" "$DOCUMENTS\AirDC++\BACKUP\"
  goto no_backup

 check_programdir:
  ; Maybe we're upgrading from an older version so lets check the old Settings directory
  IfFileExists "$INSTDIR\Settings\*.xml" 0 check_dcpp
  MessageBox MB_YESNO|MB_ICONQUESTION "A previous installation of AirDC++ has been found in the target folder, do you want to backup settings and queue? (You can find them in $INSTDIR\BACKUP later)" IDNO no_backup
  CreateDirectory "$INSTDIR\BACKUP\"
  CopyFiles "$INSTDIR\Settings\*.xml" "$INSTDIR\BACKUP\"
  CopyFiles "$INSTDIR\Settings\*.dat" "$INSTDIR\BACKUP\"
  goto no_backup

 check_dcpp:
  ; Lets check the profile for possible DC++ settings to import
  IfFileExists "$APPDATA\DC++\*.xml" 0 check_apex
  MessageBox MB_YESNO|MB_ICONQUESTION "Settings of an existing DC++ installation has been found in the user profile, do you want to import settings and queue?" IDNO check_apex
  CreateDirectory "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\DC++\*.xml" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\DC++\*.dat" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\DC++\*.txt" "$DOCUMENTS\AirDC++\"
  goto no_backup
  
 check_apex:
  IfFileExists "$APPDATA\ApexDC++\*.xml" 0 check_strong
  MessageBox MB_YESNO|MB_ICONQUESTION "Settings of an existing ApexDC++ installation has been found in the user profile, do you want to import settings and queue?" IDNO check_strong
  CreateDirectory "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\ApexDC++\*.xml" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\ApexDC++\*.dat" "$DOCUMENTS\AirDC++\"
  CopyFiles "$APPDATA\ApexDC++\*.txt" "$DOCUMENTS\AirDC++\"
  goto no_backup
  
 check_strong:
  IfFileExists "$DOCUMENTS\StrongDC++\*.xml" 0 no_backup
  MessageBox MB_YESNO|MB_ICONQUESTION "Settings of an existing StrongDC++ installation has been found in the user profile, do you want to import settings and queue?" IDNO no_backup
  CreateDirectory "$DOCUMENTS\AirDC++\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.xml" "$DOCUMENTS\AirDC++\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.dat" "$DOCUMENTS\AirDC++\"
  CopyFiles "$DOCUMENTS\StrongDC++\*.txt" "$DOCUMENTS\AirDC++\"
  goto no_backup

no_backup:
  ; Put the files there
  File "dcppboot.xml"
  
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
  WriteRegStr HKLM SOFTWARE\AirDC++\ "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayIcon" '"$INSTDIR\AirDC.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayName" "AirDC++ $2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayVersion" "$2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "Publisher" "AirDC++ Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "URLInfoAbout" "http://www.airdcpp.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "URLUpdateInfo" "http://www.airdcpp.net/download"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "HelpLink" "http://www.airdcpp.net/guides"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "NoRepair" "1"
  
  WriteUninstaller "uninstall.exe"
SectionEnd

; optional section
Section "Start menu shortcuts"
  CreateDirectory "$SMPROGRAMS\AirDC++"
  CreateShortCut "$SMPROGRAMS\AirDC++\AirDC++.lnk" "$INSTDIR\AirDC.exe" "" "$INSTDIR\AirDC.exe" 0 "" "" "AirDC++ File Sharing Application"
  CreateShortCut "$SMPROGRAMS\AirDC++\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Radox Emoticon Pack"
  SetOutPath $INSTDIR
  File /r /x .svn EmoPacks
SectionEnd

Section "Themes"
  SetOutPath $INSTDIR
  File /r /x .svn Themes
SectionEnd

Section "Store settings in the user profile directory" loc
  ; Change to nonlocal dcppboot if the checkbox left checked
  SetOutPath $INSTDIR
  File /oname=dcppboot.xml "dcppboot.nonlocal.xml" 
SectionEnd

Section "Install the 32-bit version" arch
  ; Force to install 32 bit version on an x64 system
SectionEnd

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
  
  ; Do not show the warning on XP or older
  StrCmp $R8 "0" skip
  ; Show the warning only once
  StrCmp $R9 "1" skip
  SectionGetFlags ${loc} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} skip
    StrCpy $R9 "1"
    MessageBox MB_OK|MB_ICONEXCLAMATION "If you want to keep your settings in the program directory using Windows Vista or later make sure that you DO NOT install AirDC++ to the 'Program files' folder!!! This can lead to abnormal behaviour like loss of settings or downloads!"
skip:
FunctionEnd

Function .onInit
	; Set the system arch as default
	${If} ${RunningX64}
		StrCpy $TargetArch64 "1"
	${Else}
		StrCpy $TargetArch64 "0"
	${EndIf}

  IfFileExists "$EXEDIR\AirDC.exe" 0 checkos
  StrCpy $INSTDIR $EXEDIR
checkos:
  ; Check for Vista+
  ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  IfErrors xp_or_below nt
nt:
  StrCmp $R8 '6.0' vistaplus
  StrCmp $R8 '6.1' vistaplus
  StrCmp $R8 '6.2' vistaplus
  StrCmp $R8 '6.3' 0 xp_or_below
vistaplus:
  StrCpy $R8 "1"
  goto end
xp_or_below:
	MessageBox MB_OK|MB_ICONEXCLAMATION "This version is only compatible with Windows Vista or newer operating system. You will be forwarded to www.airdcpp.net for downloading a version that is compatible with your Windows version."
	ShellExecAsUser::ShellExecAsUser "open" 'http://www.airdcpp.net/download'
	Quit
end:
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
	; "Program Files/DC++".
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
  ShellExecAsUser::ShellExecAsUser "" "$INSTDIR\AirDC.exe"
FunctionEnd

; uninstall stuff

UninstallText "This will uninstall AirDC++. Hit the Uninstall button to continue."


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
  
  ; Remove registry entries
  ; Assume they are all registered to us
  DeleteRegKey HKCU "Software\Classes\dchub"
  DeleteRegKey HKCU "Software\Classes\adc"
  DeleteRegKey HKCU "Software\Classes\adcs"
  DeleteRegKey HKCU "Software\Classes\magnet"

   ;Start Menu Shortcuts
  Delete "$SMPROGRAMS\AirDC++\AirDC++.lnk" 
  Delete "$SMPROGRAMS\AirDC++\Uninstall.lnk" 
  RMDir /r "$SMPROGRAMS\AirDC++"

  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe

  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\AirDC++\*.*"
  ; remove directories used.
  RMDir "$SMPROGRAMS\AirDC++"
  ;IfFileExists "$INSTDIR\Settings\*.xml" 0 check_if_empty
  MessageBox MB_YESNO|MB_ICONQUESTION "Also remove queue, themes and all settings?" IDNO end_uninstall

  ; delete settings directory
  RMDir /r $INSTDIR\Settings
  RMDir /r "$LOCALAPPDATA\AirDC++\"
  RMDir /r "$DOCUMENTS\AirDC++\"
  
  RMDir /r $INSTDIR\Themes
  
  
  Push "$INSTDIR"
  Call un.isEmptyDir
  Pop $0
  StrCmp $0 0 0 +2
    MessageBox MB_YESNO|MB_ICONQUESTION  "Installation directory is NOT empty. Do you still want to remove it?" IDYES kill_whole_dir
  goto end_uninstall
	
kill_whole_dir:
	RMDir /r "$INSTDIR"
	
end_uninstall:
  RMDir "$INSTDIR"
SectionEnd

; eof
