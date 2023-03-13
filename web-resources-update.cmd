@echo off

REM This will download and update Web-resources from https://www.npmjs.com/package/airdcpp-webui
REM For it to work you need to manually create 'Web-resources' in the 'installer' directory.
REM You can use the switch '/force' to force an update. 

setlocal

set appver=web-resources-update v0.4

set force=false

set scriptpath=%~dp0

if /i [%1]==[/force] set force=true

if not exist "%ProgramFiles%\7-Zip" goto NO-7-ZIP
set 7path="%ProgramFiles%\7-Zip"

for /F "tokens=* USEBACKQ" %%F IN (`npm view airdcpp-webui version`) DO set version-new-full=%%F

if [%version-new-full%] == [] goto NO-VERSION

title %appver%

if not exist installer\Web-resources goto NO-WEB-RESOURCES

if exist "installer\Web-resources\version.chk" (
    set /P version-old-full=<"installer\Web-resources\version.chk"
 ) else (
     echo 0.0.0>"installer\Web-resources\version.chk"
     set /P version-old-full=<"installer\Web-resources\version.chk"
     goto RENAME-WEB-RESOURCES
)

if [%force%]==[true] goto RENAME-WEB-RESOURCES

FOR /F "tokens=1,2 delims=-" %%A IN ("%version-new-full%") DO set version-new=%%A&set version-new-beta=%%B

FOR /F "tokens=1,2 delims=-" %%A IN ("%version-old-full%") DO set version-old=%%A&set version-old-beta=%%B

set "version-new=%version-new:.=%"
set "version-old=%version-old:.=%"

echo %version-new-full% | findstr /irc:"beta\.[0-9]" >NUL
if not %errorlevel% == 0 (
    set version-new-beta=999
  ) else (
    set "version-new-beta=%version-new-beta:beta.=%"
)

echo %version-old-beta% | findstr /irc:"beta\.[0-9]" >NUL
if not %errorlevel% == 0 (
    set version-old-beta=0
  ) else (
    set "version-old-beta=%version-old-beta:beta.=%"
)

if %version-new% GTR %version-old% goto RENAME-WEB-RESOURCES
if %version-new% LSS %version-old% goto ALREADY-LATEST
if %version-new% GTR %version-old% (
    if %version-new-beta% == 0 (
        goto RENAME-WEB-RESOURCES
    )
)

if %version-new-beta% LSS %version-old-beta% goto ALREADY-LATEST
if %version-new% EQU %version-old% (
    if %version-new-beta% GTR %version-old-beta% (
        goto RENAME-WEB-RESOURCES
    )
)
goto ALREADY-LATEST

:RENAME-WEB-RESOURCES
rename installer\Web-resources $Web-resources 2>NUL
if %errorlevel% == 0 goto DOWNLOAD
goto NO-WEB-RESOURCES

:DOWNLOAD
"%windir%\System32\bitsadmin.exe" /transfer "airdcpp-webui" http://registry.npmjs.org/airdcpp-webui/-/airdcpp-webui-%version-new-full%.tgz "%scriptpath%installer\$Web-resources\latest.tgz"
"%ProgramFiles%\7-Zip\7z.exe" x "installer\$Web-resources\latest.tgz" -o"installer\$Web-resources"
"%ProgramFiles%\7-Zip\7z.exe" x "installer\$Web-resources\latest.tar" -o"installer\$Web-resources"

timeout /t 1 /nobreak > nul
move installer\$Web-resources\package\dist installer

timeout /t 1 /nobreak > nul
rename installer\dist Web-resources
if not %errorlevel% == 0 goto NO-RENAME
goto DONE

:NO-7-ZIP
echo.
echo.Cannot find the directory %ProgramFiles%\7-Zip. Install 7-Zip and try again https://www.7-zip.org
echo.
goto END

:NO-VERSION
echo.
echo.Aborting, no version found. Possible cause, no NPM installed (https://nodejs.org) or no Internet connection.
echo.
goto END

:NO-WEB-RESOURCES
echo.
echo.Web-resources directory not found or in use!
echo.
echo.Note: If this is the first time you run this script, then you need to manually create the directory 'Web-resources' in the 'installer' directory.
echo.
goto END

:NO-RENAME
echo.
echo.Something went wrong. Unable to rename dist directory to Web-resources.
echo.
goto END

:ALREADY-LATEST
echo.
echo.You already have the latest version, %version-old-full%!
echo.
goto END

:DONE
if EXIST "installer\$Web-resources" RMDIR /S /Q "installer\$Web-resources"
if EXIST "installer\$Web-resources" echo.The $Web-resources directory seems to still exist.
echo.%version-new-full%>"installer\Web-resources\version.chk"
echo.
echo.      Old version: %version-old-full%
echo.
echo.Installed version: %version-new-full%
echo.
goto END

:END
IF %0 == "%~0" echo. & echo.This window will automatically close in 10 seconds. Wait or press any key to close it NOW! & timeout /t 10 > nul
