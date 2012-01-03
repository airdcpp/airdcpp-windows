@echo off

set send=false
set ftpaddress=builds.airdcpp.net
set ftpuser=username
set ftppass=password
set ftpdir=nightly

IF %send%==false goto :SendDisabled 
IF [%1]==[] goto :invalidParameters
set arch=%1
IF NOT EXIST compiled\%arch%\AirDC.exe goto :exeNotFound
set revFile=lastRevSent%arch%.txt
set solutionDir=%CD%

call svn update
svn info > nul
if errorlevel 1 goto :OnSVNError

for /F "tokens=1,2 delims=: " %%a in ('svn info') do call :InfoProc "%%a" "%%b"
goto :end

:InfoProc
if %1=="Revision" call :UpdateRevision %2
goto :end


:UpdateRevision

set archString=%arch%
if %arch%==Win32 set archString=x86
set fileName=airdcpp_rev%~1_%archString%.7z

echo open %ftpaddress%> checkftp.txt
echo %ftpuser%>> checkftp.txt
echo %ftppass%>> checkftp.txt
echo cd %ftpdir%>> checkftp.txt
echo dir>> checkftp.txt
echo quit>> checkftp.txt

for /f "usebackq delims=" %%i in (`FTP -v -i -s:checkftp.txt`) do (
echo %%i | find "%fileName%" > nul
if not errorlevel 1 goto :ExistsFTP
)

echo Not found from the FTP, compressing
cd installer
7za a -t7z %solutionDir%\%fileName% EmoPacks icons Language Themes > nul
cd ..
cd compiled\%arch%
7za a -t7z %solutionDir%\%fileName% AirDC.exe AirDC.pdb > nul
cd %solutionDir%
call :SendFTP %fileName%
goto :end

:SendFTP
echo open %ftpaddress%> sendftp.txt
echo %ftpuser%>> sendftp.txt
echo %ftppass%>> sendftp.txt
echo cd %ftpdir%>> sendftp.txt
echo binary>> sendftp.txt
echo mput "%~1">> sendftp.txt
echo quit>> sendftp.txt

echo Starting FTP upload
FTP -v -i -s:sendftp.txt
echo Sending finished, deleting extra files...

goto :DeleteFiles

:ExistsFTP
echo This revision exists already on the FTP, aborting
ECHO Y | DEL checkftp.txt
goto :end

:DeleteFiles
ECHO Y | DEL %~1
ECHO Y | DEL sendftp.txt
ECHO Y | DEL checkftp.txt
echo Done
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