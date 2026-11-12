@echo off
echo Building Cinema Management System...
echo.
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GCC compiler not found. Please install MinGW-w64 or TDM-GCC.
    echo Download: https://github.com/niXman/mingw-builds-binaries/releases
    pause
    exit /b 1
)

gcc -Wall -Wextra -O2 -o cinema.exe main.c file_io.c user_auth.c movies.c booking.c admin.c encryption.c payment.c

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build SUCCESSFUL! Output: cinema.exe
    echo.
    echo Run cinema.exe to start the program.
) else (
    echo.
    echo Build FAILED with errors above.
)
pause
