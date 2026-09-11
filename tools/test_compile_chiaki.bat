@echo off
setlocal enabledelayedexpansion

set "PROJECT_ROOT=%~dp0.."
set "BUILD_DIR=%PROJECT_ROOT%\build\chiaki_test"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > nul

set INCLUDES=/I "%PROJECT_ROOT%\src\PortalCore\Chiaki\include" /I "%PROJECT_ROOT%\src\PortalCore\Chiaki\nanopb" /I "%PROJECT_ROOT%\src\PortalCore\Chiaki\jerasure\include" /I "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\include" /I "%PROJECT_ROOT%\third_party\ffmpeg\include" /I "C:\Program Files\OpenSSL-Win64\include"

set CFLAGS=/nologo /TC /W3 /O2 /D_CRT_SECURE_NO_WARNINGS /D_WIN32 /DWIN32_LEAN_AND_MEAN /DPB_C99_STATIC_ASSERT /wd4244 /wd4267 /wd4018 /wd4133 /wd4090

echo [1] Compilando nanopb...
cl !CFLAGS! !INCLUDES! /c /Fo"%BUILD_DIR%/" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\nanopb\pb_common.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\nanopb\pb_encode.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\nanopb\pb_decode.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\nanopb\takion.pb.c"

if %ERRORLEVEL% neq 0 exit /b 1

echo [2] Compilando gf-complete...
cl !CFLAGS! !INCLUDES! /c /Fo"%BUILD_DIR%/" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_wgen.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_w4.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_w8.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_w16.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_w32.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_w64.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_w128.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_rand.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_general.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\gf-complete\src\gf_cpu.c"

if %ERRORLEVEL% neq 0 exit /b 1

echo [3] Compilando jerasure...
cl !CFLAGS! !INCLUDES! /c /Fo"%BUILD_DIR%/" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\jerasure\src\galois.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\jerasure\src\jerasure.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\jerasure\src\reed_sol.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\jerasure\src\cauchy.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\jerasure\src\liberation.c"

if %ERRORLEVEL% neq 0 exit /b 1

echo [4] Compilando Chiaki core C...
cl !CFLAGS! !INCLUDES! /c /Fo"%BUILD_DIR%/" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\common.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\sock.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\session.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\thread.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\base64.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\http.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\log.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\ctrl.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\rpcrypt.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\takion.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\senkusha.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\streamconnection.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\ecdh.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\launchspec.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\random.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\gkcrypt.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\audio.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\audioreceiver.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\audiosender.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\videoreceiver.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\frameprocessor.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\pscloud_audio_reassembler.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\packetstats.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\discovery.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\congestioncontrol.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\stoppipe.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\reorderqueue.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\discoveryservice.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\feedback.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\feedbacksender.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\controller.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\takionsendbuffer.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\time.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\fec.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\regist.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\orientation.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\bitstream.c" ^
   "%PROJECT_ROOT%\src\PortalCore\Chiaki\src\ffmpegdecoder.c"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Chiaki core failed
    exit /b 1
)

echo.
echo ============================================================
echo   [OK] TODO EL MOTOR CHIAKI_LIB COMPILADO CON EXITO EN MSVC!
echo ============================================================
