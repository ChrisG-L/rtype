/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TCPServer
*/

#include "infrastructure/adapters/in/network/TCPServer.hpp"
#include "infrastructure/logging/Logger.hpp"

namespace infrastructure::adapters::in::network {
    // Session implementation
    Session::Session(tcp::socket socket, std::shared_ptr<MongoDBUserRepository> userRepository)
    : _socket(std::move(socket)), _userRepository(userRepository) {}

    void Session::start() {
        do_read();
    }

    void Session::do_read() {
        auto self = shared_from_this();
        auto logger = server::logging::Logger::getNetworkLogger();

        // changer _data en fonction de la réponse
        _socket.async_read_some(
            boost::asio::buffer(_data, max_length),
            [this, self, logger](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    logger->debug("Received: {}", std::string(_data, length));
                    // do_write(length);
                    handle_command(length);
                    do_read();
                }
            }
        );
    }

    void Session::do_write(std::size_t length) {
        auto self = shared_from_this();
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(_data, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            });
    }

    void Session::handle_command(std::size_t length) {
        using infrastructure::adapters::in::network::execute::Execute;

        infrastructure::adapters::in::network::protocol::CommandParser cmdParser;
        Command cmd = cmdParser.parse(std::string(_data, length));
        std::vector<std::string> args = {"LOGIN", "killian", "PLUENET"};
        Execute execute(cmd, _userRepository);
    }

    // TCPServer implementation
    TCPServer::TCPServer(boost::asio::io_context& io_ctx, std::shared_ptr<MongoDBUserRepository> userRepository)
        : _io_ctx(io_ctx), _userRepository(userRepository) , _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4123)) {
        auto logger = server::logging::Logger::getNetworkLogger();
        logger->info("TCP Server started on port 4123");
    }

    void TCPServer::start() {
        start_accept();
    }

    void TCPServer::run() {
        _io_ctx.run();
    }

    void TCPServer::start_accept() {
        auto logger = server::logging::Logger::getNetworkLogger();

        _acceptor.async_accept(
            [this, logger](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    logger->info("New connection accepted!");
                    std::make_shared<Session>(std::move(socket), _userRepository)->start();
                }
                start_accept();
            }
        );
    }
}
