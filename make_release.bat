@echo off

set solutionDir=%CD%

set fileName=airdcpp_VERSION_src.7z
7za a -t7z %solutionDir%\%fileName% boost bzip2 client geoip installer MakeDefs miniupnpc natpmp openssl regex res windows zlib AirDC.rc AirDC.sln boost-regex.vcxproj boost-regex.vcxproj.filters bzip2.vcxproj bzip2.vcxproj.filters Changelog_AirDC.txt client.vcxproj client.vcxproj.filters EN_Example.xml regex.vcxproj regex.vcxproj.filters StrongDC.vcxproj StrongDC.vcxproj.filters zlib.vcxproj zlib.vcxproj.filters
cd %solutionDir%

set fileName=airdcpp_VERSION_x86.7z
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks Language Themes
cd ..
cd compiled\Win32
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb
cd %solutionDir%


set fileName=airdcpp_VERSION_x64.7z
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks Language Themes
cd ..
cd compiled\x64
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb
cd %solutionDir%