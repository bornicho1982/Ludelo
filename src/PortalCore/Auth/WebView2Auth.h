#pragma once
#include <string>

namespace portal::auth {

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

class WebView2Auth {
public:
    // Retrieves the auth code using an embedded WebView2 window.
    static WebView2LoginResult login();
};

} // namespace portal::auth
