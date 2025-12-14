/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** UDPClient
*/

#ifndef UDPCLIENT_HPP_
#define UDPCLIENT_HPP_

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <optional>

#include "Protocol.hpp"

namespace client::network
{
    using boost::asio::ip::udp;

    struct NetworkPlayer {
        uint8_t id;
        uint16_t x;
        uint16_t y;
        bool alive;
    };

    class UDPClient
    {
    public:
        using OnConnectedCallback = std::function<void()>;
        using OnDisconnectedCallback = std::function<void()>;
        using OnReceiveCallback = std::function<void(const std::string &)>;
        using OnErrorCallback = std::function<void(const std::string &)>;
        using OnSnapshotCallback = std::function<void(const std::vector<NetworkPlayer>&)>;

        UDPClient();
        ~UDPClient();

        void connect(const std::string &host, std::uint16_t port);
        void disconnect();
        bool isConnected() const;

        void send(const std::string &message);

        void setOnConnected(const OnConnectedCallback& callback);
        void setOnDisconnected(const OnDisconnectedCallback& callback);
        void setOnReceive(const OnReceiveCallback& callback);
        void setOnError(const OnErrorCallback& callback);
        void setOnSnapshot(const OnSnapshotCallback& callback);

        void movePlayer(uint16_t x, uint16_t y);

        std::optional<uint8_t> getLocalPlayerId() const;
        std::vector<NetworkPlayer> getPlayers() const;

    private:
        void asyncReceiveFrom();
        void asyncSendTo(std::shared_ptr<std::vector<uint8_t>>& buf, size_t totalSize);

        void handleConnect(const boost::system::error_code &error);
        void handleRead(const boost::system::error_code &error, std::size_t bytes);
        void handleWrite(const boost::system::error_code &error);

        void handlePlayerJoin(const uint8_t* payload, size_t size);
        void handlePlayerLeave(const uint8_t* payload, size_t size);
        void handleSnapshot(const uint8_t* payload, size_t size);

        boost::asio::io_context _ioContext;
        udp::socket _socket;
        std::jthread _ioThread;
        udp::endpoint _endpoint;

        bool _connected;
        mutable std::mutex _mutex;

        std::array<char, 1> _sendBuf;
        bool _isWriting;

        char _readBuffer[BUFFER_SIZE];
        std::vector<uint8_t> _accumulator;

        std::optional<uint8_t> _localPlayerId;
        std::vector<NetworkPlayer> _players;
        mutable std::mutex _playersMutex;

        OnConnectedCallback _onConnected;
        OnDisconnectedCallback _onDisconnected;
        OnReceiveCallback _onReceive;
        OnErrorCallback _onError;
        OnSnapshotCallback _onSnapshot;
    };

}

#endif /* !UDPCLIENT_HPP_ */
