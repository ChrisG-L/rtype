/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** TCPClient
*/

#include "network/TCPClient.hpp"

namespace client::network
{

    TCPClient::TCPClient() : _socket(_ioContext)
    {
    }

    TCPClient::~TCPClient()
    {
        disconnect();
    }

    // TODO: Retirer la function une fois les tests terminés
    void TCPClient::test()
    {
        std::string response;

        if (send("Hello, Server!"))
        {
            std::cout << "[TCPClient] Message sent to server" << std::endl;
        }
        else
        {
            std::cout << "[TCPClient] Failed to send message" << std::endl;
            return;
        }

        if (receive(response)) // Pour l'instant le serv ne répond pas
        {
            std::cout << "[TCPClient] Received: " << response << std::endl;
        }
        else
        {
            std::cout << "[TCPClient] No data received" << std::endl;
        }
    }

    bool TCPClient::connect(const std::string &host, std::uint16_t port)
    {
        if (_connected)
        {
            disconnect();
        }

        try
        {
            tcp::resolver resolver(_ioContext);
            tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            boost::asio::connect(_socket, endpoints);
            _connected = true;

            std::cout << "[TCPClient] Connected to " << host << ":" << port << std::endl;
            return true;
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "[TCPClient] Connection failed: " << e.what() << std::endl;
            return false;
        }
    }

    void TCPClient::disconnect()
    {
        if (_connected)
        {
            boost::system::error_code ec;
            _socket.shutdown(tcp::socket::shutdown_both, ec);
            _socket.close(ec);
            _connected = false;
            _socket = tcp::socket(_ioContext);
            std::cout << "[TCPClient] Disconnected" << std::endl;
        }
    }

    bool TCPClient::isConnected() const
    {
        return _connected && _socket.is_open();
    }

    bool TCPClient::send(const std::string &message)
    {
        if (!_connected)
        {
            return false;
        }

        try
        {
            boost::asio::write(_socket, boost::asio::buffer(message));
            return true;
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "[TCPClient] Send failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool TCPClient::receive(std::string &message)
    {
        if (!_connected)
        {
            return false;
        }

        try
        {
            // Non-bloquant : vérifier si des données sont disponibles
            if (_socket.available() == 0)
            {
                return false;
            }

            char buffer[BUFFER_SIZE];
            std::size_t len = _socket.read_some(boost::asio::buffer(buffer, BUFFER_SIZE - 1));
            message.assign(buffer, len);
            return true;
        }
        catch (const boost::system::system_error &e)
        {
            if (e.code() == boost::asio::error::eof)
            {
                _connected = false;
                std::cerr << "[TCPClient] Server disconnected" << std::endl;
            }
            return false;
        }
    }

}
