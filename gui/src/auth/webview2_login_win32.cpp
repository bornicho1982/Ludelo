#ifdef _WIN32
#include "auth/webview2_login_win32.h"
#include "auth_classifier.h"
#include "ludelo_logging.h"

#include <windows.h>
#include <wrl/client.h>
#include <WebView2.h>
#include <spdlog/spdlog.h>
#include <shlobj.h>
#include <filesystem>
#include <optional>
#include <vector>
#include <functional>

using Microsoft::WRL::ComPtr;

namespace ludelo::auth {

// ── Callback Infrastructure for MinGW ──
template <typename TInterface> struct CallbackTraits;

template <> struct CallbackTraits<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> {
    using Sig = HRESULT(HRESULT, ICoreWebView2Environment*);
    static const IID& iid() { return IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler; }
};

template <> struct CallbackTraits<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> {
    using Sig = HRESULT(HRESULT, ICoreWebView2Controller*);
    static const IID& iid() { return IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler; }
};

template <> struct CallbackTraits<ICoreWebView2NavigationStartingEventHandler> {
    using Sig = HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*);
    static const IID& iid() { return IID_ICoreWebView2NavigationStartingEventHandler; }
};

template <> struct CallbackTraits<ICoreWebView2NavigationCompletedEventHandler> {
    using Sig = HRESULT(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*);
    static const IID& iid() { return IID_ICoreWebView2NavigationCompletedEventHandler; }
};

template <> struct CallbackTraits<ICoreWebView2ExecuteScriptCompletedHandler> {
    using Sig = HRESULT(HRESULT, PCWSTR);
    static const IID& iid() { return IID_ICoreWebView2ExecuteScriptCompletedHandler; }
};

template <typename TInterface, typename Sig> class CallbackImpl;

template <typename TInterface, typename R, typename... Args>
class CallbackImpl<TInterface, R(Args...)> : public TInterface {
    LONG m_refCount{1};
    std::function<R(Args...)> m_func;
public:
    template <typename F>
    CallbackImpl(F&& f) : m_func(std::forward<F>(f)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (!ppvObject) return E_POINTER;
        if (riid == IID_IUnknown || riid == CallbackTraits<TInterface>::iid()) {
            *ppvObject = static_cast<TInterface*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) delete this;
        return count;
    }

    R STDMETHODCALLTYPE Invoke(Args... args) override {
        return m_func(args...);
    }
};

template <typename TInterface, typename F>
ComPtr<TInterface> Callback(F&& func) {
    using Sig = typename CallbackTraits<TInterface>::Sig;
    return ComPtr<TInterface>(new CallbackImpl<TInterface, Sig>(std::forward<F>(func)));
}

// ── State for Single Login Session ──
static std::optional<std::string> g_auth_code = std::nullopt;
static bool g_sony_error = false;
static std::string g_sony_error_msg;
static HWND g_hwnd = nullptr;
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

typedef HRESULT (STDAPICALLTYPE *CreateCoreWebView2EnvironmentWithOptionsFn)(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    ICoreWebView2EnvironmentOptions* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

typedef HRESULT (STDAPICALLTYPE *GetAvailableCoreWebView2BrowserVersionStringFn)(
    PCWSTR browserExecutableFolder,
    LPWSTR* versionInfo);

static HMODULE load_webview2_loader() {
    HMODULE h = LoadLibraryW(L"WebView2Loader.dll");
    if (h) return h;

    wchar_t exe_path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exe_path, MAX_PATH) > 0) {
        std::filesystem::path p(exe_path);
        std::filesystem::path dll_path = p.parent_path() / L"WebView2Loader.dll";
        h = LoadLibraryW(dll_path.wstring().c_str());
        if (h) return h;
    }
    return nullptr;
}

static bool try_install_webview2_bootstrapper() {
    spdlog::info("[auth] WebView2 runtime not found, attempting silent download and install of bootstrapper...");

    wchar_t temp_path[MAX_PATH];
    if (GetTempPathW(MAX_PATH, temp_path) == 0) {
        spdlog::warn("[auth] GetTempPathW failed");
        return false;
    }
    std::wstring installer_path = std::wstring(temp_path) + L"MicrosoftEdgeWebview2Setup.exe";

    HMODULE hUrlmon = LoadLibraryW(L"urlmon.dll");
    if (!hUrlmon) {
        spdlog::warn("[auth] LoadLibraryW(urlmon.dll) failed");
        return false;
    }

    typedef HRESULT (WINAPI *URLDownloadToFileW_fn)(LPUNKNOWN, LPCWSTR, LPCWSTR, DWORD, LPBINDSTATUSCALLBACK);
    auto pfnDownload = reinterpret_cast<URLDownloadToFileW_fn>(GetProcAddress(hUrlmon, "URLDownloadToFileW"));
    if (!pfnDownload) {
        FreeLibrary(hUrlmon);
        spdlog::warn("[auth] URLDownloadToFileW proc not found");
        return false;
    }

    const wchar_t* bootstrapper_url = L"https://go.microsoft.com/fwlink/p/?LinkId=2124703";
    HRESULT dl_hr = pfnDownload(nullptr, bootstrapper_url, installer_path.c_str(), 0, nullptr);
    FreeLibrary(hUrlmon);

    if (FAILED(dl_hr)) {
        spdlog::warn("[auth] Failed to download bootstrapper (HRESULT: 0x{:08X})", static_cast<uint32_t>(dl_hr));
        return false;
    }

    spdlog::info("[auth] Bootstrapper downloaded, executing silent install...");

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"\"" + installer_path + L"\" /silent /install";
    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(0);

    if (!CreateProcessW(nullptr, cmd_buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        spdlog::warn("[auth] Failed to launch bootstrapper (GetLastError: {})", GetLastError());
        DeleteFileW(installer_path.c_str());
        return false;
    }

    DWORD wait_res = WaitForSingleObject(pi.hProcess, 45000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteFileW(installer_path.c_str());

    return (wait_res == WAIT_OBJECT_0);
}

WebView2LoginResult WebView2LoginWin32::login() {
    g_auth_code = std::nullopt;
    g_sony_error = false;
    g_sony_error_msg.clear();
    g_controller = nullptr;

    HRESULT com_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_hr) && com_hr != RPC_E_CHANGED_MODE) {
        spdlog::error("[auth] CoInitializeEx failed (HRESULT: 0x{:08X})", (uint32_t)com_hr);
        return { WebView2LoginStatus::Unavailable, "", "CoInitializeEx failed" };
    }
    bool com_owned = SUCCEEDED(com_hr);

    HMODULE hLoader = load_webview2_loader();
    if (!hLoader) {
        spdlog::error("[auth] Could not load WebView2Loader.dll");
        if (com_owned) CoUninitialize();
        return { WebView2LoginStatus::Unavailable, "", "WebView2Loader.dll no disponible" };
    }

    auto pfnGetVer = (GetAvailableCoreWebView2BrowserVersionStringFn)GetProcAddress(hLoader, "GetAvailableCoreWebView2BrowserVersionString");
    auto pfnCreateEnv = (CreateCoreWebView2EnvironmentWithOptionsFn)GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");

    if (!pfnGetVer || !pfnCreateEnv) {
        spdlog::error("[auth] Function pointers not found in WebView2Loader.dll");
        FreeLibrary(hLoader);
        if (com_owned) CoUninitialize();
        return { WebView2LoginStatus::Unavailable, "", "Funciones de WebView2 no encontradas" };
    }

    // Check runtime version
    {
        LPWSTR version = nullptr;
        HRESULT ver_hr = pfnGetVer(nullptr, &version);
        if (SUCCEEDED(ver_hr) && version && wcslen(version) > 0) {
            char buf[256] = {};
            WideCharToMultiByte(CP_UTF8, 0, version, -1, buf, sizeof(buf), NULL, NULL);
            spdlog::info("[auth] WebView2 runtime detected: version={}", buf);
            CoTaskMemFree(version);
        } else {
            spdlog::warn("[auth] Runtime not detected initially, attempting silent bootstrap...");
            if (!try_install_webview2_bootstrapper()) {
                spdlog::error("[auth] No WebView2 runtime available on system");
                FreeLibrary(hLoader);
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
        spdlog::error("[auth] CreateWindowW failed (GetLastError: {})", GetLastError());
        FreeLibrary(hLoader);
        if (com_owned) CoUninitialize();
        return { WebView2LoginStatus::Unavailable, "", "CreateWindowW failed" };
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    spdlog::info("[auth] WebView2 window shown");

    // UserDataFolder: %APPDATA%\Ludelo\webview2
    std::wstring user_data_folder;
    {
        wchar_t* appdata = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
            std::filesystem::path p = std::filesystem::path(appdata) / L"Ludelo" / L"webview2";
            std::filesystem::create_directories(p);
            user_data_folder = p.wstring();
            CoTaskMemFree(appdata);
            spdlog::info("[auth] UserDataFolder: {}", p.string());
        }
    }

    bool wv2_initialized = false;

    HRESULT hr = pfnCreateEnv(
        nullptr,
        user_data_folder.empty() ? nullptr : user_data_folder.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [&wv2_initialized](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    spdlog::error("[auth] Environment creation FAILED (HRESULT: 0x{:08X})", (uint32_t)result);
                    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                    return S_OK;
                }

                HRESULT ctrl_hr = env->CreateCoreWebView2Controller(g_hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [&wv2_initialized](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) {
                                spdlog::error("[auth] Controller creation FAILED (HRESULT: 0x{:08X})", (uint32_t)result);
                                PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                return S_OK;
                            }

                            g_controller = controller;

                            ComPtr<ICoreWebView2> webview;
                            g_controller->get_CoreWebView2(&webview);

                            RECT bounds;
                            GetClientRect(g_hwnd, &bounds);
                            g_controller->put_Bounds(bounds);
                            g_controller->put_IsVisible(TRUE);

                            // Intercept NavigationStarting
                            EventRegistrationToken nav_token;
                            webview->add_NavigationStarting(
                                Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                        PWSTR uri;
                                        args->get_Uri(&uri);
                                        std::wstring wuri = uri ? uri : L"";
                                        if (uri) CoTaskMemFree(uri);

                                        std::string suri;
                                        int sz = WideCharToMultiByte(CP_UTF8, 0, wuri.c_str(), -1, NULL, 0, NULL, NULL);
                                        if (sz > 0) {
                                            suri.resize(sz - 1);
                                            WideCharToMultiByte(CP_UTF8, 0, wuri.c_str(), -1, &suri[0], sz, NULL, NULL);
                                        }

                                        spdlog::debug("[auth] Navigating: {}", ludelo::auth::mask_url_code(suri));

                                        std::string code;
                                        std::string err;
                                        auto classification = classify_auth_url(suri, &code, &err);

                                        if (classification == AuthUrlClassification::Success) {
                                            g_auth_code = code;
                                            spdlog::info("[auth] code captured (masked): {}", ludelo::auth::mask_secret(code));
                                            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                            return S_OK;
                                        } else if (classification == AuthUrlClassification::FatalError) {
                                            g_sony_error = true;
                                            g_sony_error_msg = err.empty() ? "Sony devolvio un error en la URL" : err;
                                            spdlog::warn("[auth] Sony error detected (code={}): {}", err, g_sony_error_msg);
                                            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                            return S_OK;
                                        }
                                        return S_OK;
                                    }).Get(), &nav_token);

                            // Inspect NavigationCompleted
                            EventRegistrationToken comp_token;
                            webview->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                        BOOL success = FALSE;
                                        args->get_IsSuccess(&success);
                                        if (!success) {
                                            COREWEBVIEW2_WEB_ERROR_STATUS status;
                                            args->get_WebErrorStatus(&status);
                                            spdlog::error("[auth] NavigationCompleted error status: {}", (int)status);
                                        } else {
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
                                                                    spdlog::warn("[auth] Sony error detected in DOM: {}", res_utf8);
                                                                    g_sony_error = true;
                                                                    g_sony_error_msg = "Sony reporto error en la pagina";
                                                                    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                                                                }
                                                            }
                                                        }
                                                        return S_OK;
                                                    }).Get());
                                        }
                                        return S_OK;
                                    }).Get(), &comp_token);

                            // Navigate to Sony authorize
                            std::string auth_url_str = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id=ba495a24-818c-472b-b12d-ff231c1b5745&redirect_uri=https%3A%2F%2Fremoteplay.dl.playstation.net%2Fremoteplay%2Fredirect&scope=psn:clientapp%20referenceDataService:countryConfig.read%20pushNotification:webSocket.desktop.connect%20sessionManager:remotePlaySession.system.update&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal";
                            std::wstring w_auth_url;
                            int auth_size = MultiByteToWideChar(CP_UTF8, 0, auth_url_str.c_str(), -1, NULL, 0);
                            w_auth_url.resize(auth_size - 1);
                            MultiByteToWideChar(CP_UTF8, 0, auth_url_str.c_str(), -1, &w_auth_url[0], auth_size);

                            webview->Navigate(w_auth_url.c_str());
                            wv2_initialized = true;
                            return S_OK;
                        }).Get());

                if (FAILED(ctrl_hr)) {
                    spdlog::error("[auth] env->CreateCoreWebView2Controller FAILED (HRESULT: 0x{:08X})", (uint32_t)ctrl_hr);
                    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
                }
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        spdlog::error("[auth] pfnCreateEnv FAILED (HRESULT: 0x{:08X})", (uint32_t)hr);
        DestroyWindow(g_hwnd);
        FreeLibrary(hLoader);
        if (com_owned) CoUninitialize();
        return { WebView2LoginStatus::Unavailable, "", "CreateCoreWebView2EnvironmentWithOptions failed" };
    }

    // Modal message pump
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_controller = nullptr;
    FreeLibrary(hLoader);
    if (com_owned) CoUninitialize();

    if (g_auth_code.has_value()) {
        return { WebView2LoginStatus::Success, g_auth_code.value(), "" };
    }

    if (g_sony_error) {
        return { WebView2LoginStatus::SonyError, "", g_sony_error_msg };
    }

    return { WebView2LoginStatus::UserCancelled, "", "Ventana cerrada por el usuario" };
}

} // namespace ludelo::auth
#endif // _WIN32

