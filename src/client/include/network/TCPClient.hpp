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
#include <atomic>
#include <chrono>

#include "Protocol.hpp"
#include "NetworkEvents.hpp"

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
        bool isConnecting() const;
        bool isAuthenticated() const;

        // Get last connection info (for reconnection)
        const std::string& getLastHost() const { return _lastHost; }
        std::uint16_t getLastPort() const { return _lastPort; }

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

        // Event queue for thread-safe event polling
        std::optional<TCPEvent> pollEvent();

    private:
        // Méthodes async
        void asyncConnect(tcp::resolver::results_type endpoints);
        void asyncRead();
        // void asyncWrite();

        // Handlers
        void handleConnect(const boost::system::error_code &error);
        void handleRead(const boost::system::error_code &error, std::size_t bytes);
        // void handleWrite(const boost::system::error_code &error);

        // HeartBeat
        void scheduleHeartbeat();
        void sendHeartbeat();

        // Contexte et socket
        boost::asio::io_context _ioContext;
        tcp::socket _socket;
        std::jthread _ioThread;

        // HeartBeat timer
        boost::asio::steady_timer _heartbeatTimer;
        std::chrono::steady_clock::time_point _lastServerResponse;
        mutable std::mutex _heartbeatMutex;

        // État
        std::atomic<bool> _connected{false};
        std::atomic<bool> _connecting{false};  // Waiting for HeartBeatAck
        std::atomic<bool> _disconnecting{false};
        std::atomic<bool> _isAuthenticated;
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

        // Last connection info (for reconnection)
        std::string _lastHost;
        std::uint16_t _lastPort = 0;

        // Event queue for thread-safe main thread communication
        EventQueue<TCPEvent> _eventQueue;
    };

}

#endif /* !TCPCLIENT_HPP_ */
