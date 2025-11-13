/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TCPServer
*/

#include "infrastructure/adapters/in/network/TCPServer.hpp"

namespace infrastructure::adapters::in::network {
    // Session implementation
    Session::Session(tcp::socket socket) : _socket(std::move(socket)) {}

    void Session::start() {
        do_read();
    }

    void Session::do_read() {
        auto self = shared_from_this();
        // changer _data en fonction de la réponse
        _socket.async_read_some(
            boost::asio::buffer(_data, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout << "Reçu: " << std::string(_data, length) << std::endl;
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
        infrastructure::adapters::in::network::protocol::CommandParser cmdParser;
        Command cmd = cmdParser.parse(std::string(_data, length));
    }

    // TCPServer implementation
    TCPServer::TCPServer(boost::asio::io_context& io_ctx)
        : _io_ctx(io_ctx), _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4123)) {
        std::cout << "Serveur TCP démarré sur le port 4123\n";
    }

    void TCPServer::start() {
        start_accept();
    }

    void TCPServer::run() {
        _io_ctx.run();
    }

    void TCPServer::start_accept() {
        _acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "Nouvelle connexion acceptée!" << std::endl;
                    std::make_shared<Session>(std::move(socket))->start();
                }
                start_accept();
            }
        );
    }
}
