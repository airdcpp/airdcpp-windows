@echo off

set solutionDir=%CD%

set fileName=airdcpp_VERSION_x86.7z
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks Themes dcppboot.xml
cd ..
cd compiled\Win32
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb
cd %solutionDir%


set fileName=airdcpp_VERSION_x64.7z
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks Themes dcppboot.xml
cd ..
cd compiled\x64
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb
cd %solutionDir%