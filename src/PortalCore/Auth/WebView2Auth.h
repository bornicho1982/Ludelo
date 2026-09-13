#pragma once
#include <string>
#include <optional>

namespace portal::auth {

class WebView2Auth {
public:
    // Retrieves the auth code using an embedded WebView2 window.
    // If WebView2 is not available, returns nullopt (caller should fallback).
    static std::optional<std::string> login();
};

} // namespace portal::auth
