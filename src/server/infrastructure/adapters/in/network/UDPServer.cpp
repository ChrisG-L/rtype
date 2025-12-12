/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UDPServer
*/

#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "Protocol.hpp"
#include <chrono>
#include <cstdint>

namespace infrastructure::adapters::in::network {
    UDPServer::UDPServer(boost::asio::io_context& io_ctx)
        : _io_ctx(io_ctx), _socket(io_ctx, udp::endpoint(udp::v4(), 4124)) {
        std::cout << "Serveur UDP démarré sur le port 4124\n";
    }

    void UDPServer::start() {
        do_read();
    }

    void UDPServer::run() {
        _io_ctx.run();
    }

    void UDPServer::stop() {
        _socket.close();
    }

    void UDPServer::do_write(const MessageType& msgType, const std::string& message) {

        struct UDPHeader head = {
            .type = static_cast<uint16_t>(msgType),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        const size_t totalSize = UDPHeader::WIRE_SIZE + message.length();
        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        memcpy(buf->data() + Header::WIRE_SIZE, message.c_str(), message.length());

        _socket.async_send_to(
            boost::asio::buffer(buf->data(), totalSize),
            _remote_endpoint,
            [this, buf](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                } else {
                    std::cout << "UDPSERVER ERROR SEND!: " << ec << std::endl;
                }
            });
    }


    void UDPServer::do_read() {
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _remote_endpoint,
            [this](const boost::system::error_code& error, std::size_t bytes) {
                std::cout << "readPort: " << _remote_endpoint.port() << std::endl;
                handle_receive(error, bytes);
            }
        );
    }

    void UDPServer::handle_receive(const boost::system::error_code& error,
        std::size_t bytes) {
        if (!error && bytes > 0) {
            std::cout << _remote_endpoint.data() << std::endl;
            if (bytes >= UDPHeader::WIRE_SIZE) {
                auto headOpt = UDPHeader::from_bytes(_readBuffer, bytes);
                if (!headOpt) {
                    do_read();
                    return;
                }
                UDPHeader head = *headOpt;
                size_t actual_payload = bytes - UDPHeader::WIRE_SIZE;
                if (head.type == static_cast<uint16_t>(MessageType::MovePlayer)) {
                    if (actual_payload >= MovePlayer::WIRE_SIZE) {
                        auto movePlayerOpt = MovePlayer::from_bytes(
                            _readBuffer + UDPHeader::WIRE_SIZE,
                            actual_payload
                        );
                        if (movePlayerOpt) {
                            std::cout << "movePlayer.x: " << movePlayerOpt->x << std::endl;
                            std::cout << "movePlayer.y: " << movePlayerOpt->y << std::endl;
                        }
                    }
                }
            }
        } else if (error) {
            std::cerr << "Erreur réception: " << error.message() << std::endl;
        }
        do_write(MessageType::Snapshop, "");
        do_read();
    }
}
