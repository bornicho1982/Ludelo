#pragma once
#include <string>
#include <cctype>
#include <spdlog/spdlog.h>

namespace ludelo::auth {

enum class AuthUrlClassification {
    Continue,
    Success,
    FatalError
};

inline std::string mask_secret(const std::string& s) {
    if (s.size() <= 8) return "****";
    return s.substr(0, 4) + "****" + s.substr(s.size() - 4);
}

inline std::string mask_url_code(const std::string& url) {
    std::string s = url;
    size_t code_pos = s.find("code=");
    if (code_pos != std::string::npos) {
        size_t start = code_pos + 5;
        size_t end = s.find('&', start);
        size_t len = (end == std::string::npos) ? (s.length() - start) : (end - start);
        std::string code = s.substr(start, len);
        std::string masked = mask_secret(code);
        s.replace(start, len, masked);
    }
    return s;
}

inline AuthUrlClassification classify_auth_url(const std::string& input, std::string* out_code = nullptr, std::string* out_error = nullptr) {
    if (out_code) out_code->clear();
    if (out_error) out_error->clear();

    auto logger = spdlog::get("auth_classifier");

    if (input.empty()) {
        if (logger) logger->debug("classify_auth_url: input empty -> Continue");
        return AuthUrlClassification::Continue;
    }

    std::string lower = input;
    for (auto& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lower.find("something went wrong") != std::string::npos ||
        lower.find("an error occurred") != std::string::npos) {
        if (out_error) *out_error = "Something went wrong";
        if (logger) logger->warn("classify_auth_url: detected fatal error message in content -> FatalError");
        return AuthUrlClassification::FatalError;
    }

    if (input.find("error=login_required") != std::string::npos) {
        if (logger) logger->debug("classify_auth_url: detected error=login_required -> Continue");
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
            if (logger) logger->info("classify_auth_url: matched redirect with code={} -> Success", mask_secret(code));
            return AuthUrlClassification::Success;
        }

        size_t err_pos = input.find("error=");
        if (err_pos != std::string::npos) {
            size_t start = err_pos + 6;
            size_t end = input.find('&', start);
            std::string err = (end == std::string::npos) ? input.substr(start) : input.substr(start, end - start);
            if (out_error) *out_error = err;
            if (logger) logger->error("classify_auth_url: matched redirect with error={} -> FatalError", err);
            return AuthUrlClassification::FatalError;
        }
    }

    if (logger) logger->debug("classify_auth_url: non-terminal URL: {} -> Continue", mask_url_code(input));
    return AuthUrlClassification::Continue;
}

inline bool classify_dom_content(const std::string& dom_content) {
    auto logger = spdlog::get("auth_classifier");
    bool is_err = classify_auth_url(dom_content) == AuthUrlClassification::FatalError;
    if (logger) logger->debug("classify_dom_content: result={}", is_err ? "FatalError" : "OK");
    return is_err;
}

} // namespace ludelo::auth

