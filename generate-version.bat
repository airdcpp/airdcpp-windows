@echo off
set file=%1client\version-revno.inc
set tmpfile=%1client\version-revno.inc.tmp
set cmd=git describe --abbrev"="4 --long

for /F "tokens=1,2,3 delims=-" %%a in ('%cmd%') do (
	echo #define GIT_TAG "%%a" > %tmpfile%
	echo #define GIT_COMMIT "%%b" >> %tmpfile%
	echo #define GIT_HASH "%%c" >> %tmpfile%
) 

for /F "tokens=*" %%a in ('git rev-list HEAD --count') do echo #define GIT_COMMIT_COUNT %%a >> %tmpfile%

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