/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#include "boot/Boot.hpp"

Boot::Boot(): io_ctx{},
    tcpClient(std::make_unique<TCPClient>(io_ctx)),
    engine(std::make_unique<core::Engine>())
{
}

void Boot::core()
{
    // tcpClient->run();
    engine->initialize();
    engine->run();
}
