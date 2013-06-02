@echo off

set solutionDir=%CD%

set fileName=airdcpp_VERSION_x86.7z
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks Language Themes dcppboot.xml popup.bmp
cd ..
cd compiled\Win32
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb
cd %solutionDir%


set fileName=airdcpp_VERSION_x64.7z
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks Language Themes dcppboot.xml popup.bmp
cd ..
cd compiled\x64
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb
cd %solutionDir%

set fileName=airdcpp_VERSION_src.7z
7za a -t7z %solutionDir%\%fileName% boost bzip2 client geoip installer leveldb MakeDefs miniupnpc minizip natpmp openssl regex res snappy windows zlib AirDC.rc AirDC.sln Changelog_AirDC.txt client.vcxproj client.vcxproj.filters EN_Example.xml StrongDC.vcxproj StrongDC.vcxproj.filters
cd %solutionDir%