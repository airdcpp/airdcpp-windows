:: Config
set NVM_VERSION=20.17.0

:: Architecture argument (Win32/x64)
IF [%1]==[] goto :invalidParameters

::# 
:: set ARCH=%1%
:: if %ARCH%==Win32 set ARCH=x86

set ARCH=%1%
if %ARCH%==x64 (set NVM_ARCH=64) else (set NVM_ARCH=32)
if %ARCH%==x64 (set NVM_FILE_SUFFIX=) else (set NVM_FILE_SUFFIX=32)

:: Install
set COMPILED_DIR=%~dp0\..\compiled\%ARCH%\Node.js

IF NOT EXIST %COMPILED_DIR% mkdir %COMPILED_DIR%
nvm install %NVM_VERSION% %NVM_ARCH%
COPY /B %APPDATA%\nvm\v%NVM_VERSION%\node%NVM_FILE_SUFFIX%.exe %COMPILED_DIR%\node.exe

goto :end

:invalidParameters
echo Please choose the architecture (Win32/x64)!

:end