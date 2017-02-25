@echo off

setlocal

title Upload nightly build of AirDC

set solutionDir=%CD%

if /i [%1]==[Win32] (
  set arch=Win32
  goto CONTINUE
)
if /i [%1]==[x64] (
  set arch=x64
  goto CONTINUE
)
if /i [%1]==[both] (
  set arch=x64
  set both=true
  goto CONTINUE
)

:ARCHITECTURE
cls
set /P c=Choose architecture Win32, x64, Both or Quit [3/6/b/q]?
if /i "%c%" EQU "3" (
  set arch=Win32
  goto CONTINUE
)
if /i "%c%" EQU "6" (
	set arch=x64
	goto CONTINUE
)
if /i "%c%" EQU "b" (
  set arch=x64
  set both=true
  goto CONTINUE
)
if /i "%c%" EQU "q" goto ENDBYUSER
goto ARCHITECTURE

:CONTINUE
for /f "delims=" %%x in (ftp_credentials.txt) do (set "%%x")

set send=true
set ftpaddress=ftp.airdcpp.net
set ftpdir=nightly
set updaterdir=updater
set versiondir=version

cd ..

rem Pre checks
if /i %send%==false goto SENDDISABLED 
if not exist compiled\%arch%\AirDC.exe goto EXENOTFOUND

rem Check revision
git describe > nul
if errorlevel 1 goto ONGITERROR

for /F "tokens=*" %%a in ('git describe --abbrev"="4') do set gitverw="%%a"
set gitver=%gitverw:~1,-1%

:UPDATEREVISION
rem Compare the current revision to ftp
set archString=%arch%
if /i %arch%==Win32 set archString=x86
set fileName=updater_%archString%_%gitver%.zip

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
if not errorlevel 1 goto EXISTSFTP
)

if exist "checkftp.txt" DEL checkftp.txt

rem Not found, make sure that we have the latest version file
echo.Not found from the FTP, fetching the latest version file

echo open %ftpaddress%> fetchversionxml.txt
echo %ftpuser%>> fetchversionxml.txt
echo %ftppass%>> fetchversionxml.txt
echo cd %ftpdir%>> fetchversionxml.txt
echo cd %versiondir%>> fetchversionxml.txt
echo binary>> fetchversionxml.txt
echo get version.xml>> fetchversionxml.txt
echo quit>> fetchversionxml.txt

FTP -v -i -s:fetchversionxml.txt
if exist "fetchversionxml.txt" DEL fetchversionxml.txt
if exist "cd" DEL cd
if exist "dir" DEL dir
if exist "open" DEL open
if exist "user" DEL user
if exist "quit" DEL quit

echo.Creating the updater file
cd %arch%
AirDC.exe /createupdate

cd ..

:SENDFTP
echo open %ftpaddress%> sendftp.txt
echo %ftpuser%>> sendftp.txt
echo %ftppass%>> sendftp.txt
echo cd %ftpdir%>> sendftp.txt
echo cd %updaterdir%>> sendftp.txt
echo binary>> sendftp.txt
echo mput "%fileName%">> sendftp.txt
echo cd ..>> sendftp.txt
echo cd %versiondir%>> sendftp.txt
echo mput version.xml.sign>> sendftp.txt
echo mput version.xml>> sendftp.txt
echo quit>> sendftp.txt

echo.Starting FTP upload
FTP -v -i -s:sendftp.txt
if exist "sendftp.txt" DEL sendftp.txt
if exist "%fileName%" DEL %fileName%
echo.Sending finished
goto END

:EXISTSFTP
echo.
echo.This revision exists already on the FTP, aborting...
if exist "checkftp.txt" DEL checkftp.txt
if exist "cd" DEL cd
if exist "dir" DEL dir
if exist "open" DEL open
if [%both%]==[true] (
  set arch=Win32
  set both=false
  cd %solutionDir%
  goto CONTINUE
)

goto END

:ONGITERROR
echo.Error get version from GIT
goto END

:EXENOTFOUND
echo.Please check that all files are in place!
goto END

:SENDDISABLED
echo.FTP uploading has been disabled, please edit send_updater_spr.cmd to enable it
goto END

:END
if exist "version.xml" DEL version.xml
if exist "version.xml.sign" DEL version.xml.sign
cd %solutionDir%
if [%both%]==[true] (
  set arch=Win32
  set both=false
  echo.
  echo.AirDC++ architecture x64 uploaded, continue with Win32...
  echo.
  goto CONTINUE
)

:ENDBYUSER
echo.
if %0 == "%~0" pause
