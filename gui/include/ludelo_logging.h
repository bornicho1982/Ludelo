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

#include "log_redaction.h"

namespace ludelo::log {

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

    std::string message = sanitize_log_message(msg.toStdString());
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

