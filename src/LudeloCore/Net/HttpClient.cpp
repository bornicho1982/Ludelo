#include "LudeloCore/Net/HttpClient.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <regex>

namespace ludelo::net {

Result<HttpResponse> HttpClient::get(const std::string& url, const std::map<std::string, std::string>& headers) {
    return request("GET", url, headers, "");
}

Result<HttpResponse> HttpClient::post(const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body) {
    return request("POST", url, headers, body);
}

bool HttpClient::parse_url(const std::string& url, std::string& host, std::string& path, uint16_t& port) {
    std::regex url_regex(R"(^https?://([^/:]+)(?::(\d+))?(/.*)?$)");
    std::smatch url_match_result;
    
    if (std::regex_match(url, url_match_result, url_regex)) {
        host = url_match_result[1];
        port = url_match_result[2].length() > 0 ? static_cast<uint16_t>(std::stoi(url_match_result[2])) : 443;
        path = url_match_result[3].length() > 0 ? url_match_result[3].str() : "/";
        return true;
    }
    return false;
}

Result<HttpResponse> HttpClient::request(const std::string& method, const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body) {
    return request_internal(method, url, headers, body, 5); // 5 max redirects
}

Result<HttpResponse> HttpClient::request_internal(const std::string& method, const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body, int redirect_limit) {
    if (redirect_limit <= 0) return std::unexpected(Error{ErrorCode::NetworkError, "Too many redirects"});

    std::string host, path;
    uint16_t port;
    if (!parse_url(url, host, path, port)) {
        return std::unexpected(Error{ErrorCode::NetworkError, "Invalid URL"});
    }

    TLSSocket socket;
    if (auto res = socket.connect(host, port); !res) {
        return std::unexpected(res.error());
    }

    std::ostringstream req;
    req << method << " " << path << " HTTP/1.1\r\n";
    req << "Host: " << host << "\r\n";
    req << "Connection: close\r\n";
    if (!body.empty()) {
        req << "Content-Length: " << body.size() << "\r\n";
    }
    for (const auto& [k, v] : headers) {
        req << k << ": " << v << "\r\n";
    }
    req << "\r\n";
    req << body;

    std::string req_str = req.str();
    if (auto res = socket.send_all(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(req_str.data()), req_str.size())); !res) {
        return std::unexpected(res.error());
    }

    std::string response_data;
    while (true) {
        auto res = socket.receive(4096);
        if (!res) {
            if (res.error().code == ErrorCode::NetworkClosed) break;
            return std::unexpected(res.error());
        }
        response_data.append(reinterpret_cast<const char*>(res->data()), res->size());
    }

    HttpResponse response;
    size_t header_end = response_data.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return std::unexpected(Error{ErrorCode::NetworkError, "Invalid HTTP response"});
    }

    std::string header_part = response_data.substr(0, header_end);
    std::string body_part = response_data.substr(header_end + 4);

    std::istringstream header_stream(header_part);
    std::string line;
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue; // skip leading blank lines if any
        
        std::regex status_regex(R"(HTTP/\d(?:\.\d)?\s+(\d{3})(?:\s*.*)?)", std::regex::icase);
        std::smatch status_match;
        if (std::regex_match(line, status_match, status_regex)) {
            response.status_code = std::stoi(status_match[1]);
        } else {
            spdlog::warn("HttpClient: failed to parse status line: '{}'", line);
        }
        break;
    }

    bool chunked = false;
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            size_t start = value.find_first_not_of(" \t");
            if (start != std::string::npos) value = value.substr(start);
            
            // Normalize key to lowercase
            std::string key_lower = key;
            std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
            
            response.headers[key_lower] = value;
            
            if (key_lower == "transfer-encoding" && value.find("chunked") != std::string::npos) {
                chunked = true;
            }
        }
    }

    if (chunked) {
        std::string decoded_body;
        size_t pos = 0;
        while (pos < body_part.size()) {
            size_t line_end = body_part.find("\r\n", pos);
            if (line_end == std::string::npos) break;
            std::string hex_len = body_part.substr(pos, line_end - pos);
            try {
                size_t chunk_len = std::stoull(hex_len, nullptr, 16);
                if (chunk_len == 0) break;
                pos = line_end + 2;
                if (pos + chunk_len > body_part.size()) break;
                decoded_body.append(body_part.substr(pos, chunk_len));
                pos += chunk_len + 2; // skip \r\n
            } catch (...) {
                break; // Invalid chunk length
            }
        }
        response.body = decoded_body;
    } else {
        response.body = body_part;
    }

    // Follow redirects
    if (response.status_code >= 300 && response.status_code < 400 && response.headers.count("location")) {
        std::string new_url = response.headers["location"];
        if (new_url.find("http") == 0 || new_url.find("/") == 0) {
            if (new_url.find("/") == 0) {
                new_url = "https://" + host + new_url;
            }
            return request_internal(method, new_url, headers, body, redirect_limit - 1);
        } else {
            // It's a custom scheme redirect (e.g. com.playstation...), don't follow, just return the 302 response
            return response;
        }
    }

    return response;
}

} // namespace ludelo::net
