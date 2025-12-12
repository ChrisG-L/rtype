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
#include <iostream>

namespace infrastructure::adapters::in::network {
    UDPServer::UDPServer(boost::asio::io_context& io_ctx)
        : _io_ctx(io_ctx), _socket(io_ctx, udp::endpoint(udp::v4(), 4124)) {
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
                    std::cerr << "UDP send error: " << ec.message() << std::endl;
                }
            });
    }


    void UDPServer::do_read() {
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _remote_endpoint,
            [this](const boost::system::error_code& error, std::size_t bytes) {
                handle_receive(error, bytes);
            }
        );
    }

    void UDPServer::handle_receive(const boost::system::error_code& error,
        std::size_t bytes) {
        if (!error && bytes > 0) {
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
                        [[maybe_unused]] auto movePlayerOpt = MovePlayer::from_bytes(
                            _readBuffer + UDPHeader::WIRE_SIZE,
                            actual_payload
                        );
                    }
                }
            }
        } else if (error) {
            std::cerr << "Receive error: " << error.message() << std::endl;
        }
        do_write(MessageType::Snapshot, "");
        do_read();
    }
}
