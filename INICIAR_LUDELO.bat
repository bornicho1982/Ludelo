@echo off
title Ludelo Launcher
cd /d "%~dp0"
echo ============================================================
echo   Iniciando Ludelo (PlayStation Remote Play para PC)...
echo ============================================================
if exist "%~dp0bin\Ludelo.exe" (
    start "" /d "%~dp0bin" "%~dp0bin\Ludelo.exe"
) else (
    echo [ERROR] No se encontro bin\Ludelo.exe. Ejecuta build.bat primero.
    pause
)
