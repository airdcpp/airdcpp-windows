@echo off
set file=%1airdcpp\airdcpp\version.inc
set tmpfile=%1airdcpp\airdcpp\version.inc.tmp

cd %1

for /F "tokens=*" %%a in ('git describe --abbrev"="4  --dirty"="-d') do echo #define GIT_TAG "%%a" >> %tmpfile%

for /F "tokens=*" %%a in ('git rev-list HEAD --count') do echo #define GIT_COMMIT_COUNT %%a >> %tmpfile%

for /F "tokens=*" %%a in ('git log -1 --format"="%%at') do echo #define VERSION_DATE %%a >> %tmpfile%

echo #define APPNAME_INC "AirDC++" >> %tmpfile%

fc /b %tmpfile% %file% > nul
if errorlevel 1 goto :versionChanged

echo No Version changes detected, using the old version file
goto :end
	
:versionChanged
	echo Version changes detected updating the version file
	copy /y %tmpfile% %file% > nul
goto :end
	
:end
ECHO Y | DEL %tmpfile%