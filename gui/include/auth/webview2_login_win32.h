#pragma once
#include <string>

namespace ludelo::auth {

enum class WebView2LoginStatus {
    Success,
    UserCancelled,
    SonyError,
    Unavailable
};

struct WebView2LoginResult {
    WebView2LoginStatus status{WebView2LoginStatus::Unavailable};
    std::string code;
    std::string error_details;
};

#ifdef _WIN32
class WebView2LoginWin32 {
public:
    static WebView2LoginResult login();
};
#endif

} // namespace ludelo::auth

