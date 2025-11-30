/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** UDPClient
*/

#ifndef UDPCLIENT_HPP_
#define UDPCLIENT_HPP_

#include <boost/asio.hpp>
#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>

#include "Protocol.hpp"

namespace client::network
{
    using boost::asio::ip::udp;

    class UDPClient
    {
    public:
        // callbacks
        using OnConnectedCallback = std::function<void()>;
        using OnDisconnectedCallback = std::function<void()>;
        using OnReceiveCallback = std::function<void(const std::string &)>;
        using OnErrorCallback = std::function<void(const std::string &)>;

        UDPClient();
        ~UDPClient();

        // Connexion
        void connect(const std::string &host, std::uint16_t port);
        void disconnect();
        bool isConnected() const;

        void send(const std::string &message);

        // Configuration des callbacks
        void setOnConnected(const OnConnectedCallback& callback);
        void setOnDisconnected(const OnDisconnectedCallback& callback);
        void setOnReceive(const OnReceiveCallback& callback);
        void setOnError(const OnErrorCallback& callback);

    private:
        // Méthodes async
        void asyncConnect(udp::resolver::results_type endpoints);
        void asyncRead(udp::resolver::endpoint_type endpoints);
        void asyncWrite();

        // Handlers
        void handleConnect(const boost::system::error_code &error);
        void handleRead(const boost::system::error_code &error, std::size_t bytes);
        void handleWrite(const boost::system::error_code &error);

        // Contexte et socket
        boost::asio::io_context _ioContext;
        udp::socket _socket;
        std::jthread _ioThread;

        // État
        bool _connected;
        mutable std::mutex _mutex;

        // Queue d'envoi
        std::queue<std::string> _sendQueue;
        bool _isWriting;

        // Buffer de réception
        char _readBuffer[BUFFER_SIZE];
        std::vector<uint8_t> _accumulator;

        // Callbacks
        OnConnectedCallback _onConnected;
        OnDisconnectedCallback _onDisconnected;
        OnReceiveCallback _onReceive;
        OnErrorCallback _onError;

        // Credentials en attente pour authentification
        std::string _pendingUsername;
        std::string _pendingPassword;
        std::string _pendingEmail;
    };

}

#endif /* !UDPCLIENT_HPP_ */
