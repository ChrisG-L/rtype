/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** main
*/


#include "infrastructure/bootstrap/GameBootstrap.hpp"
#include "infrastructure/logging/Logger.hpp"
#include "infrastructure/configuration/EnvLoader.hpp"
#include "infrastructure/version/VersionHistoryManager.hpp"

#ifdef _WIN32
    #include <windows.h>
#endif

int main(void) {
    #ifdef _WIN32
        // Configure la console Windows pour utiliser UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif
    using infrastructure::bootstrap::GameBootstrap;
    using infrastructure::configuration::EnvLoader;

    server::logging::Logger::init();
    auto logger = server::logging::Logger::getMainLogger();

    if (EnvLoader::load(".env")) {
        logger->info("Loaded configuration from .env file");
    }

    // Load version history for auto-update system
    infrastructure::version::VersionHistoryManager::getInstance().loadFromFile("version_history.txt");

    logger->info("R-Type server starting...");

    try
    {
        GameBootstrap gameBootstrap;
        gameBootstrap.launch();
    }
    catch(const std::exception& e)
    {
        logger->error("Fatal error: {}", e.what());
        server::logging::Logger::shutdown();
        return 1;
    }

    logger->info("R-Type server shutting down...");
    server::logging::Logger::shutdown();
    return 0;
}
