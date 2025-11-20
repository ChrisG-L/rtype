/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#include "boot/Boot.hpp"
#include "core/Logger.hpp"

Boot::Boot():
    tcpClient(std::make_unique<client::network::TCPClient>()),
    engine(std::make_unique<core::Engine>())
{
}

void Boot::core()
{
    // Initialize logging system
    client::logging::Logger::init();
    auto logger = client::logging::Logger::getBootLogger();

    logger->info("R-Type client starting...");

    // Configuration des callbacks réseau
    tcpClient->setOnConnected([logger]() {
        logger->info("Connected to server!");
    });
    tcpClient->setOnDisconnected([logger]() {
        logger->info("Disconnected from server");
    });
    tcpClient->connect("127.0.0.1", 4123); // TODO: A voir si on laisse hardcoder l'ip et le port

    // Le moteur démarre sans attendre la connexion
    engine->initialize(tcpClient);
    engine->run();

    logger->info("R-Type client shutting down...");
    client::logging::Logger::shutdown();
}
