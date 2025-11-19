/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** TCPClient
*/

#ifndef TCPCLIENT_HPP_
#define TCPCLIENT_HPP_

#include <iostream>
#include <boost/asio.hpp>
#include <array>

using boost::asio::ip::tcp;

class TCPClient {
    public:
        TCPClient(boost::asio::io_context& io_ctx);
        void run();

    private:
        boost::asio::io_context& _io_ctx;
        tcp::socket _socket;
};

#endif /* !TCPCLIENT_HPP_ */
