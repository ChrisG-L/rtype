/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UDPServer
*/

#include "infrastructure/adapters/in/network/UDPServer.hpp"

namespace infrastructure::adapters::in::network {
    UDPServer::UDPServer(boost::asio::io_context& io_ctx)
        : _io_ctx(io_ctx), _socket(io_ctx, udp::endpoint(udp::v4(), 4124)) {
        std::cout << "Serveur UDP démarré sur le port 4124\n";
    }

    void UDPServer::start() {
        start_receive();
    }

    void UDPServer::run() {
        _io_ctx.run();
    }

    void UDPServer::stop() {
        _socket.close();
    }

    void UDPServer::start_receive() {
        _socket.async_receive_from(
            boost::asio::buffer(_recv_buffer), _remote_endpoint,
            [this](const boost::system::error_code& error, std::size_t bytes) {
                handle_receive(error, bytes);
            }
        );
    }

    void UDPServer::handle_receive(const boost::system::error_code& error,
        std::size_t bytes_transferred) {
        if (!error && bytes_transferred > 0) {
            std::string received(_recv_buffer.data(), bytes_transferred);
            std::cout << "Reçu de " << _remote_endpoint.port() << ": "
                    << received << std::endl;
        } else if (error) {
            std::cerr << "Erreur réception: " << error.message() << std::endl;
        }

        start_receive();
    }
}
