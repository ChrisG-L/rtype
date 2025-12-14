/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Logger - Centralized logging configuration for server
*/

#include "infrastructure/logging/Logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

namespace server::logging {

    std::shared_ptr<spdlog::logger> Logger::s_networkLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_domainLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_gameLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_mainLogger = nullptr;

    void Logger::init() {
        try {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/server.log", 1024 * 1024 * 10, 5);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");

            std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

            s_networkLogger = std::make_shared<spdlog::logger>("Network", sinks.begin(), sinks.end());
            s_networkLogger->set_level(spdlog::level::debug);

            s_domainLogger = std::make_shared<spdlog::logger>("Domain", sinks.begin(), sinks.end());
            s_domainLogger->set_level(spdlog::level::info);

            s_gameLogger = std::make_shared<spdlog::logger>("Game", sinks.begin(), sinks.end());
            s_gameLogger->set_level(spdlog::level::info);

            s_mainLogger = std::make_shared<spdlog::logger>("Main", sinks.begin(), sinks.end());
            s_mainLogger->set_level(spdlog::level::info);

            spdlog::register_logger(s_networkLogger);
            spdlog::register_logger(s_domainLogger);
            spdlog::register_logger(s_gameLogger);
            spdlog::register_logger(s_mainLogger);

            spdlog::set_level(spdlog::level::debug);
            spdlog::flush_on(spdlog::level::warn);

            s_mainLogger->info("Server logging system initialized");

        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }

    void Logger::shutdown() {
        if (s_mainLogger) {
            s_mainLogger->info("Shutting down logging system");
        }
        spdlog::shutdown();
    }

    std::shared_ptr<spdlog::logger> Logger::getNetworkLogger() {
        return s_networkLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getDomainLogger() {
        return s_domainLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getGameLogger() {
        return s_gameLogger;
    }

    std::shared_ptr<spdlog::logger> Logger::getMainLogger() {
        return s_mainLogger;
    }

}
