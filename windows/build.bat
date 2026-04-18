@echo off
setlocal

echo.
echo  ==============================================
echo   WoW 3.3.5a Emulator  -  Windows Build
echo   MSVC  +  WinSock2  (Windows Only)
echo  ==============================================
echo.

:: Find Visual Studio
if defined VCVARS (call "%VCVARS%" >nul 2>&1) else (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
)

:: Create build dir
if not exist "..\build" mkdir ..\build
cd /d "%~dp0..\build"

echo [Build] Cleaning previous build...
if exist *.obj del /Q *.obj 2>nul
if exist *.o    del /Q *.o    2>nul
if exist *.dep  del /Q *.dep  2>nul
if exist wow-emulator.exe del /Q wow-emulator.exe 2>nul

echo [Build] Compiling with MSVC (Windows Only - WinSock2)...
echo.

set MYSQL_INCLUDE=C:\mysql\include
set MYSQL_LIB=C:\mysql\lib\libmysql.lib

set SRC=..\src\main.c ^
       ..\src\shared\network.c ^
       ..\src\shared\wincompat.c ^
       ..\src\shared\database.c ^
       ..\src\shared\logging.c ^
       ..\src\shared\config.c ^
       ..\src\shared\big_number.c ^
       ..\src\shared\byte_buffer.c ^
       ..\src\shared\wow_packet.c ^
       ..\src\shared\update_mask.c ^
       ..\src\shared\srp6.c ^
       ..\src\authserver\auth_server.c ^
       ..\src\authserver\auth_main.c ^
       ..\src\worldserver\world_server.c ^
       ..\src\worldserver\world_socket.c ^
       ..\src\worldserver\world_main.c ^
       ..\src\worldserver\world_player.c ^
       ..\src\worldserver\world_session.c ^
       ..\src\scripting\script_engine.c ^
       ..\src\scripting\wss_lexer.c ^
       ..\src\scripting\wss_vm.c ^
       ..\src\scripting\wss_value.c ^
       ..\src\scripting\wss_wow_hooks.c ^
       ..\src\scripting\wss_chunk.c ^
       ..\src\scripting\wss_compiler.c ^
       ..\src\scripting\wss_objectstore.c ^
       ..\src\scripting\wss_scanner.c ^
       ..\src\scripting\wss_bindings.c ^
       ..\src\scripts\core\builtin_statements.c

set INC=/I"..\include" /I"..\include\shared" /I"..\include\scripting" /I"..\include\worldserver" /I"..\include\authserver" /I"..\src" /I"..\src\shared" /I"..\src\scripting"

set CFLAGS=/W4 /O2 /DWIN32 /D_WINSOCK_DEPRECATED_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /DNDEBUG /MT /Zi
set LFLAGS=/OUT:wow-emulator.exe ws2_32.lib Advapi32.lib /DEBUG

if exist "C:\mysql\lib\libmysql.lib" (
    set CFLAGS=%CFLAGS% /D_MYSQL
    set LFLAGS=%LFLAGS% /LIBPATH:"C:\mysql\lib" libmysql.lib
    set INC=%INC% /I"C:\mysql\include"
    echo [Build] MySQL support: ENABLED
) else (
    echo [Build] MySQL support: DISABLED (libmysql.lib not found)
    echo [Build] SQLite will be used instead.
)

echo [Build] Source files:
echo %SRC%
echo.

cl.exe %CFLAGS% %INC% %SRC% /link %LFLAGS%

if errorlevel 1 (
    echo.
    echo [Build] FAILED - Check errors above.
    echo.
    echo If you see 'unresolved symbol' errors for MySQL functions,
    echo either install MySQL Connector/C or the emulator will use SQLite.
    echo Download from: https://dev.mysql.com/downloads/connector/c/
    echo.
    pause
    exit /b 1
)

echo.
echo  ==============================================
echo   Build SUCCEEDED!
echo  ==============================================
echo.
echo  Output: ..\build\wow-emulator.exe
echo.
echo  Next steps:
echo    1. Copy wow-emulator.exe to your server folder
echo    2. Edit configs\worldserver.conf with your settings
echo    3. Run install.bat to install database
echo    4. Run wow-emulator.exe to start the server
echo.
pause