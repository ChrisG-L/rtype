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
    udpClient(std::make_unique<client::network::UDPClient>()),
    engine(std::make_unique<core::Engine>())
{
}

void Boot::core()
{
    auto logger = client::logging::Logger::getBootLogger();

    logger->info("R-Type client starting...");

    tcpClient->setOnConnected([logger]() {
        logger->info("Connected to server TCP!");
    });
    tcpClient->setOnDisconnected([logger]() {
        logger->info("Disconnected from server TCP!");
    });
    tcpClient->connect("127.0.0.1", 4123);

    udpClient->setOnConnected([logger]() {
        logger->info("Connected to server UDP!");
    });
    udpClient->setOnDisconnected([logger]() {
        logger->info("Disconnected from server UDP");
    });

    udpClient->connect(tcpClient, "127.0.0.1", 4124);

    engine->initialize(tcpClient, udpClient);
    engine->run();

    logger->info("R-Type client shutting down...");
}
