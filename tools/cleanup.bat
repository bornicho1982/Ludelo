@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   Ludelo - Limpieza Integral y Organizacion del Proyecto
echo ============================================================
echo.

set "ROOT_DIR=%~dp0.."
pushd "%ROOT_DIR%"

echo [1/6] Creando directorios limpios (build, bin, bin\assets, bin\tests)...
if not exist "build\obj" mkdir "build\obj"
if not exist "bin" mkdir "bin"
if not exist "bin\assets" mkdir "bin\assets"
if not exist "bin\tests" mkdir "bin\tests"
if not exist "third_party\vulkan-headers\lib" mkdir "third_party\vulkan-headers\lib"

echo [2/6] Moviendo vulkan-1.lib a third_party\vulkan-headers\lib...
if exist "vulkan-1.lib" move /y "vulkan-1.lib" "third_party\vulkan-headers\lib\vulkan-1.lib" > nul
if exist "vulkan-1.def" move /y "vulkan-1.def" "third_party\vulkan-headers\lib\vulkan-1.def" > nul
if exist "vulkan-1.exp" del /f /q "vulkan-1.exp" > nul

echo [3/6] Copiando librerias DLL y assets hacia bin\...
for %%D in (SDL3.dll avcodec-63.dll avdevice-63.dll avfilter-12.dll avformat-63.dll avutil-61.dll libcrypto-4-x64.dll libssl-4-x64.dll swresample-7.dll swscale-10.dll) do (
    if exist "%%D" copy /y "%%D" "bin\%%D" > nul
)
if exist "assets" (
    xcopy /E /I /Y /Q "assets" "bin\assets" > nul 2>&1
)

echo [4/6] Eliminando archivos .obj sueltos en la raiz...
del /f /q "*.obj" 2>nul
echo        -> Todos los .obj en la raiz eliminados.

echo [5/6] Eliminando ejecutables viejos de pruebas sueltos...
del /f /q "test_auth.exe" "test_crypto.exe" "test_discovery.exe" "test_dualsense.exe" "test_fec.exe" "test_takion.exe" "Ludelo_LiveDiagnostic.exe" "PortalPC_LiveDiagnostic.exe" "PortalPC.exe" 2>nul
echo        -> Ejecutables de prueba eliminados.

echo [6/6] Eliminando scripts de prueba, logs temporales y scratch...
del /f /q "focus_portal.ps1" 2>nul
del /f /q "check_dumpbin.bat" 2>nul
del /f /q "check_event.ps1" 2>nul
del /f /q "check_hwnd_now.ps1" 2>nul
del /f /q "enum_windows.ps1" 2>nul
del /f /q "capture_window.ps1" 2>nul
del /f /q "inspect_gui.bat" 2>nul
del /f /q "inspect_hwnd.ps1" 2>nul
del /f /q "launch_and_inspect.bat" 2>nul
del /f /q "run_portal_test.bat" 2>nul
del /f /q "start_portal.bat" 2>nul
del /f /q "test_build_gui.bat" 2>nul
del /f /q "generate_vulkan_lib.ps1" 2>nul
del /f /q "run_test_auth.bat" 2>nul
del /f /q "run_test_crypto.bat" 2>nul
del /f /q "run_test_discovery.bat" 2>nul
del /f /q "run_test_dualsense.bat" 2>nul
del /f /q "run_test_takion.bat" 2>nul
del /f /q "run_live_diagnostic.bat" 2>nul
del /f /q "run_all_tests.bat" 2>nul
del /f /q "out.txt" 2>nul
del /f /q "test_crypto_out.txt" 2>nul
del /f /q "vulkan_exp.txt" 2>nul
del /f /q "ludelo.log" 2>nul
del /f /q "portalpc.log" 2>nul
del /f /q "chiaki_help.txt" 2>nul
del /f /q "chiaki.zip" 2>nul
if exist "scratch" rd /s /q "scratch" 2>nul
echo        -> Scripts de prueba, logs y archivos scratch eliminados.

popd
echo.
echo ============================================================
echo   [OK] Limpieza integral completada con exito!
echo ============================================================
