@echo off

REM This will download and update Web-resources from https://www.npmjs.com/package/airdcpp-webui
REM For it to work you need to manually create 'Web-resources' in the 'installer' folder.

setlocal

set scriptpath=%~dp0

if not exist "%ProgramFiles%\7-Zip" goto NO7ZIP
set 7path="%ProgramFiles%\7-Zip"

for /F "tokens=* USEBACKQ" %%F IN (`npm view airdcpp-webui version`) DO set version=%%F

if "%version%" == "" goto NOVERSION

if not exist installer\Web-resources goto NOWEBRESOURCES

if exist "installer\Web-resources\version.chk" (
    set /P oldversion=<"installer\Web-resources\version.chk"
 ) else (
     echo 0.0.0>"installer\Web-resources\version.chk"
     set /P oldversion=<"installer\Web-resources\version.chk"
 )

if "%version%" LEQ "%oldversion%" (
    goto VERSIONCHECK
  ) else (
    goto RENWEBRES
  )

:VERSIONCHECK
if "%oldversion%"=="%oldversion:beta=%" (
    set oldbetafound=false
    ) else (
      set oldbetafound=true
      if "%version%"=="%version:beta=%" (
      set betafound=false
      goto RENWEBRES
      ) else (
        set betafound=true
      )
    )
goto ALRDYEST

:RENWEBRES
rename installer\Web-resources $Web-resources 2>NUL
if %errorlevel% == 0 goto DOWNLOAD
goto NOWEBRESOURCES

:DOWNLOAD
"%windir%\System32\bitsadmin.exe" /transfer "airdcpp-webui" http://registry.npmjs.org/airdcpp-webui/-/airdcpp-webui-%version%.tgz %scriptpath%installer\$Web-resources\latest.tgz
"%ProgramFiles%\7-Zip\7z.exe" x installer\$Web-resources\latest.tgz -o"installer\$Web-resources"
"%ProgramFiles%\7-Zip\7z.exe" x installer\$Web-resources\latest.tar -o"installer\$Web-resources"

timeout /t 1 /nobreak > nul
move installer\$Web-resources\package\dist installer

timeout /t 1 /nobreak > nul
rename installer\dist Web-resources
if not %errorlevel% == 0 goto NORENAME
goto DONE

:NO7ZIP
echo.
echo.Cannot find the folder %ProgramFiles%\7-Zip. Install 7-Zip and try again http://www.7-zip.org
goto END

:NOVERSION
echo.
echo.Aborting, no version found. Possible cause, no NPM installed or no Internet connection?
echo.
goto END

:NOWEBRESOURCES
echo.
echo.Web-resources folder not found or in use!
echo.
echo.Note: If this is the first time you run this script, then you need to manually create the folder 'Web-resources' in the 'installer' folder.
echo.
goto END

:NORENAME
echo.
echo.Cannot rename dist folder to Web-resources.
echo.
goto END

:ALRDYEST
echo.
echo.You already have latest version, %oldversion%!
echo.
goto END

:DONE
if EXIST "installer\$Web-resources" RMDIR /S /Q "installer\$Web-resources"
if EXIST "installer\$Web-resources" echo.The folder $Web-resources seems to still exist.
echo.%version%>"installer\Web-resources\version.chk"
echo.
echo.      Old version: %oldversion%
echo.
echo.Installed version: %version%
echo.

goto END

:END
IF %0 == "%~0" pause
