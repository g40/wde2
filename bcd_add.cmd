rem
rem Sections courtesy of OpenAI Chatgpt
rem
@echo off

REM Check if the script is running with administrative privileges
NET SESSION >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo This script requires administrative privileges. Please run as Administrator.
    exit /b 1
)

REM Check if both the VHD file path and BCD entry name arguments are provided
IF "%1"=="" (
    echo No arguments provided. Listing existing boot entries...
    echo.
    bcdedit /enum
    exit /b 0
)
IF "%2"=="" (
    echo Incorrect number of arguments provided. Usage: %0 ^<VHD file path^> ^<BCD entry name^>
    echo.
    rem bcdedit /enum
    exit /b 1
)

REM Check if the specified VHD file exists
IF NOT EXIST "%1" (
    echo The specified VHD file does not exist: %1
    echo.
    bcdedit /enum
    exit /b 1
)

REM Create a new boot entry with the specified entry name
bcdedit /create /d "%2" /application osloader

REM Capture the generated GUID using a FOR loop
FOR /F "tokens=4" %%i IN ('bcdedit /create /d "%2" /application osloader') DO SET NEWGUID=%%i

REM Set the device to the VHD file passed as the first argument
bcdedit /set %NEWGUID% device vhd=[C:]%1

REM Set the OS device to the VHD file passed as the first argument
bcdedit /set %NEWGUID% osdevice vhd=[C:]%1

REM Set the system root to the Windows directory inside the VHD
bcdedit /set %NEWGUID% systemroot \windows

REM Optionally, add the new boot entry to the display order as the first entry
bcdedit /displayorder %NEWGUID% /addfirst

@echo Boot entry created with name "%2" and GUID: %NEWGUID%

REM List all boot entries
echo.
echo Listing all boot entries:
bcdedit /enum

pause
