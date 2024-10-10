@echo off

cd ..
set solutionDir=%CD%

for /F "tokens=*" %%a in ('git describe --abbrev"="4') do call :MakeRelease "%%a"

goto :End

:MakeRelease

:: 32 bit portable
set fileName=airdcpp_%~1_x86.7z
cd installer
"%ProgramFiles%\7-Zip\7z.exe" a -t7z %solutionDir%\releases\%~1\%fileName% Web-resources EmoPacks Themes popup.bmp dcppboot.xml
cd ..

cd compiled\x86-release\windows
"%ProgramFiles%\7-Zip\7z.exe" a -t7z %solutionDir%\releases\%~1\%fileName% AirDC.exe AirDC.pdb Node.js
cd %solutionDir%

:: 64 bit portable
set fileName=airdcpp_%~1_x64.7z
cd installer
"%ProgramFiles%\7-Zip\7z.exe" a -t7z %solutionDir%\releases\%~1\%fileName% Web-resources EmoPacks Themes popup.bmp dcppboot.xml
cd ..
cd compiled\x64-release\windows
"%ProgramFiles%\7-Zip\7z.exe" a -t7z %solutionDir%\releases\%~1\%fileName% AirDC.exe AirDC.pdb Node.js
cd %solutionDir%

:: Installer

cd installer
"%ProgramFiles(x86)%\NSIS\makensis.exe" /DGIT_VERSION=%~1 AirDC_installscript.nsi
cd %solutionDir%

:End

timeout /t 3