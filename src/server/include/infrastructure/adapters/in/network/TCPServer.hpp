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

#include "protocol/CommandParser.hpp"

#include "infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp"
#include "infrastructure/adapters/in/network/execute/Execute.hpp"

namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;
    using infrastructure::adapters::out::persistence::MongoDBUserRepository;

    class Session: public std::enable_shared_from_this<Session> {
        private:
            tcp::socket _socket;
            static constexpr std::size_t max_length = 1024;
            char _data[max_length];

            void do_read();
            void do_write(std::size_t length);
            void handle_command(std::size_t length);

        public:
            Session(tcp::socket socket, std::shared_ptr<MongoDBUserRepository> userRepository);
            std::shared_ptr<MongoDBUserRepository> _userRepository;
            void start();
    };

    class TCPServer {
        private:
            boost::asio::io_context& _io_ctx;
            std::shared_ptr<MongoDBUserRepository> _userRepository;
            tcp::acceptor _acceptor;
            void start_accept();

        public:
            TCPServer(boost::asio::io_context& io_ctx, std::shared_ptr<MongoDBUserRepository> userRepository);
            void start();
            void run();
    };
}
#endif /* !TCPSERVER_HPP_ */
