/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TCPServer
*/

#ifndef TCPSERVER_HPP_
#define TCPSERVER_HPP_

#include <array>
#include <iostream>
#include <boost/asio.hpp>

#include <memory>


namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;

    class Session: public std::enable_shared_from_this<Session> {
        private:
            tcp::socket _socket;
            static constexpr std::size_t max_length = 1024;
            char _data[max_length];

            void do_read();
            void do_write(std::size_t length);
            void handle_command(std::size_t length);

        public:
            Session(tcp::socket socket);
            void start();
    };

    class TCPServer {
        private:
            boost::asio::io_context& _io_ctx;
            tcp::acceptor _acceptor;
            void start_accept();

        public:
            TCPServer(boost::asio::io_context& io_ctx);
            void start();
            void run();
    };
}
#endif /* !TCPSERVER_HPP_ */
