#pragma once
#include <string>
#include <cctype>

namespace ludelo::auth {

enum class AuthUrlClassification {
    Continue,
    Success,
    FatalError
};

inline AuthUrlClassification classify_auth_url(const std::string& input, std::string* out_code = nullptr, std::string* out_error = nullptr) {
    if (out_code) out_code->clear();
    if (out_error) out_error->clear();

    if (input.empty()) {
        return AuthUrlClassification::Continue;
    }

    std::string lower = input;
    for (auto& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lower.find("something went wrong") != std::string::npos ||
        lower.find("an error occurred") != std::string::npos) {
        if (out_error) *out_error = "Something went wrong";
        return AuthUrlClassification::FatalError;
    }

    if (input.find("error=login_required") != std::string::npos) {
        return AuthUrlClassification::Continue;
    }

    const std::string redirect_marker = "remoteplay.dl.playstation.net/remoteplay/redirect";
    if (input.find(redirect_marker) != std::string::npos) {
        size_t code_pos = input.find("code=");
        if (code_pos != std::string::npos) {
            size_t start = code_pos + 5;
            size_t end = input.find('&', start);
            std::string code = (end == std::string::npos) ? input.substr(start) : input.substr(start, end - start);
            if (out_code) *out_code = code;
            return AuthUrlClassification::Success;
        }

        size_t err_pos = input.find("error=");
        if (err_pos != std::string::npos) {
            size_t start = err_pos + 6;
            size_t end = input.find('&', start);
            std::string err = (end == std::string::npos) ? input.substr(start) : input.substr(start, end - start);
            if (out_error) *out_error = err;
            return AuthUrlClassification::FatalError;
        }
    }

    return AuthUrlClassification::Continue;
}

inline bool classify_dom_content(const std::string& dom_content) {
    return classify_auth_url(dom_content) == AuthUrlClassification::FatalError;
}

} // namespace ludelo::auth

