/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Logger - Centralized logging configuration for client
*/

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace client::logging {

    class Logger {
    public:
        static void init();
        static void shutdown();

        static std::shared_ptr<spdlog::logger> getNetworkLogger();
        static std::shared_ptr<spdlog::logger> getEngineLogger();
        static std::shared_ptr<spdlog::logger> getGraphicsLogger();
        static std::shared_ptr<spdlog::logger> getSceneLogger();
        static std::shared_ptr<spdlog::logger> getUILogger();
        static std::shared_ptr<spdlog::logger> getBootLogger();
        static std::shared_ptr<spdlog::logger> getAudioLogger();

    private:
        static std::shared_ptr<spdlog::logger> s_networkLogger;
        static std::shared_ptr<spdlog::logger> s_engineLogger;
        static std::shared_ptr<spdlog::logger> s_graphicsLogger;
        static std::shared_ptr<spdlog::logger> s_sceneLogger;
        static std::shared_ptr<spdlog::logger> s_uiLogger;
        static std::shared_ptr<spdlog::logger> s_bootLogger;
        static std::shared_ptr<spdlog::logger> s_audioLogger;
    };

} // namespace client::logging

#endif /* !LOGGER_HPP_ */
