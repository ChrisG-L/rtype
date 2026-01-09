/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** ILogger - Port for logging
*/

#ifndef ILOGGER_HPP_
#define ILOGGER_HPP_

#include <string>
#include <format>

namespace application::ports::out {
    class ILogger {
        public:
            virtual ~ILogger() = default;
            virtual void debug(const std::string& message) = 0;
            virtual void info(const std::string& message) = 0;
            virtual void warn(const std::string& message) = 0;
            virtual void error(const std::string& message) = 0;

            template<typename... Args>
            void debug(std::format_string<Args...> fmt, Args&&... args) {
                debug(std::format(fmt, std::forward<Args>(args)...));
            }

            template<typename... Args>
            void info(std::format_string<Args...> fmt, Args&&... args) {
                info(std::format(fmt, std::forward<Args>(args)...));
            }

            template<typename... Args>
            void warn(std::format_string<Args...> fmt, Args&&... args) {
                warn(std::format(fmt, std::forward<Args>(args)...));
            }

            template<typename... Args>
            void error(std::format_string<Args...> fmt, Args&&... args) {
                error(std::format(fmt, std::forward<Args>(args)...));
            }
    };
}

#endif /* !ILOGGER_HPP_ */
