// Archivo: src/LudeloCore/Cloud/GaikaiClient.h
#pragma once

#include <string>
#include <vector>
#include <expected>
#include <cstdint>
#include <optional>
#include <memory>

#include "LudeloCore/Common.h"

namespace ludelo::cloud {

struct CloudGame {
    std::string game_id;
    std::string title;
    std::string cover_url;
    std::string platform; // PS4 or PS5
    std::vector<std::string> genres;
};

struct CloudStreamConfig {
    int video_bitrate;
    int fps;
    std::string resolution;
};

struct CloudSession {
    std::string session_id;
    std::string streaming_endpoint;
    std::string turn_server;
    std::string turn_username;
    std::string turn_password;
    CloudGame game_info;
};

class GaikaiClient {
public:
    GaikaiClient();
    ~GaikaiClient();

    ludelo::Result<CloudSession> create_session(const std::string& access_token, const std::string& game_id, const CloudStreamConfig& config);
    ludelo::Result<std::vector<CloudGame>> get_catalog(const std::string& access_token);
    ludelo::Result<bool> check_entitlement(const std::string& access_token);
    ludelo::VoidResult end_session(const std::string& session_id);

private:
    std::string m_kamaji_base_url{"https://psnow.playstation.com"};
};

} // namespace ludelo::cloud
