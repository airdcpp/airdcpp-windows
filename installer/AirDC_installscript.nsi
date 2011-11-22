; Install script for AirDC++
; This script is based on the DC++ install script, (c) 2001-2010 Jacek Sieka
; You need the ANSI version of the MoreInfo plugin installed in NSIS to be able to compile this script
; Its available from http://nsis.sourceforge.net/MoreInfo_plug-in

; Uncomment the above line if you want to build installer for the 64-bit version
; !define X64

;!include "Sections.nsh"
 !include "MUI2.nsh"

!define X64
 
SetCompressor "lzma"

; The name of the installer
Name "AirDC++"

ShowInstDetails show
ShowUninstDetails show

  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\nsis.bmp" ; optional
  !define MUI_ABORTWARNING
  
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
!ifdef X64
  OutFile "AirDC_Installer64.exe"
!else
  OutFile "AirDC_Installer.exe"
!endif

; The default installation directory
!ifdef X64
  InstallDir "$PROGRAMFILES64\AirDC++"
!else
  InstallDir "$PROGRAMFILES\AirDC++"
!endif

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\AirDC++ "Install_Dir"


; The text to prompt the user to enter a directory
ComponentText "Welcome to the AirDC++ installer."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install AirDC++ into. If you upgrade select the same directory where your old version resides and your existing settings will be imported."

; The stuff to install
Section "AirDC++ (required)" dcpp

!ifdef X64
  ;Check if we have a valid PROGRAMFILES64 enviroment variable
  StrCmp $PROGRAMFILES64 $PROGRAMFILES32 0 _64_ok
  MessageBox MB_YESNO|MB_ICONEXCLAMATION "This installer will try to install the 64-bit version of AirDC++ but this doesn't seem to be 64-bit operating system. Do you want to continue?" IDYES _64_ok
  Quit
  _64_ok:
!endif

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Check for existing settings in the profile first
  ;IfFileExists "$DOCUMENTS\AirDC++\*.xml" 0 check_programdir
  ;MessageBox MB_YESNO|MB_ICONQUESTION "Settings of a previous installation of AirDC++ has been found in the user profile, do you want to backup settings and queue? (You can find them in $DOCUMENTS\AirDC++\BACKUP later)" IDNO no_backup
  ;CreateDirectory "$DOCUMENTS\AirDC++\BACKUP\"
  ;CopyFiles "$DOCUMENTS\AirDC++\*.xml" "$DOCUMENTS\AirDC++\BACKUP\"
  ;goto no_backup

  ; Maybe we're upgrading from an older version so lets check the old Settings directory
  IfFileExists "$INSTDIR\Settings\*.xml" 0 no_backup
  MessageBox MB_YESNO|MB_ICONQUESTION "A previous installation of AirDC++ has been found in the target folder, do you want to backup settings and queue? (You can find them in $INSTDIR\BACKUP later)" IDNO no_backup
  CreateDirectory "$INSTDIR\BACKUP\"
  CopyFiles "$INSTDIR\Settings\*.xml" "$INSTDIR\BACKUP\"
  CopyFiles "$INSTDIR\Settings\*.dat" "$INSTDIR\BACKUP\"

;check_dcpp:
  ; Lets check the profile for possible DC++ settings to import
 ; IfFileExists "$APPDATA\DC++\*.xml" 0 no_backup
  ;MessageBox MB_YESNO|MB_ICONQUESTION "Settings of an existing DC++ installation has been found in the user profile, do you want to import settings and queue?" IDNO no_backup
  ;CreateDirectory "$DOCUMENTS\AirDC++\"
  ;CopyFiles "$APPDATA\DC++\*.xml" "$DOCUMENTS\AirDC++\"
  ;CopyFiles "$APPDATA\DC++\*.dat" "$DOCUMENTS\AirDC++\"
  ;CopyFiles "$APPDATA\DC++\*.txt" "$DOCUMENTS\AirDC++\"

no_backup:
  ; Put file there
  File "popup.bmp"
  !ifdef X64
	File "..\compiled\x64\AirDC.pdb"
	File "..\compiled\x64\AirDC.exe"
  !else
	File "..\compiled\Win32\AirDC.pdb"
	File "..\compiled\Win32\AirDC.exe"
  !endif
  File /r /x .svn icons
  
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
  ;MoreInfo::GetProductVersion "$INSTDIR\AirDC.exe"
  ;Pop $2
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\AirDC++ "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayIcon" '"$INSTDIR\AirDC.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayName" "AirDC++ $2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "DisplayVersion" "$2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "Publisher" "AirDC++ Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "URLInfoAbout" "http://www.airdcpp.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "URLUpdateInfo" "http://www.airdcpp.net/downloads.html"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "HelpLink" "http://www.airdcpp.net/Guides/guides.html"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AirDC++" "NoRepair" "1"
  
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "IP -> Country mappings"
  SetOutPath $INSTDIR
  File "GeoIPCountryWhois.csv"
SectionEnd

; optional section
Section "Start Menu Shortcuts"
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

Section "Language Translations" 
  SetOutPath $INSTDIR
  File /r /x .svn Language
SectionEnd

Function .onInit
  IfFileExists "$EXEDIR\AirDC.exe" 0 checkos
  StrCpy $INSTDIR $EXEDIR
checkos:
  ; Check for Vista+
  ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  IfErrors xp_or_below nt
nt:
  StrCmp $R8 '6.0' vistaplus
  StrCmp $R8 '6.1' 0 xp_or_below
vistaplus:
  StrCpy $R8 "1"
  goto end
xp_or_below:
  StrCpy $R8 "0"
end:
  ; Set the program component really required (read only)
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${dcpp} $0
FunctionEnd

Function LaunchLink
  ExecShell "" "$INSTDIR\AirDC.exe"
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
  Delete "$INSTDIR\GeoIPCountryWhois.csv"
  ; remove EmoPacks dir
  RMDir /r "$INSTDIR\EmoPacks"
  
  RMDir /r "$INSTDIR\icons"
  RMDir /r "$INSTDIR\Language"
  
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
  ;Delete "$SMPROGRAMS\AirDC++\*.*"
  ; remove directories used.
  ;RMDir "$SMPROGRAMS\AirDC++"
  IfFileExists "$INSTDIR\Settings\*.xml" 0 check_if_empty
  MessageBox MB_YESNO|MB_ICONQUESTION "Also remove queue and all settings?" IDNO end_uninstall

  ; delete settings directory
  RMDir /r $INSTDIR\Settings
  

check_if_empty:
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
