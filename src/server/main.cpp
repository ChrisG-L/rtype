/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** main
*/


#include "infrastructure/boostrap/GameBootstrap.hpp"
#include "infrastructure/logging/Logger.hpp"

int main(void) {
    using infrastructure::boostrap::GameBootstrap;

    // Initialize logging
    server::logging::Logger::init();
    auto logger = server::logging::Logger::getMainLogger();

    logger->info("R-Type server starting...");

    try
    {
        GameBootstrap gameBoostrap;
        gameBoostrap.launch();
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
