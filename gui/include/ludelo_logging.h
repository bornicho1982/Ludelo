#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <string_view>
#include <memory>
#include <filesystem>
#include <QStandardPaths>
#include <QDir>
#include <QtGlobal>
#include <QMessageLogContext>

namespace ludelo::log {

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

inline void qt_spdlog_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::string category = context.category ? context.category : "qt";
    std::shared_ptr<spdlog::logger> logger;

    if (category.find("qmlbackend") != std::string::npos || category == "chiaki.gui") {
        logger = spdlog::get("qmlbackend");
    } else if (category.find("auth") != std::string::npos) {
        logger = spdlog::get("auth_classifier");
    } else if (category.find("psn") != std::string::npos) {
        logger = spdlog::get("psnaccountid");
    }

    if (!logger) {
        logger = spdlog::default_logger();
    }
    if (!logger) return;

    std::string message = msg.toStdString();
    switch (type) {
    case QtDebugMsg:
        logger->debug("[{}] {}", category, message);
        break;
    case QtInfoMsg:
        logger->info("[{}] {}", category, message);
        break;
    case QtWarningMsg:
        logger->warn("[{}] {}", category, message);
        break;
    case QtCriticalMsg:
        logger->error("[{}] {}", category, message);
        break;
    case QtFatalMsg:
        logger->critical("[{}] {}", category, message);
        break;
    }
}

inline void init_logging() {
    if (spdlog::get("ludelo")) {
        return;
    }

    QString base_dir = QString::fromLocal8Bit(qgetenv("APPDATA"));
    if (base_dir.isEmpty()) {
        base_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    } else {
        base_dir += "/Ludelo";
    }
    QDir dir(base_dir);
    dir.mkpath("logs");
    QString log_path = dir.filePath("logs/ludelo.log");
    std::string log_file = log_path.toStdString();

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, 5 * 1024 * 1024, 3);
    file_sink->set_level(spdlog::level::debug);

    std::vector<spdlog::sink_ptr> sinks { console_sink, file_sink };

    // Default global logger (INFO)
    auto default_logger = std::make_shared<spdlog::logger>("ludelo", sinks.begin(), sinks.end());
    default_logger->set_level(spdlog::level::info);
    default_logger->flush_on(spdlog::level::debug);
    default_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
    spdlog::register_logger(default_logger);
    spdlog::set_default_logger(default_logger);

    // Specific category loggers (DEBUG)
    auto make_logger = [&](const std::string& name, spdlog::level::level_enum lvl) {
        auto l = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        l->set_level(lvl);
        l->flush_on(spdlog::level::debug);
        l->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        spdlog::register_logger(l);
        return l;
    };

    make_logger("qmlbackend", spdlog::level::debug);
    make_logger("auth_classifier", spdlog::level::debug);
    make_logger("psnaccountid", spdlog::level::debug);

    qInstallMessageHandler(qt_spdlog_handler);

    default_logger->info("==================================================");
    default_logger->info("Ludelo logging initialized: {}", log_file);
    default_logger->info("==================================================");
}

} // namespace ludelo::log

