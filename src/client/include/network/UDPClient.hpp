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

#include "Protocol.hpp"
#include "TCPClient.hpp"

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
        void connect(std::shared_ptr<client::network::TCPClient> tcpClient, const std::string &host, std::uint16_t port);
        void disconnect();
        bool isConnected() const;
        bool isAuthenticated() const;

        void send(const std::string &message);

        // Configuration des callbacks
        void setOnConnected(const OnConnectedCallback& callback);
        void setOnDisconnected(const OnDisconnectedCallback& callback);
        void setOnReceive(const OnReceiveCallback& callback);
        void setOnError(const OnErrorCallback& callback);

        
        void movePlayer(uint16_t x, uint16_t y);
        
        private:
        // Méthodes async
        void asyncReceiveFrom();
        void asyncSendTo(std::shared_ptr<std::vector<uint8_t>>& buf, size_t totalSize);

        // Handlers
        void handleConnect(const boost::system::error_code &error);
        void handleRead(const boost::system::error_code &error, std::size_t bytes);
        void handleWrite(const boost::system::error_code &error);

        // Contexte et socket
        boost::asio::io_context _ioContext;
        udp::socket _socket;
        std::jthread _ioThread;
        udp::endpoint _endpoint; 

        // État
        bool _connected;
        mutable std::mutex _mutex;

        // Queue d'envoi
        std::array<char, 1> _sendBuf;
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

        std::shared_ptr<client::network::TCPClient> _tcpClient;
    };

}

#endif /* !UDPCLIENT_HPP_ */
