/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#ifndef BOOT_HPP_
#define BOOT_HPP_

#include <memory>
#include <string>
#include <optional>

#include "../network/UDPClient.hpp"
#include "../network/TCPClient.hpp"
#include "../core/Engine.hpp"

struct GraphicsOptions {
    std::optional<std::string> name;  // --graphics=<name> (e.g., "sfml", "sdl2")
    std::optional<std::string> path;  // --graphics-path=<full_path>
};

class Boot {
    public:
        Boot(int argc, char* argv[]);
        void core();

    private:
        void parseArgs(int argc, char* argv[]);
        void printHelp() const;

        std::unique_ptr<core::Engine> engine;  // Must be destroyed first (before network clients)
        std::shared_ptr<client::network::UDPClient> udpClient;
        std::shared_ptr<client::network::TCPClient> tcpClient;
        GraphicsOptions _graphicsOptions;
};

#endif /* !BOOT_HPP_ */
