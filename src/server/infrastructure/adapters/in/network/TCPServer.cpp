/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** TCPServer
*/

#include "infrastructure/adapters/in/network/TCPServer.hpp"
#include "Protocol.hpp"
#include "infrastructure/adapters/in/network/protocol/Command.hpp"
#include "infrastructure/logging/Logger.hpp"
#include <cstring>
#include <vector>

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

        _socket.async_read_some(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            [this, self, logger](boost::system::error_code ec, std::size_t bytes) {
                if (!ec) {
                    _accumulator.insert(_accumulator.end(), _readBuffer, _readBuffer + bytes);
                    if (_accumulator.size() >= Header::WIRE_SIZE) {
                        Header head = Header::from_bytes(_accumulator.data());
                        handle_command(head);
                    }
                    logger->debug("Received: {} bytes", bytes);
                    do_read();
                }
            }
        );
    }

    void Session::do_write(const std::string& message, const MessageType& msgType) {
        struct Header head = {
            .type = static_cast<uint16_t>(msgType),
            .payload_size = static_cast<uint32_t>(message.length())
        };
        const size_t totalSize = Header::WIRE_SIZE + message.length();
        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        memcpy(buf->data() + Header::WIRE_SIZE, message.c_str(), message.length());

        auto self = shared_from_this();
        boost::asio::async_write(
            _socket,
            boost::asio::buffer(buf->data(), totalSize),
            [this, self, buf](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // do_read();
                }
            });
    }

    void Session::handle_command(const Header& head) {
        using infrastructure::adapters::in::network::execute::Execute;

        Command cmd = {.type = head.type, .buf = std::vector<uint8_t>(_accumulator.begin() + Header::WIRE_SIZE, _accumulator.end())};
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
