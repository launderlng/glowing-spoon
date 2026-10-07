@echo off
setlocal enabledelayedexpansion
REM ============================================================
REM  Builds xzx.exe from src\main.cpp
REM  Just double-click this file on Windows.
REM  It will use whichever C++ compiler you have:
REM    1) Visual Studio (cl)  - preferred
REM    2) MinGW-w64   (g++)
REM ============================================================

cd /d "%~dp0"
echo.
echo  Building xzx.exe ...
echo.

REM ---- 1) cl already on PATH (Developer Command Prompt) --------
where cl >nul 2>&1
if %errorlevel%==0 goto build_cl

REM ---- 2) try to locate and load Visual Studio tools ----------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
  if defined VSPATH (
    if exist "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" (
      echo  Found Visual Studio at !VSPATH!
      call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
      where cl >nul 2>&1
      if !errorlevel!==0 goto build_cl
    )
  )
)

REM ---- 3) MinGW g++ -------------------------------------------
where g++ >nul 2>&1
if %errorlevel%==0 goto build_gpp

echo  ERROR: No C++ compiler found.
echo.
echo  Install ONE of these, then run build.bat again:
echo    - Visual Studio 2022  (Desktop development with C++ workload)
echo        https://visualstudio.microsoft.com/downloads/
echo    - OR MSYS2 / MinGW-w64  (gives you g++)
echo        https://www.msys2.org/
echo.
pause
exit /b 1

:build_cl
echo  Compiler: MSVC (cl)
cl /nologo /std:c++17 /O2 /EHsc /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 ^
   src\main.cpp /Fe:xzx.exe ^
   /link /SUBSYSTEM:WINDOWS comctl32.lib shell32.lib
if exist xzx.obj del xzx.obj >nul 2>&1
if exist main.obj del main.obj >nul 2>&1
goto done

:build_gpp
echo  Compiler: MinGW (g++)
g++ -std=c++17 -O2 -municode -mwindows -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 ^
    src\main.cpp -o xzx.exe -lcomctl32 -lshell32
goto done

:done
echo.
if exist xzx.exe (
  echo  SUCCESS -^> %cd%\xzx.exe
) else (
  echo  Build failed. See the messages above.
)
echo.
pause
