@echo off
setlocal
echo ===================================================
echo   Ludelo Diagnostic / Debug Runner
echo ===================================================
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
set QT_LOGGING_RULES=*.debug=true;qt.qml.*=true;qt.webview.*=true
set QT_DEBUG_PLUGINS=1

echo MSYS2 Path injected.
echo Running build\gui\Ludelo.exe directly in console...
echo Logs are also mirrored to: %%APPDATA%%\Ludelo\logs\ludelo.log
echo ---------------------------------------------------

build\gui\Ludelo.exe

echo ---------------------------------------------------
echo Ludelo exited with code %ERRORLEVEL%.
pause

