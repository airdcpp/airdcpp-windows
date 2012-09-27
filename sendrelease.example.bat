@echo off

set send=true
set ftpaddress=builds.airdcpp.net
set ftpuser=username
set ftppass=password
set ftpdir=nightly
set updaterdir=updater
set versiondir=version

::Pre checks
IF %send%==false goto :SendDisabled 
IF [%1]==[] goto :invalidParameters
set arch=%1
IF NOT EXIST compiled\%arch%\AirDC.exe goto :exeNotFound
set solutionDir=%CD%


::Check revision
call svn update
svn info > nul
if errorlevel 1 goto :OnSVNError

for /F "tokens=1,2 delims=: " %%a in ('svn info') do call :InfoProc "%%a" "%%b"
goto :end

:InfoProc
if %1=="Revision" call :UpdateRevision %2
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

echo Not found from the FTP, fetching the latest version file

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

echo Creating the updater file
cd %arch%
AirDC.exe /createupdate
cd ..
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
echo This revision exists already on the FTP, aborting
ECHO Y | DEL checkftp.txt
goto :end

:OnSVNError
echo Error get revision from SVN
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