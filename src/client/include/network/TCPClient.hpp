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

using boost::asio::ip::tcp;

class TCPClient {
    public:
        explicit TCPClient(boost::asio::io_context& io_ctx): _io_ctx{io_ctx}, _socket{io_ctx} {
            tcp::resolver resolver(_io_ctx);
            //TODO ça serait pas mal de faire un système de port dynamique
            tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "4123");
            boost::asio::connect(_socket, endpoints);
        };

    private:
        boost::asio::io_context& _io_ctx;
        tcp::socket _socket;
};

#endif /* !TCPCLIENT_HPP_ */
