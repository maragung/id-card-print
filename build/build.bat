@echo off
setlocal enabledelayedexpansion

echo Finding MSVC...
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo MSVC not found.
    exit /b 1
)

call "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Cleaning old build...
if exist *.obj del *.obj
if exist *.res del *.res

echo Building Application (ID Card Print)...
rc /fo app.res ..\src\app.rc

:: Using a unique name to bypass persistent file locks if necessary
cl /O2 /MT /EHsc /W4 /DUNICODE /D_UNICODE ..\src\main_win.cpp app.res /Fe:IDCardPrint_App.exe /link user32.lib gdi32.lib comdlg32.lib comctl32.lib gdiplus.lib ole32.lib shell32.lib /SUBSYSTEM:WINDOWS

if %ERRORLEVEL% equ 0 (
    echo Build successful: IDCardPrint_App.exe
    :: Attempt to rename to final name, ignore errors
    move /Y IDCardPrint_App.exe IDCardPrint.exe >nul 2>&1
) else (
    echo Build failed.
    exit /b 1
)
