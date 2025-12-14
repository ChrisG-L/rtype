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

namespace server::logging {

    class Logger {
    public:
        static void init();
        static void shutdown();

        // Get loggers for different server components
        static std::shared_ptr<spdlog::logger> getNetworkLogger();
        static std::shared_ptr<spdlog::logger> getDomainLogger();
        static std::shared_ptr<spdlog::logger> getGameLogger();
        static std::shared_ptr<spdlog::logger> getMainLogger();

    private:
        static std::shared_ptr<spdlog::logger> s_networkLogger;
        static std::shared_ptr<spdlog::logger> s_domainLogger;
        static std::shared_ptr<spdlog::logger> s_gameLogger;
        static std::shared_ptr<spdlog::logger> s_mainLogger;
    };

} // namespace server::logging

#endif /* !SERVER_LOGGER_HPP_ */
