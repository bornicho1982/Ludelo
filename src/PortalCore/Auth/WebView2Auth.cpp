#include "WebView2Auth.h"
#include <windows.h>
#include <wrl.h>
#include <WebView2.h>
#include <spdlog/spdlog.h>
#include <shlobj.h>
#include <filesystem>

using namespace Microsoft::WRL;

namespace portal::auth {

static std::optional<std::string> g_auth_code = std::nullopt;
static HWND g_hwnd = nullptr;
// Must survive the full lifetime of the window so the webview stays alive and paints
static ComPtr<ICoreWebView2Controller> g_controller;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_SIZE:
            if (g_controller) {
                RECT bounds;
                GetClientRect(hWnd, &bounds);
                g_controller->put_Bounds(bounds);
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

std::optional<std::string> WebView2Auth::login() {
    g_auth_code = std::nullopt;
    g_controller = nullptr;

    // ── COM must be initialized as STA on this thread ──
    HRESULT com_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_hr) && com_hr != RPC_E_CHANGED_MODE) {
        spdlog::error("WebView2Auth: CoInitializeEx failed (HRESULT: 0x{:08X})", (uint32_t)com_hr);
        return std::nullopt;
    }
    bool com_owned = SUCCEEDED(com_hr);

    // ── Check runtime version ──
    {
        LPWSTR version = nullptr;
        HRESULT ver_hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
        if (SUCCEEDED(ver_hr) && version && wcslen(version) > 0) {
            char buf[256] = {};
            WideCharToMultiByte(CP_UTF8, 0, version, -1, buf, sizeof(buf), NULL, NULL);
            spdlog::info("WebView2Auth: runtime version = {}", buf);
            CoTaskMemFree(version);
        } else {
            spdlog::error("WebView2Auth: no WebView2 runtime found (HRESULT: 0x{:08X})", (uint32_t)ver_hr);
            if (com_owned) CoUninitialize();
            return std::nullopt;
        }
    }

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"LudeloWebView2AuthClass";
    RegisterClassExW(&wcex);

    int width = 500;
    int height = 750;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowW(L"LudeloWebView2AuthClass", L"PlayStation Network Login",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        (screenW - width) / 2, (screenH - height) / 2, width, height,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hwnd) {
        spdlog::error("WebView2Auth: CreateWindowW failed (GetLastError: {})", GetLastError());
        if (com_owned) CoUninitialize();
        return std::nullopt;
    }

    // Show window BEFORE creating the environment — the controller needs a visible HWND
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    // ── UserDataFolder: writable path under %APPDATA%\Ludelo\webview2 ──
    std::wstring user_data_folder;
    {
        wchar_t* appdata = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
            std::filesystem::path p = std::filesystem::path(appdata) / L"Ludelo" / L"webview2";
            std::filesystem::create_directories(p);
            user_data_folder = p.wstring();
            CoTaskMemFree(appdata);
            spdlog::info("WebView2Auth: UserDataFolder = {}", p.string());
        } else {
            spdlog::warn("WebView2Auth: SHGetKnownFolderPath failed, using default UserDataFolder");
        }
    }

    bool wv2_initialized = false;

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        user_data_folder.empty() ? nullptr : user_data_folder.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [&wv2_initialized](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    spdlog::error("WebView2Auth: Environment callback FAILED (HRESULT: 0x{:08X})", (uint32_t)result);
                    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                    return S_OK;
                }
                spdlog::info("WebView2Auth: Environment created OK");

                HRESULT ctrl_hr = env->CreateCoreWebView2Controller(g_hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [&wv2_initialized](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            spdlog::info("WebView2Auth: Controller callback fired (HRESULT: 0x{:08X}, ptr: {})",
                                (uint32_t)result, (void*)controller);

                            if (FAILED(result) || !controller) {
                                spdlog::error("WebView2Auth: Controller creation FAILED (HRESULT: 0x{:08X})", (uint32_t)result);
                                PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                return S_OK;
                            }

                            // Store in global so it survives and WM_SIZE can resize it
                            g_controller = controller;

                            ComPtr<ICoreWebView2> webview;
                            g_controller->get_CoreWebView2(&webview);

                            // Set bounds to full client area
                            RECT bounds;
                            GetClientRect(g_hwnd, &bounds);
                            spdlog::info("WebView2Auth: put_Bounds({{0,0,{},{}}}) ", bounds.right, bounds.bottom);
                            g_controller->put_Bounds(bounds);
                            g_controller->put_IsVisible(TRUE);

                            // ── NavigationStarting: log every URL, intercept final redirect ──
                            EventRegistrationToken nav_token;
                            webview->add_NavigationStarting(
                                Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                        PWSTR uri;
                                        args->get_Uri(&uri);
                                        std::wstring wuri = uri;
                                        CoTaskMemFree(uri);

                                        // Convert to UTF-8 for logging
                                        std::string suri;
                                        int sz = WideCharToMultiByte(CP_UTF8, 0, wuri.c_str(), -1, NULL, 0, NULL, NULL);
                                        if (sz > 0) {
                                            suri.resize(sz - 1);
                                            WideCharToMultiByte(CP_UTF8, 0, wuri.c_str(), -1, &suri[0], sz, NULL, NULL);
                                            // Redact secrets
                                            std::string safe = suri;
                                            for (const char* key : {"code=", "token=", "npsso="}) {
                                                size_t p = safe.find(key);
                                                if (p != std::string::npos) {
                                                    size_t vstart = p + strlen(key);
                                                    size_t vend = safe.find("&", vstart);
                                                    if (vend == std::string::npos) safe.replace(vstart, std::string::npos, "***");
                                                    else safe.replace(vstart, vend - vstart, "***");
                                                }
                                            }
                                            spdlog::info("WebView2 nav: {}", safe);
                                        }

                                        // Only intercept the exact redirect URI WITH code=
                                        const std::wstring redirect_prefix = L"https://remoteplay.dl.playstation.net/remoteplay/redirect";
                                        if (wuri.find(redirect_prefix) == 0 && wuri.find(L"code=") != std::wstring::npos) {
                                            size_t code_pos = wuri.find(L"code=");
                                            size_t start = code_pos + 5;
                                            size_t end = wuri.find(L"&", start);
                                            std::wstring wcode = (end == std::wstring::npos)
                                                ? wuri.substr(start)
                                                : wuri.substr(start, end - start);

                                            std::string scode;
                                            int size_needed = WideCharToMultiByte(CP_UTF8, 0, wcode.c_str(), (int)wcode.length(), NULL, 0, NULL, NULL);
                                            scode.resize(size_needed);
                                            WideCharToMultiByte(CP_UTF8, 0, wcode.c_str(), (int)wcode.length(), &scode[0], size_needed, NULL, NULL);

                                            g_auth_code = scode;
                                            spdlog::info("WebView2Auth: auth code captured ({} chars)", scode.length());

                                            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                        }
                                        return S_OK;
                                    }).Get(), &nav_token);

                            // ── NavigationCompleted: log errors ──
                            EventRegistrationToken comp_token;
                            webview->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                        BOOL success = FALSE;
                                        args->get_IsSuccess(&success);
                                        if (!success) {
                                            COREWEBVIEW2_WEB_ERROR_STATUS status;
                                            args->get_WebErrorStatus(&status);
                                            spdlog::error("WebView2Auth: NavigationCompleted FAILED, WebErrorStatus={}", (int)status);
                                        } else {
                                            spdlog::info("WebView2Auth: NavigationCompleted OK");
                                        }
                                        return S_OK;
                                    }).Get(), &comp_token);

                            // ── Navigate to Sony authorize ──
                            std::string auth_url_str = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id=ba495a24-818c-472b-b12d-ff231c1b5745&redirect_uri=https%3A%2F%2Fremoteplay.dl.playstation.net%2Fremoteplay%2Fredirect&scope=psn:clientapp%20referenceDataService:countryConfig.read%20pushNotification:webSocket.desktop.connect%20sessionManager:remotePlaySession.system.update&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal";
                            spdlog::info("WebView2Auth: calling Navigate()");

                            std::wstring w_auth_url;
                            int auth_size = MultiByteToWideChar(CP_UTF8, 0, auth_url_str.c_str(), -1, NULL, 0);
                            w_auth_url.resize(auth_size - 1);
                            MultiByteToWideChar(CP_UTF8, 0, auth_url_str.c_str(), -1, &w_auth_url[0], auth_size);

                            HRESULT nav_hr = webview->Navigate(w_auth_url.c_str());
                            if (FAILED(nav_hr)) {
                                spdlog::error("WebView2Auth: Navigate() FAILED (HRESULT: 0x{:08X})", (uint32_t)nav_hr);
                                PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                            } else {
                                spdlog::info("WebView2Auth: Navigate() dispatched OK");
                            }

                            wv2_initialized = true;
                            return S_OK;
                        }).Get());

                if (FAILED(ctrl_hr)) {
                    spdlog::error("WebView2Auth: CreateCoreWebView2Controller call FAILED (HRESULT: 0x{:08X})", (uint32_t)ctrl_hr);
                    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                }

                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        spdlog::error("WebView2Auth: CreateCoreWebView2EnvironmentWithOptions FAILED (HRESULT: 0x{:08X})", (uint32_t)hr);
        DestroyWindow(g_hwnd);
        if (com_owned) CoUninitialize();
        return std::nullopt;
    }

    // ── STA message pump — drives all async callbacks ──
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up: release controller so it doesn't dangle
    g_controller = nullptr;

    if (com_owned) CoUninitialize();

    if (!wv2_initialized) {
        spdlog::warn("WebView2Auth: controller never initialized, returning nullopt");
        return std::nullopt;
    }

    if (g_auth_code.has_value()) {
        spdlog::info("WebView2Auth: login completed with code");
        return g_auth_code.value();
    }

    spdlog::info("WebView2Auth: user closed window without completing login");
    return std::string(""); // empty = user cancelled
}

} // namespace portal::auth
