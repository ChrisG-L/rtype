/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Logger - Centralized logging configuration for server
*/

#include "infrastructure/logging/Logger.hpp"
#include "infrastructure/tui/TUISink.hpp"
#include "infrastructure/tui/LogBuffer.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

namespace server::logging {

    std::shared_ptr<spdlog::logger> Logger::s_networkLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_domainLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_gameLogger = nullptr;
    std::shared_ptr<spdlog::logger> Logger::s_mainLogger = nullptr;
    std::shared_ptr<infrastructure::tui::TUISink_mt> Logger::s_tuiSink = nullptr;
    spdlog::level::level_enum Logger::s_previousLevel = spdlog::level::debug;
    bool Logger::s_enabled = true;
    bool Logger::s_tuiMode = false;

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

    void Logger::initWithTUI(std::shared_ptr<infrastructure::tui::LogBuffer> logBuffer) {
        try {
            // Drop existing loggers if they were already registered (e.g., by init())
            spdlog::drop("Network");
            spdlog::drop("Domain");
            spdlog::drop("Game");
            spdlog::drop("Main");

            // Create TUI sink (replaces console sink)
            s_tuiSink = std::make_shared<infrastructure::tui::TUISink_mt>(logBuffer);
            s_tuiSink->set_level(spdlog::level::trace);

            // Create file sink for persistence
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/server.log", 1024 * 1024 * 10, 5);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");

            // Use TUI sink + file sink (no console sink)
            std::vector<spdlog::sink_ptr> sinks{s_tuiSink, file_sink};

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

            s_tuiMode = true;
            s_mainLogger->info("Server logging system initialized (TUI mode)");

        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }

    void Logger::shutdown() {
        if (s_mainLogger) {
            s_mainLogger->info("Shutting down logging system");
        }
        s_tuiSink.reset();
        s_tuiMode = false;
        spdlog::shutdown();
    }

    bool Logger::isTUIMode() {
        return s_tuiMode;
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

    void Logger::setLevel(spdlog::level::level_enum level) {
        if (s_networkLogger) s_networkLogger->set_level(level);
        if (s_domainLogger) s_domainLogger->set_level(level);
        if (s_gameLogger) s_gameLogger->set_level(level);
        if (s_mainLogger) s_mainLogger->set_level(level);
        spdlog::set_level(level);
    }

    void Logger::setEnabled(bool enabled) {
        if (enabled && !s_enabled) {
            // Re-enable logs
            setLevel(s_previousLevel);
            s_enabled = true;
        } else if (!enabled && s_enabled) {
            // Disable logs - save current level and set to off
            s_previousLevel = s_networkLogger ? s_networkLogger->level() : spdlog::level::debug;
            setLevel(spdlog::level::off);
            s_enabled = false;
        }
    }

    bool Logger::isEnabled() {
        return s_enabled;
    }

}
