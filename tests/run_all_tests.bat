@echo off
setlocal enabledelayedexpansion

set "TESTS_DIR=%~dp0"
set "BIN_TESTS=%TESTS_DIR%..\bin\tests"

echo ============================================================
echo   Ludelo - Ejecucion de Suite Completa de Tests
echo ============================================================
echo.

if not exist "%BIN_TESTS%" (
    echo [INFO] Compilando tests primero mediante build.bat tests...
    call "%TESTS_DIR%..\build.bat" tests
    exit /b %ERRORLEVEL%
)

set "FAILED=0"
set "PASSED=0"

for %%T in (test_fec.exe test_discovery.exe test_dualsense.exe test_takion.exe test_auth.exe test_crypto.exe) do (
    if exist "%BIN_TESTS%\%%T" (
        echo ------------------------------------------------------------
        echo [RUNNING] %%T...
        "%BIN_TESTS%\%%T"
        if !ERRORLEVEL! equ 0 (
            echo [PASSED] %%T completado con exito.
            set /a PASSED+=1
        ) else (
            echo [FAILED] %%T fallo con codigo !ERRORLEVEL!.
            set /a FAILED+=1
        )
    ) else (
        echo [SKIPPED] %%T no encontrado. Compila con 'build.bat tests'.
    )
    echo.
)

echo ============================================================
echo   RESUMEN DE PRUEBAS:
echo   Superadas : %PASSED% / 6
echo   Fallidas  : %FAILED% / 6
echo ============================================================

if %FAILED% equ 0 (
    echo   RESULTADO GLOBAL: TODAS LAS PRUEBAS PASARON EXITOSAMENTE
    exit /b 0
) else (
    echo   RESULTADO GLOBAL: HUBO PRUEBAS FALLIDAS
    exit /b 1
)
