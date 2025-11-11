/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UDPServer
*/

#ifndef UDPSERVER_HPP_
#define UDPSERVER_HPP_

#include <array>
#include <iostream>
#include <boost/asio.hpp>


namespace infrastructure::adapters::in::network {
    using boost::asio::ip::udp;

    class UDPServer {
        private:
            udp::socket _socket;
            boost::asio::io_context& _io_ctx;
            udp::endpoint _remote_endpoint;
            std::array<char, 1024> _recv_buffer;

            void start_receive();
            void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);

        public:
            UDPServer(boost::asio::io_context& io_ctx);
            void start(boost::asio::io_context& io_ctx);
            void run();
            void stop();
    };
}
#endif /* !UDPSERVER_HPP_ */
