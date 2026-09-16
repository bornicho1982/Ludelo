#pragma once
#include <string>
#include <string_view>
#include <cctype>
#include <vector>
#include <algorithm>

namespace ludelo::log {

// Obfuscate secret: first 4 characters + "****"
inline std::string mask_secret(const std::string& s) {
    if (s.empty()) return "****";
    if (s.size() <= 4) return "****";
    return s.substr(0, 4) + "****";
}

// Redact sensitive URLs:
// 1. Path: /2.0/oauth/token/<access_token> -> /2.0/oauth/token/<first_4>****
// 2. Query parameters: code, access_token, refresh_token, userid, user_id, useruuid, user_uuid, npsso
inline std::string mask_url_secrets(const std::string& url) {
    std::string s = url;

    // 1. Check path token: /2.0/oauth/token/<token>
    const std::string token_path_marker = "/2.0/oauth/token/";
    size_t marker_pos = s.find(token_path_marker);
    if (marker_pos != std::string::npos) {
        size_t token_start = marker_pos + token_path_marker.length();
        size_t token_end = s.find_first_of("? /&#", token_start);
        if (token_end == std::string::npos) token_end = s.length();
        if (token_end > token_start) {
            std::string token = s.substr(token_start, token_end - token_start);
            std::string masked = mask_secret(token);
            s.replace(token_start, token_end - token_start, masked);
        }
    }

    // 2. Query / param masking for each sensitive key
    const char* keys[] = {
        "access_token=", "refresh_token=", "code=", "user_id=", "userid=", "user_uuid=", "useruuid=", "npsso="
    };

    for (const char* k : keys) {
        size_t pos = 0;
        size_t k_len = std::string_view(k).length();
        while ((pos = s.find(k, pos)) != std::string::npos) {
            if (pos > 0 && s[pos - 1] != '?' && s[pos - 1] != '&' && s[pos - 1] != ' ' && s[pos - 1] != '\n' && s[pos - 1] != '"') {
                pos += k_len;
                continue;
            }
            size_t val_start = pos + k_len;
            size_t val_end = s.find_first_of("& \r\n\"'#", val_start);
            if (val_end == std::string::npos) val_end = s.length();
            std::string val = s.substr(val_start, val_end - val_start);
            std::string masked = mask_secret(val);
            s.replace(val_start, val_end - val_start, masked);
            pos = val_start + masked.length();
        }
    }

    return s;
}

// Redact JSON sensitive values: "key": "value" -> "key": "first4****"
inline std::string mask_json_secrets(const std::string& json_str) {
    std::string s = json_str;
    const char* keys[] = {
        "\"access_token\"", "\"refresh_token\"", "\"code\"", "\"user_id\"", "\"userid\"", "\"user_uuid\"", "\"useruuid\"", "\"npsso\""
    };

    for (const char* k : keys) {
        size_t pos = 0;
        size_t k_len = std::string_view(k).length();
        while ((pos = s.find(k, pos)) != std::string::npos) {
            size_t colon_pos = s.find(':', pos + k_len);
            if (colon_pos == std::string::npos) break;
            size_t val_start = s.find_first_not_of(" \t\r\n", colon_pos + 1);
            if (val_start == std::string::npos) break;

            if (s[val_start] == '"') {
                val_start++; // skip opening quote
                size_t val_end = s.find('"', val_start);
                if (val_end != std::string::npos) {
                    std::string val = s.substr(val_start, val_end - val_start);
                    std::string masked = mask_secret(val);
                    s.replace(val_start, val_end - val_start, masked);
                    pos = val_start + masked.length();
                    continue;
                }
            } else if (std::isdigit(static_cast<unsigned char>(s[val_start]))) {
                size_t val_end = s.find_first_of(", \t\r\n}", val_start);
                if (val_end == std::string::npos) val_end = s.length();
                std::string val = s.substr(val_start, val_end - val_start);
                std::string masked = mask_secret(val);
                s.replace(val_start, val_end - val_start, masked);
                pos = val_start + masked.length();
                continue;
            }
            pos += k_len;
        }
    }
    return s;
}

// General log sanitizer: redacts bodies, URLs, Bearer headers, and sensitive key-value pairs
inline std::string sanitize_log_message(const std::string& msg) {
    std::string s = msg;

    // Suppress raw Response Body and Request Body from logs
    const std::string req_body_tag = "Request Body:";
    size_t req_pos = s.find(req_body_tag);
    if (req_pos != std::string::npos) {
        s = s.substr(0, req_pos + req_body_tag.length()) + " [REDACTED]";
        return s;
    }
    const std::string res_body_tag = "Response Body:";
    size_t res_pos = s.find(res_body_tag);
    if (res_pos != std::string::npos) {
        s = s.substr(0, res_pos + res_body_tag.length()) + " [REDACTED]";
        return s;
    }

    // Mask URLs
    s = mask_url_secrets(s);

    // Mask JSON patterns
    s = mask_json_secrets(s);

    // Mask Bearer tokens
    const std::string bearer_tag = "Bearer ";
    size_t b_pos = 0;
    while ((b_pos = s.find(bearer_tag, b_pos)) != std::string::npos) {
        size_t val_start = b_pos + bearer_tag.length();
        size_t val_end = s.find_first_of(" \r\n\"'", val_start);
        if (val_end == std::string::npos) val_end = s.length();
        std::string val = s.substr(val_start, val_end - val_start);
        std::string masked = mask_secret(val);
        s.replace(val_start, val_end - val_start, masked);
        b_pos = val_start + masked.length();
    }

    return s;
}

// Scan log text and check if any known cleartext secret is leaked
inline bool scan_log_for_leaks(const std::string& log_text,
                               const std::vector<std::string>& known_secrets,
                               std::string* out_leak_found = nullptr) {
    for (const auto& secret : known_secrets) {
        if (secret.empty() || secret.size() <= 4) continue;
        if (log_text.find(secret) != std::string::npos) {
            if (out_leak_found) *out_leak_found = secret;
            return true; // Leak detected!
        }
    }
    return false; // Clean!
}

// Scans log text for patterns like access_token=..., "refresh_token": ..., user_id: ...
// Returns true ONLY if all found credential fields are either masked (end in ****) or [REDACTED].
inline bool has_no_cleartext_credentials(const std::string& log_text) {
    const char* cred_keys[] = {
        "\"access_token\"", "access_token",
        "\"refresh_token\"", "refresh_token",
        "\"user_id\"", "user_id",
        "\"userid\"", "userid"
    };

    for (const char* key : cred_keys) {
        size_t pos = 0;
        size_t klen = std::string_view(key).length();
        while ((pos = log_text.find(key, pos)) != std::string::npos) {
            size_t next_pos = pos + klen;
            size_t newline = log_text.find_first_of("\r\n", next_pos);
            size_t colon_or_eq = log_text.find_first_of(":=", next_pos);
            
            if (colon_or_eq == std::string::npos || (newline != std::string::npos && colon_or_eq > newline)) {
                pos = next_pos;
                continue;
            }
            
            bool valid_separator = true;
            for (size_t i = next_pos; i < colon_or_eq; ++i) {
                char ch = log_text[i];
                if (ch != ' ' && ch != '\t' && ch != '"' && ch != '\'') {
                    valid_separator = false;
                    break;
                }
            }
            if (!valid_separator) {
                pos = next_pos;
                continue;
            }

            size_t val_start = log_text.find_first_not_of(" \t\"'", colon_or_eq + 1);
            if (val_start == std::string::npos || (newline != std::string::npos && val_start > newline)) {
                pos = next_pos;
                continue;
            }
            size_t val_end = log_text.find_first_of(" \t\r\n\"',;}&", val_start);
            if (val_end == std::string::npos) val_end = log_text.length();
            std::string val = log_text.substr(val_start, val_end - val_start);
            
            // Value is safe if it is "[REDACTED]", or ends with "****", or is empty
            if (val != "[REDACTED]" && !val.empty()) {
                if (val.size() < 4 || val.substr(val.size() - 4) != "****") {
                    // Leak detected: value is in the clear!
                    return false;
                }
            }
            pos = val_end;
        }
    }
    return true;
}

} // namespace ludelo::log

#ifdef QT_CORE_LIB
#include <QString>

namespace ludelo::log {
inline QString sanitize_psn_url(const QString& url) {
    return QString::fromStdString(mask_url_secrets(url.toStdString()));
}
inline QString mask_secret_qstr(const QString& s) {
    return QString::fromStdString(mask_secret(s.toStdString()));
}
inline QString sanitize_log_message_qstr(const QString& msg) {
    return QString::fromStdString(sanitize_log_message(msg.toStdString()));
}
} // namespace ludelo::log
#endif
