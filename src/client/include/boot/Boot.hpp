/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Boot
*/

#ifndef BOOT_HPP_
#define BOOT_HPP_

#include "network/TCPClient.hpp"
#include <memory>

using boost::asio::ip::tcp;

class Boot {
    public:
        Boot(): io_ctx{}, tcpClient(std::make_unique<TCPClient>(io_ctx)) {};
        ~Boot() = default;

        

    protected:
    private:
        boost::asio::io_context io_ctx;
        std::unique_ptr<TCPClient> tcpClient;
};

#endif /* !BOOT_HPP_ */
