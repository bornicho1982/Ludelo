// Archivo: src/LudeloCore/Net/HttpClient.h
#pragma once
#include "LudeloCore/Net/TLSSocket.h"
#include <map>
#include <string>

namespace ludelo::net {

struct HttpResponse {
    int status_code = 0;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpClient {
public:
    HttpClient() = default;
    ~HttpClient() = default;

    Result<HttpResponse> get(const std::string& url, const std::map<std::string, std::string>& headers = {});
    Result<HttpResponse> post(const std::string& url, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

private:
    Result<HttpResponse> request(const std::string& method, const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body);
    static bool parse_url(const std::string& url, std::string& host, std::string& path, uint16_t& port);
    
private:
    static Result<HttpResponse> request_internal(const std::string& method, const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body, int redirect_limit);
};

} // namespace ludelo::net
