@echo off

REM This will download and update Web-resources from https://www.npmjs.com/package/airdcpp-webui

SETLOCAL

SET scriptpath=%~dp0

if not exist "%ProgramFiles%\7-Zip" goto NO7ZIP
set 7path="%ProgramFiles%\7-Zip"

for /F "tokens=* USEBACKQ" %%F IN (`npm view airdcpp-webui version`) DO (
set version=%%F
)

if "%version%" == "" goto NOVERSION

if not exist installer\Web-resources goto NOWEBRESOURCES

rename installer\Web-resources $Web-resources
if %errorlevel% == 0 goto DOWNLOAD
goto NOWEBRESOURCES

:DOWNLOAD
"%windir%\System32\bitsadmin.exe" /transfer "airdcpp-webui" http://registry.npmjs.org/airdcpp-webui/-/airdcpp-webui-%version%.tgz %scriptpath%installer\$Web-resources\latest.tgz

"%ProgramFiles%\7-Zip\7z.exe" x Installer\$Web-resources\latest.tgz -o"Installer\$Web-resources"

"%ProgramFiles%\7-Zip\7z.exe" x Installer\$Web-resources\latest.tar -o"Installer\$Web-resources"

timeout /t 2 /nobreak > nul
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
goto END

:NORENAME
echo.
echo.Cannot rename dist folder to Web-resources.
echo.
goto END

:DONE
if EXIST "Installer\$Web-resources" RMDIR /S /Q "Installer\$Web-resources"
if EXIST "Installer\$Web-resources" echo.The folder $Web-resources seems to still exists.
echo.Done!
pause
goto END

:END
