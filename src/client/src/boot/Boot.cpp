/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#include "boot/Boot.hpp"
#include "core/Logger.hpp"
#include "config/ServerConfigManager.hpp"
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
        else if (arg.starts_with("--server=")) {
            parseServerArg(arg.substr(9));
        }
    }
}

void Boot::parseServerArg(const std::string& serverArg)
{
    auto& serverConfig = config::ServerConfigManager::getInstance();

    // Format: host:port or just host
    size_t colonPos = serverArg.rfind(':');
    if (colonPos != std::string::npos && colonPos > 0) {
        std::string host = serverArg.substr(0, colonPos);
        std::string portStr = serverArg.substr(colonPos + 1);

        try {
            int port = std::stoi(portStr);
            if (port > 0 && port <= 65535) {
                serverConfig.setHost(host);
                serverConfig.setTcpPort(static_cast<uint16_t>(port));
                // UDP is usually TCP - 1
                serverConfig.setUdpPort(static_cast<uint16_t>(port - 1));
            }
        } catch (...) {
            // Invalid port, just set host
            serverConfig.setHost(serverArg);
        }
    } else {
        // Just host, no port
        serverConfig.setHost(serverArg);
    }
}

void Boot::printHelp() const
{
    std::cout << "R-Type Client\n\n"
              << "Usage: rtype_client [OPTIONS]\n\n"
              << "Options:\n"
              << "  --server=<host[:port]>  Server address (default: 127.0.0.1:4125)\n"
              << "                          Examples: 51.254.137.175, myserver.com:4125\n"
              << "  --graphics=<name>       Graphics backend name (e.g., sdl2, sfml)\n"
              << "                          Builds lib name: librtype_<name>.so (Linux)\n"
              << "                                           rtype_<name>.dll (Windows)\n"
              << "  --graphics-path=<path>  Full path to a custom graphics plugin\n"
              << "  -h, --help              Show this help message\n\n"
              << "Examples:\n"
              << "  rtype_client                               # Uses localhost (default)\n"
              << "  rtype_client --server=51.254.137.175       # Connect to VPS\n"
              << "  rtype_client --server=myserver.com:4125    # Connect with custom port\n"
              << "  rtype_client --graphics=sdl2               # Uses SDL2 backend\n";
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

    // Get server configuration
    auto& serverConfig = config::ServerConfigManager::getInstance();
    std::string host = serverConfig.getHost();
    uint16_t tcpPort = serverConfig.getTcpPort();
    uint16_t udpPort = serverConfig.getUdpPort();

    logger->info("Connecting to server {}:{} (TCP) / {} (UDP)", host, tcpPort, udpPort);

    // Connect to servers at startup
    tcpClient->connect(host, tcpPort);
    udpClient->connect(host, udpPort);

    engine->initialize(udpClient, tcpClient, _graphicsOptions);
    engine->run();

    logger->info("R-Type client shutting down...");
}
