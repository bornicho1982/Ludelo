@echo off
title Ludelo Launcher
cd /d "%~dp0.."
echo ============================================================
echo   Iniciando Ludelo (PlayStation Remote Play para PC)...
echo ============================================================
if exist "bin\Ludelo.exe" (
    start "" /d "bin" "bin\Ludelo.exe"
) else (
    echo [ERROR] No se encontro bin\Ludelo.exe. Ejecuta build.bat primero.
    pause
)
