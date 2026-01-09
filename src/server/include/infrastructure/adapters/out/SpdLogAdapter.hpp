/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** SpdLogAdapter - spdlog adapter for ILogger port
*/

#ifndef SPDLOGADAPTER_HPP_
#define SPDLOGADAPTER_HPP_

#include "application/ports/out/ILogger.hpp"
#include "infrastructure/logging/Logger.hpp"

namespace infrastructure::adapters::out {
    class SpdLogAdapter : public application::ports::out::ILogger {
        public:
            void debug(const std::string& message) override {
                server::logging::Logger::getMainLogger()->debug(message);
            }

            void info(const std::string& message) override {
                server::logging::Logger::getMainLogger()->info(message);
            }

            void warn(const std::string& message) override {
                server::logging::Logger::getMainLogger()->warn(message);
            }

            void error(const std::string& message) override {
                server::logging::Logger::getMainLogger()->error(message);
            }
    };
}

#endif /* !SPDLOGADAPTER_HPP_ */
