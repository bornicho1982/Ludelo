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
static bool g_sony_error = false;
static std::string g_sony_error_msg;
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

static bool try_install_webview2_bootstrapper() {
    spdlog::info("WebView2Auth: runtime not found, attempting silent download and install of Microsoft WebView2 Evergreen bootstrapper...");

    wchar_t temp_path[MAX_PATH];
    if (GetTempPathW(MAX_PATH, temp_path) == 0) {
        spdlog::warn("WebView2Auth: GetTempPathW failed");
        return false;
    }
    std::wstring installer_path = std::wstring(temp_path) + L"MicrosoftEdgeWebview2Setup.exe";

    HMODULE hUrlmon = LoadLibraryW(L"urlmon.dll");
    if (!hUrlmon) {
        spdlog::warn("WebView2Auth: LoadLibraryW(urlmon.dll) failed");
        return false;
    }

    typedef HRESULT (WINAPI *URLDownloadToFileW_fn)(LPUNKNOWN, LPCWSTR, LPCWSTR, DWORD, LPBINDSTATUSCALLBACK);
    auto pfnDownload = reinterpret_cast<URLDownloadToFileW_fn>(GetProcAddress(hUrlmon, "URLDownloadToFileW"));
    if (!pfnDownload) {
        FreeLibrary(hUrlmon);
        spdlog::warn("WebView2Auth: URLDownloadToFileW proc not found");
        return false;
    }

    const wchar_t* bootstrapper_url = L"https://go.microsoft.com/fwlink/p/?LinkId=2124703";
    HRESULT dl_hr = pfnDownload(nullptr, bootstrapper_url, installer_path.c_str(), 0, nullptr);
    FreeLibrary(hUrlmon);

    if (FAILED(dl_hr)) {
        spdlog::warn("WebView2Auth: failed to download bootstrapper (HRESULT: 0x{:08X})", static_cast<uint32_t>(dl_hr));
        return false;
    }

    spdlog::info("WebView2Auth: bootstrapper downloaded to {}, executing silent install...", 
        std::filesystem::path(installer_path).string());

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"\"" + installer_path + L"\" /silent /install";
    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(0);

    if (!CreateProcessW(nullptr, cmd_buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        spdlog::warn("WebView2Auth: failed to launch bootstrapper (GetLastError: {})", GetLastError());
        DeleteFileW(installer_path.c_str());
        return false;
    }

    // Wait for installer to finish (up to 45 seconds)
    DWORD wait_res = WaitForSingleObject(pi.hProcess, 45000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteFileW(installer_path.c_str());

    if (wait_res != WAIT_OBJECT_0) {
        spdlog::warn("WebView2Auth: bootstrapper installation timed out or failed");
        return false;
    }

    LPWSTR version = nullptr;
    HRESULT ver_hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
    if (SUCCEEDED(ver_hr) && version && wcslen(version) > 0) {
        char buf[256] = {};
        WideCharToMultiByte(CP_UTF8, 0, version, -1, buf, sizeof(buf), NULL, NULL);
        spdlog::info("WebView2Auth: silent installation succeeded! Detected runtime version = {}", buf);
        CoTaskMemFree(version);
        return true;
    }

    spdlog::warn("WebView2Auth: runtime still not detected after installation attempt");
    return false;
}

WebView2LoginResult WebView2Auth::login() {
    g_auth_code = std::nullopt;
    g_sony_error = false;
    g_sony_error_msg.clear();
    g_controller = nullptr;

    // ── COM must be initialized as STA on this thread ──
    HRESULT com_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_hr) && com_hr != RPC_E_CHANGED_MODE) {
        spdlog::error("WebView2Auth: CoInitializeEx failed (HRESULT: 0x{:08X})", (uint32_t)com_hr);
        return { WebView2LoginStatus::Unavailable, "", "CoInitializeEx failed" };
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
            spdlog::warn("WebView2Auth: runtime not detected initially (HRESULT: 0x{:08X}), attempting silent bootstrap...", (uint32_t)ver_hr);
            if (!try_install_webview2_bootstrapper()) {
                spdlog::error("WebView2Auth: no WebView2 runtime available");
                if (com_owned) CoUninitialize();
                return { WebView2LoginStatus::Unavailable, "", "No WebView2 runtime available" };
            }
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
        return { WebView2LoginStatus::Unavailable, "", "CreateWindowW failed" };
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

                                        // Use classify_auth_url to handle normal signin, final redirect code, or fatal errors
                                        std::string code;
                                        std::string err;
                                        auto classification = classify_auth_url(suri, &code, &err);

                                        if (classification == AuthUrlClassification::Success) {
                                            g_auth_code = code;
                                            spdlog::info("WebView2Auth: auth code captured ({} chars)", code.length());
                                            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                            return S_OK;
                                        } else if (classification == AuthUrlClassification::FatalError) {
                                            g_sony_error = true;
                                            g_sony_error_msg = err.empty() ? "Sony devolvió un error de autenticación en la URL" : err;
                                            spdlog::warn("WebView2Auth: fatal OAuth error detected in URL: {}", g_sony_error_msg);
                                            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                            return S_OK;
                                        }
                                        // AuthUrlClassification::Continue: normal OAuth flow, await user interaction
                                        return S_OK;
                                    }).Get(), &nav_token);

                            // ── NavigationCompleted: inspect errors & DOM ──
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
                                            // Inspect DOM for Sony error indicators ("Something went wrong")
                                            sender->ExecuteScript(L"document.title + ' ' + (document.body ? document.body.innerText.substring(0, 300) : '')",
                                                Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                                                    [](HRESULT error, PCWSTR resultJson) -> HRESULT {
                                                        if (SUCCEEDED(error) && resultJson) {
                                                            std::wstring res_w = resultJson;
                                                            std::string res_utf8;
                                                            int sz = WideCharToMultiByte(CP_UTF8, 0, res_w.c_str(), -1, NULL, 0, NULL, NULL);
                                                            if (sz > 0) {
                                                                res_utf8.resize(sz - 1);
                                                                WideCharToMultiByte(CP_UTF8, 0, res_w.c_str(), -1, &res_utf8[0], sz, NULL, NULL);
                                                                if (classify_dom_content(res_utf8)) {
                                                                    spdlog::warn("WebView2Auth: Sony error page detected in DOM: {}", res_utf8);
                                                                    g_sony_error = true;
                                                                    g_sony_error_msg = "Sony reportó 'Something went wrong' en la página";
                                                                    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                                                }
                                                            }
                                                        }
                                                        return S_OK;
                                                    }).Get());
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
        return { WebView2LoginStatus::Unavailable, "", "CreateCoreWebView2EnvironmentWithOptions failed" };
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
        spdlog::warn("WebView2Auth: controller never initialized");
        return { WebView2LoginStatus::Unavailable, "", "Controlador no inicializado" };
    }

    if (g_auth_code.has_value()) {
        spdlog::info("WebView2Auth: login completed with code");
        return { WebView2LoginStatus::Success, g_auth_code.value(), "" };
    }

    if (g_sony_error) {
        spdlog::warn("WebView2Auth: login stopped due to Sony service error: {}", g_sony_error_msg);
        return { WebView2LoginStatus::SonyError, "", g_sony_error_msg };
    }

    spdlog::info("WebView2Auth: user closed window without completing login");
    return { WebView2LoginStatus::UserCancelled, "", "Ventana cerrada por el usuario" };
}

} // namespace portal::auth
