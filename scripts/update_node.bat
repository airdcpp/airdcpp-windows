@echo on

:: NOTE: the 64 bit version must be installed first (it will get no postfix then)

:: Config
set NVM_VERSION=22.20.0

:: Architecture argument (Win32/x64)
IF [%1]==[] goto :invalidParameters

set ARCH=%1%
if %ARCH%==x64 (set NVM_ARCH=64) else (set NVM_ARCH=32)
if %ARCH%==x64 (set NVM_FILE_SUFFIX=) else (set NVM_FILE_SUFFIX=32)

:: Install
set OUTPUT_DIR=%~dp0\..\compiled\%ARCH%-release\windows\Node.js

IF NOT EXIST %OUTPUT_DIR% mkdir %OUTPUT_DIR%
nvm install %NVM_VERSION% %NVM_ARCH%
COPY /B %APPDATA%\nvm\v%NVM_VERSION%\node%NVM_FILE_SUFFIX%.exe %OUTPUT_DIR%\node.exe

goto :end

:invalidParameters
echo Please choose the architecture (Win32/x64)!

:end