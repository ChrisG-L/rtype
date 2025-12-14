/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#ifndef BOOT_HPP_
#define BOOT_HPP_

#include <memory>

#include "../network/UDPClient.hpp"
#include "../core/Engine.hpp"

class Boot {
    public:
        Boot();
        void core();

    private:
        std::shared_ptr<client::network::UDPClient> udpClient;
        std::unique_ptr<core::Engine> engine;
};

#endif /* !BOOT_HPP_ */
