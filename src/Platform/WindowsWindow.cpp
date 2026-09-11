// Archivo: src/Platform/WindowsWindow.cpp
#include "Platform/WindowsWindow.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <dwmapi.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <spdlog/spdlog.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

// DWM backdrop type enum (requires Windows 11 SDK / build 22000+)
// Defined here to avoid SDK version dependency
#ifndef DWMWA_COLOR_NONE
enum DWM_SYSTEMBACKDROP_TYPE {
    DWMSBT_AUTO            = 0,
    DWMSBT_NONE            = 1,
    DWMSBT_MAINWINDOW      = 2,  // Mica
    DWMSBT_TRANSIENTWINDOW = 3,  // Acrylic
    DWMSBT_TABBEDWINDOW    = 4,  // Mica Alt
};
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace portal::platform {

std::string WindowsWindow::get_exe_directory() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string result(path);
    auto pos = result.find_last_of("\\/");
    return (pos != std::string::npos) ? result.substr(0, pos) : result;
}

float WindowsWindow::get_dpi_scale() {
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return static_cast<float>(dpi) / 96.0f;
    }
    return 1.0f;
}

void WindowsWindow::show_error_dialog(const std::string& title, const std::string& message) {
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

void WindowsWindow::open_url(const std::string& url) {
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

bool WindowsWindow::is_compositor_mode() {
    // Check if Steam overlay is active or SteamOS Game Mode
    return GetEnvironmentVariableA("SteamGameId", nullptr, 0) > 0 ||
           GetEnvironmentVariableA("GAMESCOPE_WAYLAND_DISPLAY", nullptr, 0) > 0;
}

std::string WindowsWindow::get_gpu_name() {
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return "Unknown GPU";
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        return "Unknown GPU";
    }

    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    char name[128];
    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), nullptr, nullptr);
    return std::string(name);
}

bool WindowsWindow::is_hdr_display_available() {
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(0, &adapter))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    if (FAILED(adapter->EnumOutputs(0, &output))) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
    if (FAILED(output.As(&output6))) {
        return false;
    }

    DXGI_OUTPUT_DESC1 desc;
    if (FAILED(output6->GetDesc1(&desc))) {
        return false;
    }

    bool hdr = (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
    spdlog::info("HDR display check: {} (color space: {})", hdr ? "available" : "not available",
        static_cast<int>(desc.ColorSpace));
    return hdr;
}

// ─── Mica Backdrop (Windows 11 build 22000+) ──────────────

bool WindowsWindow::apply_mica_backdrop(void* hwnd_ptr) {
    if (!hwnd_ptr) return false;
    HWND hwnd = static_cast<HWND>(hwnd_ptr);

    // Verify we are running on Windows 11 build 22000 or later
    OSVERSIONINFOEXW ovi{};
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    // RtlGetVersion is the reliable way — GetVersionEx lies in packaged apps
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOEXW*);
    static RtlGetVersionFn RtlGetVersion = nullptr;
    if (!RtlGetVersion) {
        RtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
            GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    }
    if (RtlGetVersion) RtlGetVersion(&ovi);

    bool win11 = (ovi.dwBuildNumber >= 22000);
    spdlog::info("[Mica] OS Build: {} — Windows 11: {}", ovi.dwBuildNumber, win11);

    // Force dark title bar regardless of OS version (requires build 18985+ / Win10 20H1)
    BOOL use_dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));

    if (!win11) {
        // Pre-Win11 fallback: extend glass frame to cover entire client area
        // This gives a frosted effect on Win10 with the Aero theme
        MARGINS margins{-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        spdlog::info("[Mica] Fallback: DWM extended frame (Win10 frosted effect)");
        return false;
    }

    // Extend frame into the entire client area so Mica shows through
    MARGINS margins{-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Apply Mica (DWMSBT_MAINWINDOW = 2)
    DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_MAINWINDOW;
    HRESULT hr = DwmSetWindowAttribute(hwnd,
        DWMWA_SYSTEMBACKDROP_TYPE,
        &backdrop,
        sizeof(backdrop));

    if (SUCCEEDED(hr)) {
        spdlog::info("[Mica] Mica backdrop applied successfully (DWMSBT_MAINWINDOW)");
        return true;
    }

    // Some early Win11 builds (22000-22500) use the older DWMWA_MICA_EFFECT (1029)
    constexpr DWORD DWMWA_MICA_EFFECT_LEGACY = 1029;
    BOOL mica_on = TRUE;
    hr = DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT_LEGACY, &mica_on, sizeof(mica_on));
    if (SUCCEEDED(hr)) {
        spdlog::info("[Mica] Mica applied via legacy attribute (DWMWA_MICA_EFFECT)");
        return true;
    }

    spdlog::warn("[Mica] DwmSetWindowAttribute DWMWA_SYSTEMBACKDROP_TYPE failed: 0x{:08X}", hr);
    return false;
}

void WindowsWindow::remove_mica_backdrop(void* hwnd_ptr) {
    if (!hwnd_ptr) return;
    HWND hwnd = static_cast<HWND>(hwnd_ptr);

    DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_NONE;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

    MARGINS margins{0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

}  // namespace portal::platform
