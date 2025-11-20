/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** TCPClient
*/

#include "network/TCPClient.hpp"

namespace client::network
{
    TCPClient::TCPClient()
        : _socket(_ioContext), _connected(false), _isWriting(false)
    {
        std::cout << "[TCPClient] Créé" << std::endl;
    }

    TCPClient::~TCPClient()
    {
        disconnect();
    }

    void TCPClient::setOnConnected(OnConnectedCallback callback)
    {
        _onConnected = callback;
    }

    void TCPClient::setOnDisconnected(OnDisconnectedCallback callback)
    {
        _onDisconnected = callback;
    }

    void TCPClient::setOnReceive(OnReceiveCallback callback)
    {
        _onReceive = callback;
    }

    void TCPClient::setOnError(OnErrorCallback callback)
    {
        _onError = callback;
    }

    void TCPClient::connect(const std::string &host, std::uint16_t port)
    {
        if (_connected) {
            std::cout << "[TCPClient] Déjà connecté" << std::endl;
            disconnect();
        }

        std::cout << "[TCPClient] Connexion à " << host << ":" << port << "..." << std::endl;

        try {
            tcp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            asyncConnect(endpoints);

            _ioThread = std::thread([this]() {
                std::cout << "[TCPClient] Thread IO démarré" << std::endl;
                _ioContext.run();
                std::cout << "[TCPClient] Thread IO terminé" << std::endl;
            });

            std::cout << "[TCPClient] Connexion lancée" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "[TCPClient] Erreur résolution: " << e.what() << std::endl;
            if (_onError) {
                _onError(std::string("Connexion échouée: ") + e.what());
            }
        }
    }

    void TCPClient::disconnect()
    {
        if (!_connected) {
            return;
        }

        std::cout << "[TCPClient] Déconnexion" << std::endl;

        _connected = false;
        _ioContext.stop();

        boost::system::error_code ec;
        _socket.shutdown(tcp::socket::shutdown_both, ec);
        _socket.close(ec);

        if (_ioThread.joinable()) {
            _ioThread.join();
        }

        if (_onDisconnected) {
            _onDisconnected();
        }

        _ioContext.restart();
        _socket = tcp::socket(_ioContext);

        {
            std::lock_guard<std::mutex> lock(_mutex);
            while (!_sendQueue.empty()) {
                _sendQueue.pop();
            }
            _isWriting = false;
        }

        std::cout << "[TCPClient] Déconnecté" << std::endl;
    }

    bool TCPClient::isConnected() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _connected && _socket.is_open();
    }

    void TCPClient::send(const std::string &message)
    {
        if (!isConnected()) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _sendQueue.push(message);
        }

        boost::asio::post(_ioContext, [this]() {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_isWriting) {
                _isWriting = true;
                asyncWrite();
            }
        });
    }

    void TCPClient::asyncConnect(tcp::resolver::results_type endpoints)
    {
        boost::asio::async_connect(
            _socket,
            endpoints,
            [this](const boost::system::error_code &error, const tcp::endpoint &) {
                handleConnect(error);
            }
        );
    }

    void TCPClient::asyncRead()
    {
        _socket.async_read_some(
            boost::asio::buffer(_readBuffer, BUFFER_SIZE),
            [this](const boost::system::error_code &error, std::size_t bytes) {
                handleRead(error, bytes);
            }
        );
    }

    void TCPClient::asyncWrite()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_sendQueue.empty()) {
            _isWriting = false;
            return;
        }

        _isWriting = true;
        const std::string &message = _sendQueue.front();

        boost::asio::async_write(
            _socket,
            boost::asio::buffer(message),
            [this](const boost::system::error_code &error, std::size_t) {
                handleWrite(error);
            }
        );
    }

    void TCPClient::handleConnect(const boost::system::error_code &error)
    {
        if (!error) {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _connected = true;
            }

            std::cout << "[TCPClient] Connecté avec succès" << std::endl;

            if (_onConnected) {
                _onConnected();
            }

            asyncRead();
        } else {
            std::cerr << "[TCPClient] Échec connexion: " << error.message() << std::endl;

            if (_onError) {
                _onError("Connexion échouée: " + error.message());
            }
        }
    }

    void TCPClient::handleRead(const boost::system::error_code &error, std::size_t bytes)
    {
        if (!error) {
            std::string message(_readBuffer, bytes);

            std::cout << "[TCPClient] Reçu " << bytes << " octets" << std::endl;

            if (_onReceive) {
                _onReceive(message);
            }

            asyncRead();
        } else {
            if (error == boost::asio::error::eof) {
                std::cout << "[TCPClient] Serveur déconnecté" << std::endl;
            } else {
                std::cerr << "[TCPClient] Erreur lecture: " << error.message() << std::endl;
            }

            if (_onError) {
                _onError("Erreur lecture: " + error.message());
            }

            disconnect();
        }
    }

    void TCPClient::handleWrite(const boost::system::error_code &error)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_sendQueue.empty()) {
                _sendQueue.pop();
            }
        }

        if (!error) {
            asyncWrite();
        } else {
            std::cerr << "[TCPClient] Erreur envoi: " << error.message() << std::endl;

            if (_onError) {
                _onError("Erreur envoi: " + error.message());
            }

            _isWriting = false;
            disconnect();
        }
    }

}
