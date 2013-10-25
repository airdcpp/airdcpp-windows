@echo off
set file=%1client\version-revno.inc
set tmpfile=%1client\version-revno.inc.tmp

for /F "tokens=1,2,3 delims=-" %%a in ('git describe --abbrev"="4 --long') do (
	echo #define GIT_TAG "%%a" > %tmpfile%
	echo #define GIT_COMMIT "%%b" >> %tmpfile%
	echo #define GIT_HASH "%%c" >> %tmpfile%
) 

for /F "tokens=*" %%a in ('git rev-list HEAD --count') do echo #define GIT_COMMIT_COUNT %%a >> %tmpfile%

for /F "tokens=*" %%a in ('git log -1 --format"="%%at') do echo #define VERSION_DATE %%a >> %tmpfile%

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