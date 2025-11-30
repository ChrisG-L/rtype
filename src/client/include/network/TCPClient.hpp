/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** TCPClient
*/

#ifndef TCPCLIENT_HPP_
#define TCPCLIENT_HPP_

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
    using boost::asio::ip::tcp;

    class TCPClient
    {
    public:
        // callbacks
        using OnConnectedCallback = std::function<void()>;
        using OnDisconnectedCallback = std::function<void()>;
        using OnReceiveCallback = std::function<void(const std::string &)>;
        using OnErrorCallback = std::function<void(const std::string &)>;

        TCPClient();
        ~TCPClient();

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

        void sendLoginData(const std::string& username, const std::string& password);
        void sendRegisterData(const std::string& username, const std::string& email, const std::string& password);

        // Setters pour credentials en attente (utilisés lors de la réponse serveur)
        void setLoginCredentials(const std::string& username, const std::string& password);
        void setRegisterCredentials(const std::string& username, const std::string& email, const std::string& password);

    private:
        // Méthodes async
        void asyncConnect(tcp::resolver::results_type endpoints);
        void asyncRead();
        void asyncWrite();

        // Handlers
        void handleConnect(const boost::system::error_code &error);
        void handleRead(const boost::system::error_code &error, std::size_t bytes);
        void handleWrite(const boost::system::error_code &error);

        // Contexte et socket
        boost::asio::io_context _ioContext;
        tcp::socket _socket;
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

#endif /* !TCPCLIENT_HPP_ */
