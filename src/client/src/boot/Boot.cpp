/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#include "boot/Boot.hpp"
#include "core/Logger.hpp"
#include <cstring>
#include <iostream>

Boot::Boot(int argc, char* argv[]):
    engine(std::make_unique<core::Engine>()),
    udpClient(std::make_shared<client::network::UDPClient>()),
    tcpClient(std::make_shared<client::network::TCPClient>())
{
    parseArgs(argc, argv);
}

void Boot::parseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printHelp();
            client::logging::Logger::shutdown();
            std::exit(0);
        }
        else if (arg.starts_with("--graphics=")) {
            _graphicsOptions.name = arg.substr(11);
        }
        else if (arg.starts_with("--graphics-path=")) {
            _graphicsOptions.path = arg.substr(16);
        }
    }
}

void Boot::printHelp() const
{
    std::cout << "R-Type Client\n\n"
              << "Usage: rtype_client [OPTIONS]\n\n"
              << "Options:\n"
              << "  --graphics=<name>       Graphics backend name (e.g., sdl2, sfml)\n"
              << "                          Builds lib name: librtype_<name>.so (Linux)\n"
              << "                                           rtype_<name>.dll (Windows)\n"
              << "  --graphics-path=<path>  Full path to a custom graphics plugin\n"
              << "  -h, --help              Show this help message\n\n"
              << "Examples:\n"
              << "  rtype_client                          # Uses SFML (default)\n"
              << "  rtype_client --graphics=sdl2          # Uses SDL2 backend\n"
              << "  rtype_client --graphics-path=./my_plugin.so  # Custom plugin\n";
}

void Boot::core()
{
    auto logger = client::logging::Logger::getBootLogger();

    logger->info("R-Type client starting...");

    // UDP callbacks for game networking
    udpClient->setOnConnected([logger]() {
        logger->info("Connected to server UDP!");
    });
    udpClient->setOnDisconnected([logger]() {
        logger->info("Disconnected from server UDP");
    });

    // TCP callbacks for authentication
    tcpClient->setOnConnected([logger]() {
        logger->info("Connected to auth server TCP!");
    });
    tcpClient->setOnDisconnected([logger]() {
        logger->info("Disconnected from auth server TCP");
    });
    tcpClient->setOnError([logger](const std::string& error) {
        logger->error("TCP error: {}", error);
    });

    // Connect to servers at startup
    tcpClient->connect("127.0.0.1", 4125);
    udpClient->connect("127.0.0.1", 4124);

    engine->initialize(udpClient, tcpClient, _graphicsOptions);
    engine->run();

    logger->info("R-Type client shutting down...");
}
