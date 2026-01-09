/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Logger - Centralized logging configuration for client
*/

#include "core/Logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

namespace client::logging {

    std::shared_ptr<spdlog::logger> Logger::s_networkLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_engineLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_graphicsLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_sceneLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_uiLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_bootLogger = nullptr;

    void Logger::init() {
        try {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/client.log", 1024 * 1024 * 10, 3);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");

            std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

            s_networkLogger = std::make_shared<spdlog::logger>("Network", sinks.begin(), sinks.end());
            s_networkLogger->set_level(spdlog::level::debug);

            s_engineLogger = std::make_shared<spdlog::logger>("Engine", sinks.begin(), sinks.end());
            s_engineLogger->set_level(spdlog::level::info);

            s_graphicsLogger = std::make_shared<spdlog::logger>("Graphics", sinks.begin(), sinks.end());
            s_graphicsLogger->set_level(spdlog::level::info);

            s_sceneLogger = std::make_shared<spdlog::logger>("Scene", sinks.begin(), sinks.end());
            s_sceneLogger->set_level(spdlog::level::info);

            s_uiLogger = std::make_shared<spdlog::logger>("UI", sinks.begin(), sinks.end());
            s_uiLogger->set_level(spdlog::level::info);

            s_bootLogger = std::make_shared<spdlog::logger>("Boot", sinks.begin(), sinks.end());
            s_bootLogger->set_level(spdlog::level::info);

            spdlog::register_logger(s_networkLogger);
            spdlog::register_logger(s_engineLogger);
            spdlog::register_logger(s_graphicsLogger);
            spdlog::register_logger(s_sceneLogger);
            spdlog::register_logger(s_uiLogger);
            spdlog::register_logger(s_bootLogger);

            spdlog::set_level(spdlog::level::debug);
            spdlog::flush_on(spdlog::level::warn);

            s_bootLogger->info("Client logging system initialized");

        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }

    void Logger::shutdown() {
        if (s_bootLogger) {
            s_bootLogger->info("Shutting down logging system");
        }
        s_networkLogger.reset();
        s_engineLogger.reset();
        s_graphicsLogger.reset();
        s_sceneLogger.reset();
        s_uiLogger.reset();
        s_bootLogger.reset();
        spdlog::shutdown();
    }

    std::shared_ptr<spdlog::logger> Logger::getNetworkLogger() {
        return s_networkLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getEngineLogger() {
        return s_engineLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getGraphicsLogger() {
        return s_graphicsLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getSceneLogger() {
        return s_sceneLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getUILogger() {
        return s_uiLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getBootLogger() {
        return s_bootLogger;
    }

}