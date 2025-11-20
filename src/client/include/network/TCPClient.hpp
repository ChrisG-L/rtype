/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** TCPClient
*/

#ifndef TCPCLIENT_HPP_
#define TCPCLIENT_HPP_

#include <boost/asio.hpp>
#include <string>
#include <cstdint>
#include <iostream>

namespace client::network
{
    using boost::asio::ip::tcp;
    static constexpr std::size_t BUFFER_SIZE = 4096; // Taille du buffer pour la réception

    class TCPClient
    {
    public:
        TCPClient();
        ~TCPClient();

        void test(); // TODO: Retirer la function une fois les tests terminés

        bool connect(const std::string &host, std::uint16_t port);
        void disconnect();
        bool isConnected() const;
        bool send(const std::string &message);
        bool receive(std::string &message);

    private:
        boost::asio::io_context _ioContext;
        tcp::socket _socket;
        bool _connected = false;
    };

}

#endif /* !TCPCLIENT_HPP_ */
