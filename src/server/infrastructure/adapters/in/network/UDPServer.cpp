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

    static constexpr int BROADCAST_INTERVAL_MS = 50;

    UDPServer::UDPServer(boost::asio::io_context& io_ctx)
        : _io_ctx(io_ctx),
          _socket(io_ctx, udp::endpoint(udp::v4(), 4124)),
          _broadcastTimer(io_ctx) {
    }

    void UDPServer::start() {
        do_read();
        scheduleBroadcast();
    }

    void UDPServer::run() {
        _io_ctx.run();
    }

    void UDPServer::stop() {
        _broadcastTimer.cancel();
        _socket.close();
    }

    void UDPServer::sendTo(const udp::endpoint& endpoint, const void* data, size_t size) {
        auto buf = std::make_shared<std::vector<uint8_t>>(
            static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + size
        );

        _socket.async_send_to(
            boost::asio::buffer(buf->data(), buf->size()),
            endpoint,
            [buf](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Send error: " << ec.message() << std::endl;
                }
            }
        );
    }

    void UDPServer::sendPlayerJoin(const udp::endpoint& endpoint, uint8_t playerId) {
        const size_t totalSize = UDPHeader::WIRE_SIZE + PlayerJoin::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PlayerJoin),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PlayerJoin pj{.player_id = playerId};
        pj.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        sendTo(endpoint, buf.data(), buf.size());

        std::cout << "Player " << static_cast<int>(playerId)
                  << " joined from " << endpoint.address().to_string()
                  << ":" << endpoint.port() << std::endl;
    }

    void UDPServer::sendPlayerLeave(uint8_t playerId) {
        const size_t totalSize = UDPHeader::WIRE_SIZE + PlayerLeave::WIRE_SIZE;
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::PlayerLeave),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());

        PlayerLeave pl{.player_id = playerId};
        pl.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = _gameWorld.getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }

        std::cout << "Player " << static_cast<int>(playerId) << " left" << std::endl;
    }

    void UDPServer::broadcastSnapshot() {
        if (_gameWorld.getPlayerCount() == 0) {
            return;
        }

        GameSnapshot snapshot = _gameWorld.getSnapshot();
        const size_t totalSize = UDPHeader::WIRE_SIZE + snapshot.wire_size();
        std::vector<uint8_t> buf(totalSize);

        UDPHeader head{
            .type = static_cast<uint16_t>(MessageType::Snapshot),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };
        head.to_bytes(buf.data());
        snapshot.to_bytes(buf.data() + UDPHeader::WIRE_SIZE);

        auto endpoints = _gameWorld.getAllEndpoints();
        for (const auto& ep : endpoints) {
            sendTo(ep, buf.data(), buf.size());
        }
    }

    void UDPServer::scheduleBroadcast() {
        _broadcastTimer.expires_after(std::chrono::milliseconds(BROADCAST_INTERVAL_MS));
        _broadcastTimer.async_wait([this](boost::system::error_code ec) {
            if (!ec) {
                broadcastSnapshot();
                scheduleBroadcast();
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

    void UDPServer::handle_receive(const boost::system::error_code& error, std::size_t bytes) {
        if (error) {
            std::cerr << "Receive error: " << error.message() << std::endl;
            do_read();
            return;
        }

        if (bytes < UDPHeader::WIRE_SIZE) {
            do_read();
            return;
        }

        auto headOpt = UDPHeader::from_bytes(_readBuffer, bytes);
        if (!headOpt) {
            do_read();
            return;
        }

        UDPHeader head = *headOpt;
        size_t payload_size = bytes - UDPHeader::WIRE_SIZE;
        const uint8_t* payload = reinterpret_cast<const uint8_t*>(_readBuffer) + UDPHeader::WIRE_SIZE;

        auto playerIdOpt = _gameWorld.getPlayerIdByEndpoint(_remote_endpoint);
        if (!playerIdOpt) {
            auto newIdOpt = _gameWorld.addPlayer(_remote_endpoint);
            if (newIdOpt) {
                sendPlayerJoin(_remote_endpoint, *newIdOpt);
                playerIdOpt = newIdOpt;
            } else {
                std::cerr << "Server full, rejecting connection" << std::endl;
                do_read();
                return;
            }
        }

        uint8_t playerId = *playerIdOpt;

        if (head.type == static_cast<uint16_t>(MessageType::MovePlayer)) {
            if (payload_size >= MovePlayer::WIRE_SIZE) {
                auto moveOpt = MovePlayer::from_bytes(payload, payload_size);
                if (moveOpt) {
                    _gameWorld.movePlayer(playerId, moveOpt->x, moveOpt->y);
                }
            }
        }

        do_read();
    }
}
