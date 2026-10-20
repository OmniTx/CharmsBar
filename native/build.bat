@echo off
setlocal
where cl >nul 2>&1
if errorlevel 1 (
    echo [CharmsBar] cl.exe not found on PATH.
    echo Run this from the "x64 Native Tools Command Prompt for VS" or a msvc-dev-cmd environment.
    exit /b 1
)
if not exist bin mkdir bin
cl /nologo /std:c++17 /W4 /O2 /MT /EHsc /DUNICODE /D_UNICODE ^
   src\*.cpp ^
   /Fe:bin\charmsbar-native.exe /Fo:bin\ /Fd:bin\ ^
   /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib gdi32.lib dwmapi.lib
if errorlevel 1 (
    echo [CharmsBar] Build failed.
    exit /b 1
)
echo [CharmsBar] Built bin\charmsbar-native.exe
endlocal
