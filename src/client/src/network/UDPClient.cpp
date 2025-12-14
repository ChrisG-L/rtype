/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** UDPClient
*/

#include "network/UDPClient.hpp"
#include "Protocol.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <vector>

namespace client::network
{
    UDPClient::UDPClient()
        : _socket(_ioContext), _connected(false), _isWriting(false), _localPlayerId(std::nullopt)
    {
        client::logging::Logger::getNetworkLogger()->debug("UDPClient created");
    }

    UDPClient::~UDPClient()
    {
        disconnect();
    }

    void UDPClient::setOnConnected(const OnConnectedCallback& callback)
    {
        _onConnected = callback;
    }

    void UDPClient::setOnDisconnected(const OnDisconnectedCallback& callback)
    {
        _onDisconnected = callback;
    }

    void UDPClient::setOnReceive(const OnReceiveCallback& callback)
    {
        _onReceive = callback;
    }

    void UDPClient::setOnError(const OnErrorCallback& callback)
    {
        _onError = callback;
    }

    void UDPClient::setOnSnapshot(const OnSnapshotCallback& callback)
    {
        _onSnapshot = callback;
    }

    void UDPClient::connect(const std::string &host, std::uint16_t port)
    {
        auto logger = client::logging::Logger::getNetworkLogger();
        if (_connected) {
            logger->warn("Already connected, disconnecting...");
            disconnect();
        }
        logger->info("Connecting to {}:{}...", host, port);

        try {
            udp::resolver resolver(_ioContext);
            auto results = resolver.resolve(host, std::to_string(port));
            _endpoint = *results.begin();

            _socket.open(udp::v4());

            _connected = true;

            asyncReceiveFrom();

            _ioThread = std::jthread([this, logger]() {
                logger->debug("IO thread started");
                _ioContext.run();
                logger->debug("IO thread terminated");
            });

            logger->info("Connected to server UDP at {}:{}",
                        _endpoint.address().to_string(), _endpoint.port());

            UDPHeader head{
                .type = static_cast<uint16_t>(MessageType::HeartBeat),
                .sequence_num = 0,
                .timestamp = UDPHeader::getTimestamp()
            };
            auto buf = std::make_shared<std::vector<uint8_t>>(UDPHeader::WIRE_SIZE);
            head.to_bytes(buf->data());
            asyncSendTo(buf, UDPHeader::WIRE_SIZE);

            if (_onConnected) {
                _onConnected();
            }

        } catch (const std::exception &e) {
            logger->error("Connection error: {}", e.what());
            if (_onError) {
                _onError(std::string("Connexion echouee: ") + e.what());
            }
        }
    }

    void UDPClient::disconnect()
    {
        std::scoped_lock lock(_mutex);
        if (!_connected) {
            return;
        }

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Disconnecting UDP...");

        _connected = false;
        _ioContext.stop();

        boost::system::error_code ec;
        if (_socket.is_open()) {
            _socket.close(ec);
        }

        {
            std::lock_guard<std::mutex> plock(_playersMutex);
            _localPlayerId = std::nullopt;
            _players.clear();
        }

        if (_onDisconnected) {
            _onDisconnected();
        }

        logger->info("UDP disconnected successfully");
    }

    bool UDPClient::isConnected() const
    {
        return _connected && _socket.is_open();
    }

    std::optional<uint8_t> UDPClient::getLocalPlayerId() const
    {
        std::lock_guard<std::mutex> lock(_playersMutex);
        return _localPlayerId;
    }

    std::vector<NetworkPlayer> UDPClient::getPlayers() const
    {
        std::lock_guard<std::mutex> lock(_playersMutex);
        return _players;
    }

    void UDPClient::asyncReceiveFrom()
    {
        if (!isConnected()) {
            return;
        }
        _socket.async_receive_from(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            _endpoint,
            [this](const boost::system::error_code &error, std::size_t bytes) {
                handleRead(error, bytes);
            }
        );
    }

    void UDPClient::asyncSendTo(std::shared_ptr<std::vector<uint8_t>>& buf, size_t totalSize) {
        if (!isConnected()) {
            client::logging::Logger::getNetworkLogger()->warn("Not connected");
            return;
        }
        _socket.async_send_to(
            boost::asio::buffer(buf->data(), totalSize),
            _endpoint,
            [buf](const boost::system::error_code &error, std::size_t) {
                if (error) {
                    client::logging::Logger::getNetworkLogger()->error("UDP write error: {}", error.message());
                }
            }
        );
    }

    void UDPClient::handlePlayerJoin(const uint8_t* payload, size_t size) {
        auto pjOpt = PlayerJoin::from_bytes(payload, size);
        if (!pjOpt) return;

        std::lock_guard<std::mutex> lock(_playersMutex);
        _localPlayerId = pjOpt->player_id;

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Joined game as player {}", static_cast<int>(*_localPlayerId));
    }

    void UDPClient::handlePlayerLeave(const uint8_t* payload, size_t size) {
        auto plOpt = PlayerLeave::from_bytes(payload, size);
        if (!plOpt) return;

        std::lock_guard<std::mutex> lock(_playersMutex);
        _players.erase(
            std::remove_if(_players.begin(), _players.end(),
                [&](const NetworkPlayer& p) { return p.id == plOpt->player_id; }),
            _players.end()
        );

        auto logger = client::logging::Logger::getNetworkLogger();
        logger->info("Player {} left the game", static_cast<int>(plOpt->player_id));
    }

    void UDPClient::handleSnapshot(const uint8_t* payload, size_t size) {
        auto gsOpt = GameSnapshot::from_bytes(payload, size);
        if (!gsOpt) return;

        std::vector<NetworkPlayer> newPlayers;
        newPlayers.reserve(gsOpt->player_count);

        for (uint8_t i = 0; i < gsOpt->player_count; ++i) {
            const auto& ps = gsOpt->players[i];
            newPlayers.push_back(NetworkPlayer{
                .id = ps.id,
                .x = ps.x,
                .y = ps.y,
                .alive = ps.alive != 0
            });
        }

        {
            std::lock_guard<std::mutex> lock(_playersMutex);
            _players = std::move(newPlayers);
        }

        if (_onSnapshot) {
            std::lock_guard<std::mutex> lock(_playersMutex);
            _onSnapshot(_players);
        }
    }

    void UDPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
        auto logger = client::logging::Logger::getNetworkLogger();
        if (!error && bytes > 0) {
            if (bytes >= UDPHeader::WIRE_SIZE) {
                auto headOpt = UDPHeader::from_bytes(_readBuffer, bytes);
                if (!headOpt) {
                    asyncReceiveFrom();
                    return;
                }
                UDPHeader head = *headOpt;
                size_t payload_size = bytes - UDPHeader::WIRE_SIZE;
                const uint8_t* payload = reinterpret_cast<const uint8_t*>(_readBuffer) + UDPHeader::WIRE_SIZE;

                switch (static_cast<MessageType>(head.type)) {
                    case MessageType::PlayerJoin:
                        handlePlayerJoin(payload, payload_size);
                        break;
                    case MessageType::PlayerLeave:
                        handlePlayerLeave(payload, payload_size);
                        break;
                    case MessageType::Snapshot:
                        handleSnapshot(payload, payload_size);
                        break;
                    default:
                        break;
                }
                asyncReceiveFrom();
            }
        } else {
            if (error == boost::asio::error::eof) {
                logger->info("Server disconnected");
            } else {
                logger->error("Read error: {}", error.message());
            }

            if (_onError) {
                _onError("Erreur lecture: " + error.message());
            }

            disconnect();
        }
    }


    void UDPClient::movePlayer(uint16_t x, uint16_t y) {
        MovePlayer movePlayer = {.x = x, .y = y};

        UDPHeader head = {
            .type = static_cast<uint16_t>(MessageType::MovePlayer),
            .sequence_num = 0,
            .timestamp = UDPHeader::getTimestamp()
        };

        const size_t totalSize = UDPHeader::WIRE_SIZE + MovePlayer::WIRE_SIZE;

        auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

        head.to_bytes(buf->data());
        movePlayer.to_bytes(buf->data() + UDPHeader::WIRE_SIZE);
        asyncSendTo(buf, totalSize);
    }

}
