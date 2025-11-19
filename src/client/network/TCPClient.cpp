/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** TCPClient
*/

#include "network/TCPClient.hpp"

TCPClient::TCPClient(boost::asio::io_context& io_ctx): _io_ctx{io_ctx}, _socket{io_ctx}
{
    tcp::resolver resolver(_io_ctx);
    //TODO ça serait pas mal de faire un système de port dynamique
    tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "4123");
    boost::asio::async_connect(_socket, endpoints, [this](const boost::system::error_code& error,
        const tcp::endpoint& endpoint) {
        if (!error) {
            std::cout << "Connected to " << endpoint << std::endl;
        } else {
            std::cout << "Connection failed: " << error.message() << std::endl;
        }
    });
}

void TCPClient::run()
{
    // _socket.w
    // for (;;) {
    //     // std::array<char, 128> buf;
    //     std::string buf2 = "coucou";
    //     // boost::system::error_code error;

    //     // size_t len = _socket.read_some(boost::asio::buffer(buf), error);
    //     // std::cout << "len: " << len << std::endl;

    //     // if (error = boost::asio::error::eof)
    //     //     break;
    //     // else if (error) {
    //     //     throw boost::system::system_error(error);
    //     // }
    //     // std::cout.write(buf.data(), len) << std::endl;
    //     _socket.write_some(boost::asio::buffer(buf2));
    // }
}
