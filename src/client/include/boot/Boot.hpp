/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#ifndef BOOT_HPP_
#define BOOT_HPP_

#include "../network/TCPClient.hpp"
#include "../core/Engine.hpp"
#include <memory>

using boost::asio::ip::tcp;

class Boot {
    public:
        Boot();
        void core();

    private:
        boost::asio::io_context io_ctx;
        std::unique_ptr<TCPClient> tcpClient;
        std::unique_ptr<core::Engine> engine;
};

#endif /* !BOOT_HPP_ */
