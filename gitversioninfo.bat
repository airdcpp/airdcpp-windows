@echo off
call git describe --abbrev=4 > version.txt
for /F "tokens=1,2,3 delims=-" %%a in (version.txt) do ( 
echo #define GIT_TAG %%a > %1client\version-revno.inc
echo #define GIT_COMMIT %%b >> %1client\version-revno.inc
echo #define GIT_HASH "%%c" >> %1client\version-revno.inc
)
for /F "tokens=*" %%a in ('git rev-list HEAD --count') do echo #define GIT_COMMIT_COUNT %%a >> %1client\version-revno.inc

ECHO Y | DEL version.txt