@echo off
setlocal enabledelayedexpansion

:: ============================================================================
::  Ludelo - Master Build System (C++23 / MSVC)
::  PlayStation Remote Play Client for PC - Windows 11
:: ============================================================================

set "PROJECT_ROOT=%~dp0"
set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "BIN_DIR=%PROJECT_ROOT%\bin"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "OBJ_DIR=%PROJECT_ROOT%\build\obj"
set "PROTO_GEN_DIR=%PROJECT_ROOT%\build\proto_gen"
set "TESTS_BIN_DIR=%PROJECT_ROOT%\bin\tests"

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=app"

:: ─── Help / Usage ────────────────────────────────────────────────────────────
if /i "%TARGET%"=="help"   goto :usage
if /i "%TARGET%"=="-h"     goto :usage
if /i "%TARGET%"=="--help" goto :usage

:: ─── Clean Target ────────────────────────────────────────────────────────────
if /i "%TARGET%"=="clean" (
    echo [CLEAN] Eliminando directorios de build y artefactos intermedios...
    if exist "%BUILD_DIR%"    rd /s /q "%BUILD_DIR%"
    if exist "%TESTS_BIN_DIR%" rd /s /q "%TESTS_BIN_DIR%"
    if exist "%BIN_DIR%\Ludelo.exe" del /f /q "%BIN_DIR%\Ludelo.exe"
    if exist "%BIN_DIR%\PortalPC.exe" del /f /q "%BIN_DIR%\PortalPC.exe"
    if exist "%BIN_DIR%\Ludelo_LiveDiagnostic.exe" del /f /q "%BIN_DIR%\Ludelo_LiveDiagnostic.exe"
    echo [CLEAN] Completado.
    exit /b 0
)

:: ─── Initialize MSVC Environment ─────────────────────────────────────────────
if not defined DevEnvDir (
    set "VCVARS="

    :: VS 2022 - buscar con vswhere primero (metodo mas robusto)
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        for /f "usebackq tokens=*" %%I in (
            `"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`
        ) do (
            if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" (
                set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
            )
        )
    )

    :: Fallback manual para VS 2022 Community / Professional / Enterprise / BuildTools
    if not defined VCVARS (
        for %%E in (Community Professional Enterprise BuildTools) do (
            if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" (
                set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
            )
        )
    )

    if not defined VCVARS (
        echo [ERROR] No se encontro Visual Studio 2022. Instala MSVC desde:
        echo         https://visualstudio.microsoft.com/downloads/
        exit /b 1
    )

    echo [ENV] Configurando entorno MSVC x64 desde:
    echo       !VCVARS!
    call "!VCVARS!" > nul
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Fallo al inicializar el entorno de compilacion MSVC.
        exit /b 1
    )
    echo [ENV] MSVC listo: !VCToolsVersion!
)

:: ─── Detect OpenSSL ──────────────────────────────────────────────────────────
set "OPENSSL_INC="
set "OPENSSL_LIB="
if exist "C:\Program Files\OpenSSL-Win64\include" (
    set "OPENSSL_INC=/I "C:\Program Files\OpenSSL-Win64\include""
    set "OPENSSL_LIB=/LIBPATH:"C:\Program Files\OpenSSL-Win64\lib\VC\x64\MD""
)
if exist "C:\Program Files\OpenSSL\include" (
    set "OPENSSL_INC=/I "C:\Program Files\OpenSSL\include""
    set "OPENSSL_LIB=/LIBPATH:"C:\Program Files\OpenSSL\lib\VC\x64\MD""
)

:: ─── Ensure Directories ──────────────────────────────────────────────────────
if not exist "%BUILD_DIR%"       mkdir "%BUILD_DIR%"
if not exist "%OBJ_DIR%"         mkdir "%OBJ_DIR%"
if not exist "%PROTO_GEN_DIR%"   mkdir "%PROTO_GEN_DIR%"
if not exist "%BIN_DIR%"         mkdir "%BIN_DIR%"
if not exist "%BIN_DIR%\assets"  mkdir "%BIN_DIR%\assets"
if not exist "%TESTS_BIN_DIR%"   mkdir "%TESTS_BIN_DIR%"
if not exist "%PROJECT_ROOT%\third_party\vulkan-headers\lib" mkdir "%PROJECT_ROOT%\third_party\vulkan-headers\lib"

:: ─── Ensure Vulkan Import Library ────────────────────────────────────────────
if exist "%PROJECT_ROOT%\vulkan-1.lib" (
    move /y "%PROJECT_ROOT%\vulkan-1.lib" "%PROJECT_ROOT%\third_party\vulkan-headers\lib\vulkan-1.lib" > nul 2>&1
)
if exist "%PROJECT_ROOT%\vulkan-1.def" (
    move /y "%PROJECT_ROOT%\vulkan-1.def" "%PROJECT_ROOT%\third_party\vulkan-headers\lib\vulkan-1.def" > nul 2>&1
)

:: ─── Dispatch Target ─────────────────────────────────────────────────────────
if /i "%TARGET%"=="app"   goto :build_app
if /i "%TARGET%"=="tests" goto :build_tests
if /i "%TARGET%"=="msix"  goto :build_msix
if /i "%TARGET%"=="all" (
    call :build_app
    if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
    call :build_tests
    exit /b !ERRORLEVEL!
)
if /i "%TARGET%"=="diag"  goto :build_diag

echo [ERROR] Objetivo desconocido: '%TARGET%'
goto :usage

:: ============================================================================
::  Subrutina: Generar codigo Protobuf
:: ============================================================================
:generate_protos
echo.
echo [PROTO] Generando codigo C++ desde archivos .proto...

:: Buscar protoc: primero en PATH, luego en vcpkg tools
set "PROTOC="
where protoc > nul 2>&1
if !ERRORLEVEL! equ 0 (
    set "PROTOC=protoc"
) else (
    :: Buscar en vcpkg tools (instalacion tipica)
    for /d %%D in ("%USERPROFILE%\.vcpkg\downloads\tools\protobuf-*") do (
        if exist "%%D\windows\protoc.exe" set "PROTOC=%%D\windows\protoc.exe"
    )
    :: Buscar en vcpkg installed
    if exist "%PROJECT_ROOT%\build\vcpkg_installed\x64-windows\tools\protobuf\protoc.exe" (
        set "PROTOC=%PROJECT_ROOT%\build\vcpkg_installed\x64-windows\tools\protobuf\protoc.exe"
    )
    if exist "C:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe" (
        set "PROTOC=C:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe"
    )
)

if not defined PROTOC (
    echo [WARN] protoc no encontrado. Saltando generacion de protos.
    echo        Para generarlos, instala protobuf via vcpkg o anade protoc al PATH.
    exit /b 0
)

set "PROTO_INCLUDES=/I "%PROJECT_ROOT%\proto""

for %%P in ("%PROJECT_ROOT%\proto\*.proto") do (
    echo        Generando %%~nP.pb.cc ...
    "!PROTOC!" !PROTO_INCLUDES! --cpp_out="%PROTO_GEN_DIR%" "%%P"
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Fallo generando %%~nP.proto
        exit /b 1
    )
)

:: Recopilar archivos .pb.cc generados
set "PROTO_SRCS="
for %%F in ("%PROTO_GEN_DIR%\*.pb.cc") do (
    set "PROTO_SRCS=!PROTO_SRCS! "%%F""
)

echo [PROTO] Generacion completada.
exit /b 0

:: ============================================================================
::  Subrutina: Build Ludelo Application
:: ============================================================================
:build_app
echo.
echo ============================================================
echo   Compilando Ludelo GUI (Vulkan + SDL3 + ImGui + C++23)
echo ============================================================
echo.

:: ── [0/4] Generar Protobuf ────────────────────────────────
call :generate_protos
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

:: ── [1/4] Compilar Chiaki Core (C) ───────────────────────
call :build_chiaki_core
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

:: ── [2/4] Compilar unidades C++23 ────────────────────────
echo.
echo [2/4] Compilando unidades C++23 hacia build\obj\...

set INCLUDES=/I "%PROJECT_ROOT%\src" ^
 /I "%PROJECT_ROOT%\build\proto_gen" ^
 /I "%PROJECT_ROOT%\third_party\SDL3\include" ^
 /I "%PROJECT_ROOT%\third_party\vulkan-headers\include" ^
 /I "%PROJECT_ROOT%\third_party\imgui" ^
 /I "%PROJECT_ROOT%\third_party\imgui\backends" ^
 /I "%PROJECT_ROOT%\third_party\spdlog\include" ^
 /I "%PROJECT_ROOT%\third_party\json\single_include" ^
 /I "%PROJECT_ROOT%\third_party\ffmpeg\include" ^
 /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\include" ^
 /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\nanopb" ^
 /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\jerasure\include" ^
 /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\gf-complete\include" ^
 /I "%PROJECT_ROOT%\third_party\webview2\build\native\include" ^
 !OPENSSL_INC!

set "CFLAGS=/nologo /std:c++latest /EHsc /utf-8 /O2 /MP /W3 /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DVK_USE_PLATFORM_WIN32_KHR"

:: Recopilar archivos proto generados para compilacion C++
set "PROTO_CPP_SRCS="
if exist "%PROTO_GEN_DIR%\*.pb.cc" (
    for %%F in ("%PROTO_GEN_DIR%\*.pb.cc") do (
        set "PROTO_CPP_SRCS=!PROTO_CPP_SRCS! "%%F""
    )
)

cl !CFLAGS! !INCLUDES! /c /Fo"%OBJ_DIR%/" ^
   "%PROJECT_ROOT%\src\main.cpp" ^
   "%PROJECT_ROOT%\src\UI\App.cpp" ^
   "%PROJECT_ROOT%\src\Platform\WindowsWindow.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Auth\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Cloud\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Config\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Crypto\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Discovery\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Input\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Net\*.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Stream\*.cpp" ^
   "%PROJECT_ROOT%\third_party\imgui\imgui.cpp" ^
   "%PROJECT_ROOT%\third_party\imgui\imgui_draw.cpp" ^
   "%PROJECT_ROOT%\third_party\imgui\imgui_tables.cpp" ^
   "%PROJECT_ROOT%\third_party\imgui\imgui_widgets.cpp" ^
   "%PROJECT_ROOT%\third_party\imgui\backends\imgui_impl_sdl3.cpp" ^
   "%PROJECT_ROOT%\third_party\imgui\backends\imgui_impl_vulkan.cpp" ^
   !PROTO_CPP_SRCS!

if !ERRORLEVEL! neq 0 (
    echo.
    echo [ERROR] Fallo la compilacion de objetos en build\obj\.
    exit /b 1
)

:: ── [3/4] Enlazar ────────────────────────────────────────
echo.
echo [3/4] Enlazando bin\Ludelo.exe...

set LIBPATHS=/LIBPATH:"%PROJECT_ROOT%\third_party\SDL3\lib\x64" ^
 /LIBPATH:"%PROJECT_ROOT%\third_party\ffmpeg\lib" ^
 /LIBPATH:"%PROJECT_ROOT%\third_party\vulkan-headers\lib" ^
 /LIBPATH:"%BUILD_DIR%" ^
 /LIBPATH:"%PROJECT_ROOT%\third_party\webview2\build\native\x64"
if defined OPENSSL_LIB set LIBPATHS=!LIBPATHS! !OPENSSL_LIB!

set "LIBS=chiaki.lib vulkan-1.lib SDL3.lib"
set "LIBS=!LIBS! avcodec.lib avformat.lib avutil.lib swscale.lib swresample.lib"
set "LIBS=!LIBS! libcrypto.lib libssl.lib"
set "LIBS=!LIBS! ws2_32.lib shell32.lib user32.lib gdi32.lib advapi32.lib"
set "LIBS=!LIBS! ole32.lib hid.lib setupapi.lib crypt32.lib iphlpapi.lib bcrypt.lib"
set "LIBS=!LIBS! dwmapi.lib uxtheme.lib WebView2Loader.dll.lib"

pushd "%OBJ_DIR%"
link /NOLOGO /SUBSYSTEM:WINDOWS /MACHINE:X64 !LIBPATHS! /OUT:"%BIN_DIR%\Ludelo.exe" *.obj !LIBS!
set LINK_RES=!ERRORLEVEL!
popd

if !LINK_RES! neq 0 (
    echo.
    echo [ERROR] Fallo el enlazado de Ludelo.exe.
    exit /b !LINK_RES!
)

:: ── [4/4] Desplegar DLLs y assets ────────────────────────
echo.
echo [4/4] Desplegando dependencias de ejecucion (DLLs y assets en bin\)...

:: FFmpeg DLLs
for %%D in (avcodec-63.dll avdevice-63.dll avfilter-12.dll avformat-63.dll avutil-61.dll swresample-7.dll swscale-10.dll) do (
    if exist "%PROJECT_ROOT%\%%D" copy /y "%PROJECT_ROOT%\%%D" "%BIN_DIR%\%%D" > nul
)

:: SDL3
if exist "%PROJECT_ROOT%\SDL3.dll" copy /y "%PROJECT_ROOT%\SDL3.dll" "%BIN_DIR%\SDL3.dll" > nul
if exist "%PROJECT_ROOT%\third_party\SDL3\lib\x64\SDL3.dll" (
    copy /y "%PROJECT_ROOT%\third_party\SDL3\lib\x64\SDL3.dll" "%BIN_DIR%\SDL3.dll" > nul
)

:: OpenSSL — copiar la version que este disponible (prioridad: OpenSSL 3, fallback OpenSSL 1.1/4)
set "OPENSSL_BIN="
if exist "C:\Program Files\OpenSSL-Win64\bin" set "OPENSSL_BIN=C:\Program Files\OpenSSL-Win64\bin"
if exist "C:\Program Files\OpenSSL\bin"       set "OPENSSL_BIN=C:\Program Files\OpenSSL\bin"

if defined OPENSSL_BIN (
    for %%D in (libcrypto-3-x64.dll libssl-3-x64.dll libcrypto-4-x64.dll libssl-4-x64.dll) do (
        if exist "!OPENSSL_BIN!\%%D" (
            if not exist "%BIN_DIR%\%%D" copy /y "!OPENSSL_BIN!\%%D" "%BIN_DIR%\" > nul
        )
    )
)
:: Tambien copiar desde la raiz del proyecto si estan ahi
for %%D in (libcrypto-4-x64.dll libssl-4-x64.dll) do (
    if exist "%PROJECT_ROOT%\%%D" (
        if not exist "%BIN_DIR%\%%D" copy /y "%PROJECT_ROOT%\%%D" "%BIN_DIR%\%%D" > nul
    )
)
)

:: WebView2
if exist "%PROJECT_ROOT%\third_party\webview2\build\native\x64\WebView2Loader.dll" (
    copy /y "%PROJECT_ROOT%\third_party\webview2\build\native\x64\WebView2Loader.dll" "%BIN_DIR%\WebView2Loader.dll" > nul
)

:: Assets y shaders
if exist "%PROJECT_ROOT%\assets" (
    xcopy /E /I /Y /Q "%PROJECT_ROOT%\assets" "%BIN_DIR%\assets" > nul 2>&1
)

echo.
echo ============================================================
echo   [OK] Ludelo compilado y desplegado con exito en:
echo        %BIN_DIR%\Ludelo.exe
echo ============================================================
exit /b 0

:: ============================================================================
::  Subrutina: Build & Run Tests
:: ============================================================================
:build_tests
echo.
echo ============================================================
echo   Compilando Suite de Tests hacia bin\tests\...
echo ============================================================
echo.

set INCLUDES=/I "%PROJECT_ROOT%\src" ^
 /I "%PROJECT_ROOT%\build\proto_gen" ^
 /I "%PROJECT_ROOT%\third_party\spdlog\include" ^
 /I "%PROJECT_ROOT%\third_party\json\single_include" ^
 /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\include" ^
 !OPENSSL_INC!

if not exist "%BUILD_DIR%\chiaki.lib" (
    call :build_chiaki_core
    if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
)

set "LIBPATHS_T=/LIBPATH:"%BUILD_DIR%""
if defined OPENSSL_LIB set "LIBPATHS_T=!LIBPATHS_T! !OPENSSL_LIB!"

set "CFLAGS=/nologo /std:c++latest /EHsc /utf-8 /O2 /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DNOMINMAX"
set "COMMON_LIBS=chiaki.lib libcrypto.lib libssl.lib crypt32.lib shell32.lib ws2_32.lib hid.lib setupapi.lib iphlpapi.lib bcrypt.lib"

set "TESTS_OBJ_DIR=%BUILD_DIR%\obj_tests"
if not exist "!TESTS_OBJ_DIR!" mkdir "!TESTS_OBJ_DIR!"

for %%T in (test_fec test_discovery test_dualsense test_takion test_auth test_crypto) do (
    if exist "%PROJECT_ROOT%\tests\%%T.cpp" (
        echo [TEST BUILD] Compilando %%T.exe...
        set "EXTRA_SRCS="
        if "%%T"=="test_auth"      set "EXTRA_SRCS="%PROJECT_ROOT%\src\LudeloCore\Auth\PSNAuth.cpp" "%PROJECT_ROOT%\src\LudeloCore\Auth\Keychain.cpp" "%PROJECT_ROOT%\src\LudeloCore\Net\HttpClient.cpp" "%PROJECT_ROOT%\src\LudeloCore\Net\TLSSocket.cpp" "%PROJECT_ROOT%\src\LudeloCore\Net\TCPSocket.cpp""
        if "%%T"=="test_crypto"    set "EXTRA_SRCS="%PROJECT_ROOT%\src\LudeloCore\Crypto\ECDHKeyExchange.cpp" "%PROJECT_ROOT%\src\LudeloCore\Crypto\RPCrypt.cpp" "%PROJECT_ROOT%\src\LudeloCore\Crypto\SecureRandom.cpp""
        if "%%T"=="test_discovery" set "EXTRA_SRCS="%PROJECT_ROOT%\src\LudeloCore\Discovery\DDPDiscovery.cpp" "%PROJECT_ROOT%\src\LudeloCore\Discovery\ConsoleRegistry.cpp" "%PROJECT_ROOT%\src\LudeloCore\Auth\Keychain.cpp" "%PROJECT_ROOT%\src\LudeloCore\Net\UDPSocket.cpp""
        if "%%T"=="test_dualsense" set "EXTRA_SRCS="%PROJECT_ROOT%\src\LudeloCore\Input\DualSenseHID.cpp""
        if "%%T"=="test_fec"       set "EXTRA_SRCS="%PROJECT_ROOT%\src\LudeloCore\Stream\FECDecoder.cpp""
        if "%%T"=="test_takion"    set "EXTRA_SRCS="

        cl !CFLAGS! !INCLUDES! /Fo"!TESTS_OBJ_DIR!/" /Fe"%TESTS_BIN_DIR%\%%T.exe" ^
           "%PROJECT_ROOT%\tests\%%T.cpp" !EXTRA_SRCS! ^
           /link !LIBPATHS_T! !COMMON_LIBS!

        if !ERRORLEVEL! equ 0 (
            echo        -- [OK] bin\tests\%%T.exe
        ) else (
            echo        -- [FAIL] Fallo compilar %%T.exe
        )
    )
)

echo.
echo ============================================================
echo   Ejecutando Suite de Tests...
echo ============================================================
set "PASSED=0"
set "FAILED=0"

for %%T in (test_fec test_discovery test_dualsense test_takion test_auth test_crypto) do (
    if exist "%TESTS_BIN_DIR%\%%T.exe" (
        "%TESTS_BIN_DIR%\%%T.exe" > nul 2>&1
        if !ERRORLEVEL! equ 0 (
            echo   [PASS] %%T
            set /a PASSED+=1
        ) else (
            echo   [FAIL] %%T
            set /a FAILED+=1
        )
    )
)

echo ------------------------------------------------------------
echo   Resultados: !PASSED! pasados, !FAILED! fallidos
echo ============================================================
exit /b 0

:: ============================================================================
::  Subrutina: Build MSIX Package
:: ============================================================================
:build_msix
echo.
echo ============================================================
echo   Empaquetando Ludelo como MSIX para Microsoft Store
echo ============================================================
echo.

:: Primero compilar la app
call :build_app
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

set "STAGING_DIR=%PROJECT_ROOT%\packaging\staging"
if exist "!STAGING_DIR!" rd /s /q "!STAGING_DIR!"
mkdir "!STAGING_DIR!"

:: Copiar ejecutable y DLLs al staging
copy /y "%BIN_DIR%\Ludelo.exe" "!STAGING_DIR!\" > nul
for %%D in ("%BIN_DIR%\*.dll") do copy /y "%%D" "!STAGING_DIR!\" > nul 2>&1

:: Copiar assets
xcopy /E /I /Y /Q "%BIN_DIR%\assets" "!STAGING_DIR!\assets" > nul 2>&1

:: Copiar manifest y assets de packaging
copy /y "%PROJECT_ROOT%\packaging\AppxManifest.xml" "!STAGING_DIR!\" > nul 2>&1
xcopy /E /I /Y /Q "%PROJECT_ROOT%\packaging\Assets" "!STAGING_DIR!\Assets" > nul 2>&1

:: Verificar que existe el manifest
if not exist "!STAGING_DIR!\AppxManifest.xml" (
    echo [ERROR] No se encontro packaging\AppxManifest.xml
    echo         Ejecuta primero la Task 13 del plan maestro para crearlo.
    exit /b 1
)

:: Buscar makeappx.exe en Windows SDK
set "MAKEAPPX="
for /d %%K in ("C:\Program Files (x86)\Windows Kits\10\bin\10.*") do (
    if exist "%%K\x64\makeappx.exe" set "MAKEAPPX=%%K\x64\makeappx.exe"
)
if not defined MAKEAPPX (
    for /d %%K in ("C:\Program Files (x86)\Windows Kits\10\bin\*") do (
        if exist "%%K\x64\makeappx.exe" set "MAKEAPPX=%%K\x64\makeappx.exe"
    )
)

if not defined MAKEAPPX (
    echo [ERROR] makeappx.exe no encontrado. Instala Windows 10/11 SDK.
    exit /b 1
)

:: Empaquetar
set "MSIX_OUTPUT=%PROJECT_ROOT%\packaging\Ludelo_1.0.0.0_x64.msix"
"!MAKEAPPX!" pack /d "!STAGING_DIR!" /p "!MSIX_OUTPUT!" /o
if !ERRORLEVEL! neq 0 (
    echo [ERROR] Fallo el empaquetado MSIX.
    exit /b 1
)

:: Firmar con certificado de desarrollo (si existe)
set "SIGNTOOL="
for /d %%K in ("C:\Program Files (x86)\Windows Kits\10\bin\10.*") do (
    if exist "%%K\x64\signtool.exe" set "SIGNTOOL=%%K\x64\signtool.exe"
)

set "DEV_CERT=%PROJECT_ROOT%\packaging\Ludelo_Dev.pfx"
if defined SIGNTOOL (
    if exist "!DEV_CERT!" (
        echo [SIGN] Firmando con certificado de desarrollo...
        "!SIGNTOOL!" sign /fd SHA256 /a /f "!DEV_CERT!" /p "ludelo_dev" "!MSIX_OUTPUT!"
        if !ERRORLEVEL! equ 0 (
            echo [SIGN] Firmado correctamente.
        ) else (
            echo [WARN] Firma con cert de dev fallo. El MSIX no esta firmado (solo valido para Store upload).
        )
    ) else (
        echo [WARN] No se encontro packaging\Ludelo_Dev.pfx
        echo        Para testing local ejecuta: packaging\create_dev_cert.ps1
        echo        Para subir a la Store no es necesario (Partner Center firma automaticamente).
    )
)

echo.
echo ============================================================
echo   [OK] MSIX generado en:
echo        !MSIX_OUTPUT!
echo ============================================================
exit /b 0

:: ============================================================================
::  Subrutina: Build Diagnostic Tool
:: ============================================================================
:build_diag
echo [DIAG] Compilando Ludelo Live Diagnostic Tool...

if not exist "%BUILD_DIR%\chiaki.lib" (
    call :build_chiaki_core
    if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
)

set "DIAG_OBJ_DIR=%BUILD_DIR%\obj_diag"
if not exist "!DIAG_OBJ_DIR!" mkdir "!DIAG_OBJ_DIR!"

set "DIAG_INC=/I "%PROJECT_ROOT%\src" /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\include" /I "%PROJECT_ROOT%\third_party\spdlog\include" /I "%PROJECT_ROOT%\third_party\json\single_include" /I "%PROJECT_ROOT%\third_party\webview2\build\native\include""
if defined OPENSSL_INC set "DIAG_INC=!DIAG_INC! !OPENSSL_INC!"
set "DIAG_LIBPATHS=/LIBPATH:"%BUILD_DIR%""
if defined OPENSSL_LIB set "DIAG_LIBPATHS=!DIAG_LIBPATHS! !OPENSSL_LIB!"

cl /nologo /std:c++latest /EHsc /utf-8 /O2 ^
   !DIAG_INC! ^
   /Fo"!DIAG_OBJ_DIR!/" ^
   /Fe"%BIN_DIR%\Ludelo_LiveDiagnostic.exe" ^
   "%PROJECT_ROOT%\tools\Ludelo_LiveDiagnostic.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Discovery\DDPDiscovery.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Net\UDPSocket.cpp" ^
   "%PROJECT_ROOT%\src\LudeloCore\Input\DualSenseHID.cpp" ^
   /link !DIAG_LIBPATHS! chiaki.lib libcrypto.lib libssl.lib crypt32.lib shell32.lib bcrypt.lib ws2_32.lib hid.lib setupapi.lib iphlpapi.lib

if !ERRORLEVEL! equ 0 (
    echo [OK] bin\Ludelo_LiveDiagnostic.exe compilado exitosamente.
) else (
    echo [ERROR] Fallo al compilar la herramienta de diagnostico.
)
exit /b !ERRORLEVEL!

:: ============================================================================
::  Subrutina: Compilar Chiaki Core (C)
:: ============================================================================
:build_chiaki_core
set "CHIAKI_OBJ_DIR=%BUILD_DIR%\obj_chiaki"
if not exist "!CHIAKI_OBJ_DIR!" mkdir "!CHIAKI_OBJ_DIR!"

echo [CHIAKI] Compilando motor Chiaki Core (nanopb + gf-complete + jerasure + chiaki C)...
set CHIAKI_INCLUDES=/I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\include" /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\nanopb" /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\jerasure\include" /I "%PROJECT_ROOT%\src\LudeloCore\Chiaki\gf-complete\include" /I "%PROJECT_ROOT%\third_party\ffmpeg\include"

set "CHIAKI_CFLAGS=/nologo /TC /O2 /MP /W3 /D_CRT_SECURE_NO_WARNINGS /D_WIN32 /DWIN32_LEAN_AND_MEAN /DPB_C99_STATIC_ASSERT /wd4244 /wd4267 /wd4018 /wd4133 /wd4090"

cl !CHIAKI_CFLAGS! !CHIAKI_INCLUDES! !OPENSSL_INC! /c /Fo"!CHIAKI_OBJ_DIR!/" ^
   "%PROJECT_ROOT%\src\LudeloCore\Chiaki\nanopb\*.c" ^
   "%PROJECT_ROOT%\src\LudeloCore\Chiaki\gf-complete\src\*.c" ^
   "%PROJECT_ROOT%\src\LudeloCore\Chiaki\jerasure\src\*.c" ^
   "%PROJECT_ROOT%\src\LudeloCore\Chiaki\src\*.c" ^
   "%PROJECT_ROOT%\src\LudeloCore\Chiaki\src\remote\remote_stubs.c"

if !ERRORLEVEL! neq 0 (
    echo [ERROR] Fallo la compilacion de objetos Chiaki Core.
    exit /b 1
)

lib /NOLOGO /OUT:"%BUILD_DIR%\chiaki.lib" "!CHIAKI_OBJ_DIR!\*.obj"
if !ERRORLEVEL! neq 0 (
    echo [ERROR] Fallo la creacion de chiaki.lib.
    exit /b 1
)
exit /b 0

:: ============================================================================
::  Ayuda
:: ============================================================================
:usage
echo.
echo Uso: build.bat [objetivo]
echo.
echo Objetivos disponibles:
echo   app     (default) Compila Ludelo en bin\Ludelo.exe
echo   tests   Compila y ejecuta la suite de pruebas en bin\tests\
echo   msix    Empaqueta Ludelo como .msix para Microsoft Store
echo   diag    Compila la herramienta de diagnostico en bin\
echo   all     Compila tanto la aplicacion como los tests
echo   clean   Elimina build\ y ejecutables generados
echo   help    Muestra este mensaje de ayuda
echo.
echo Ejemplos:
echo   build.bat              - Compila la app (modo por defecto)
echo   build.bat clean        - Limpia artefactos de compilacion
echo   build.bat all          - Compila todo incluyendo tests
echo   build.bat msix         - Genera el paquete MSIX para la Store
echo.
exit /b 0
