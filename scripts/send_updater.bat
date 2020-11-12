@echo off

:: for /f "delims=" %%x in (ftp_credentials.txt) do (set "%%x")

set dryrun=true
set send=true
set ftpaddress=ftp.airdcpp.net
set ftpdir=nightly
set updaterdir=updater
set versiondir=version
set startupdir=%CD%

SET SOLUTION_DIR=%~dp0\..\

cd %SOLUTION_DIR%

for /f "delims=" %%x in (%SOLUTION_DIR%\scripts\ftp_credentials.txt) do (set "%%x")

::Pre checks
IF %send%==false goto :SendDisabled 
IF [%1]==[] goto :invalidParameters
set arch=%1
IF NOT EXIST compiled\%arch%\AirDC.exe goto :exeNotFound


::Check revision
call git describe > nul
if errorlevel 1 goto :OnGitError

for /F "tokens=*" %%a in ('git describe --abbrev"="4') do call :UpdateRevision "%%a"
goto :end

:UpdateRevision

::Compare the current revision to ftp
set archString=%arch%
if %arch%==Win32 set archString=x86
set fileName=updater_%archString%_%~1.zip

cd compiled

echo open %ftpaddress%> checkftp.txt
echo %ftpuser%>> checkftp.txt
echo %ftppass%>> checkftp.txt
echo cd %ftpdir%>> checkftp.txt
echo cd %updaterdir%>> checkftp.txt
echo dir>> checkftp.txt
echo quit>> checkftp.txt

for /f "usebackq delims=" %%i in (`FTP -v -i -s:checkftp.txt`) do (
echo %%i | find "%fileName%" > nul
if not errorlevel 1 goto :ExistsFTP
)

ECHO Y | DEL checkftp.txt

::Not found, make sure that we have the latest version file

echo Not found from the FTP, fetching the latest version file...

echo open %ftpaddress%> fetchversionxml.txt
echo %ftpuser%>> fetchversionxml.txt
echo %ftppass%>> fetchversionxml.txt
echo cd %ftpdir%>> fetchversionxml.txt
echo cd %versiondir%>> fetchversionxml.txt
echo binary>> fetchversionxml.txt
echo get version.xml>> fetchversionxml.txt
echo quit>> fetchversionxml.txt

FTP -v -i -s:fetchversionxml.txt
ECHO Y | DEL fetchversionxml.txt
ECHO Y | DEL cd
ECHO Y | DEL dir
ECHO Y | DEL open
ECHO Y | DEL quit

:: Install Node.js
echo Installing Node.js...
call %SOLUTION_DIR%\scripts\update_node.bat > nul

:: Create updater
echo Creating the updater file...
cd %arch%
AirDC.exe /createupdate
cd ..

:: Send (if enabled)
IF %dryrun%==true goto :end 
call :SendFTP %fileName%
goto :end

:SendFTP
echo open %ftpaddress%> sendftp.txt
echo %ftpuser%>> sendftp.txt
echo %ftppass%>> sendftp.txt
echo cd %ftpdir%>> sendftp.txt
echo cd %updaterdir%>> sendftp.txt
echo binary>> sendftp.txt
echo mput "%~1">> sendftp.txt
echo cd ..>> sendftp.txt
echo cd %versiondir%>> sendftp.txt
echo mput version.xml.sign>> sendftp.txt
echo mput version.xml>> sendftp.txt
echo quit>> sendftp.txt

echo Starting FTP upload
FTP -v -i -s:sendftp.txt
ECHO Y | DEL sendftp.txt
ECHO Y | DEL %~1
echo Sending finished

goto :end

:ExistsFTP
echo This version exists on the FTP already, aborting
ECHO Y | DEL checkftp.txt
goto :end

:OnGitError
echo Error get version from GIT
goto :end

:invalidParameters
echo Please choose the architecture (Win32/x64)!
goto :end

:exeNotFound
echo Please choose a proper architecture (Win32/x64) and check that all files are in place!
goto :end

:SendDisabled
echo FTP uploading has been disabled, please edit sendrelease.bat to enable it
goto :end

:end
cd %startupdir%
if %0 == "%~0" timeout /t 5
