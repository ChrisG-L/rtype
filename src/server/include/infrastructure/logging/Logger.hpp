/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Logger - Centralized logging configuration for server
*/

#ifndef SERVER_LOGGER_HPP_
#define SERVER_LOGGER_HPP_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

// Forward declarations
namespace infrastructure::tui {
    class LogBuffer;
    template<typename Mutex> class TUISink;
    using TUISink_mt = TUISink<std::mutex>;
}

namespace server::logging {

    class Logger {
    public:
        // Standard init (console + file sinks)
        static void init();

        // TUI init (TUI sink + file sink, no console)
        static void initWithTUI(std::shared_ptr<infrastructure::tui::LogBuffer> logBuffer);

        static void shutdown();

        // Get loggers for different server components
        static std::shared_ptr<spdlog::logger> getNetworkLogger();
        static std::shared_ptr<spdlog::logger> getDomainLogger();
        static std::shared_ptr<spdlog::logger> getGameLogger();
        static std::shared_ptr<spdlog::logger> getMainLogger();

        // Set log level for all loggers
        static void setLevel(spdlog::level::level_enum level);
        static void setEnabled(bool enabled);
        static bool isEnabled();

        // Check if TUI mode is active
        static bool isTUIMode();

    private:
        static spdlog::level::level_enum s_previousLevel;
        static bool s_enabled;
        static bool s_tuiMode;
        static std::shared_ptr<spdlog::logger> s_networkLogger;
        static std::shared_ptr<spdlog::logger> s_domainLogger;
        static std::shared_ptr<spdlog::logger> s_gameLogger;
        static std::shared_ptr<spdlog::logger> s_mainLogger;
        static std::shared_ptr<infrastructure::tui::TUISink_mt> s_tuiSink;
    };

} // namespace server::logging

#endif /* !SERVER_LOGGER_HPP_ */
