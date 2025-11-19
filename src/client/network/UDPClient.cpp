/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** UDPClient - Implémentation du client UDP
*/

#include "UDPClient.hpp"
#include <iostream>

namespace rtype::network {

UDPClient::UDPClient()
    : m_socket(m_ioContext)
{
}

UDPClient::~UDPClient() {
    disconnect();
}

bool UDPClient::connect(const std::string& host, std::uint16_t port) {
    try {
        // Résout l'adresse du serveur
        udp::resolver resolver(m_ioContext);
        auto endpoints = resolver.resolve(udp::v4(), host, std::to_string(port));

        if (endpoints.empty()) {
            std::cerr << "Impossible de résoudre l'adresse: " << host << std::endl;
            return false;
        }

        m_serverEndpoint = *endpoints.begin();

        // Ouvre le socket
        m_socket.open(udp::v4());

        // Démarre la réception
        m_running = true;
        startReceive();

        // Lance le thread réseau
        m_networkThread = std::thread(&UDPClient::runIOContext, this);

        m_connected = true;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Erreur de connexion: " << e.what() << std::endl;
        return false;
    }
}

void UDPClient::disconnect() {
    if (!m_running) return;

    m_running = false;
    m_connected = false;

    // Arrête l'io_context
    m_ioContext.stop();

    // Ferme le socket
    if (m_socket.is_open()) {
        boost::system::error_code ec;
        m_socket.close(ec);
    }

    // Attend la fin du thread
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
}

void UDPClient::sendRaw(const std::vector<std::uint8_t>& data) {
    if (!m_connected) return;

    try {
        m_socket.send_to(boost::asio::buffer(data), m_serverEndpoint);
    } catch (const std::exception& e) {
        std::cerr << "Erreur d'envoi: " << e.what() << std::endl;
    }
}

void UDPClient::setReceiveCallback(ReceiveCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_receiveCallback = std::move(callback);
}

void UDPClient::updateAcks(std::uint16_t receivedSequence) {
    // Calcule la différence entre le nouveau et l'ancien ACK
    if (receivedSequence > m_lastAck) {
        std::uint32_t diff = receivedSequence - m_lastAck;

        if (diff <= 32) {
            // Décale le bitfield et ajoute l'ancien ACK
            m_ackBitfield = (m_ackBitfield << diff) | (1 << (diff - 1));
        } else {
            // Trop grand écart, réinitialise le bitfield
            m_ackBitfield = 0;
        }

        m_lastAck = receivedSequence;
    } else if (receivedSequence < m_lastAck) {
        // Paquet ancien mais valide, marque dans le bitfield
        std::uint32_t diff = m_lastAck - receivedSequence;
        if (diff <= 32) {
            m_ackBitfield |= (1 << (diff - 1));
        }
    }
}

void UDPClient::startReceive() {
    m_socket.async_receive_from(
        boost::asio::buffer(m_receiveBuffer),
        m_serverEndpoint,
        [this](const boost::system::error_code& error, std::size_t bytesReceived) {
            handleReceive(error, bytesReceived);
        });
}

void UDPClient::handleReceive(const boost::system::error_code& error, std::size_t bytesReceived) {
    if (!m_running) return;

    if (!error && bytesReceived > 0) {
        // Appelle le callback
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_receiveCallback) {
            m_receiveCallback(m_receiveBuffer.data(), bytesReceived);
        }
    }

    // Continue à recevoir
    if (m_running) {
        startReceive();
    }
}

void UDPClient::runIOContext() {
    while (m_running) {
        try {
            m_ioContext.run();
            break;
        } catch (const std::exception& e) {
            std::cerr << "Erreur IO: " << e.what() << std::endl;
        }
    }
}

} // namespace rtype::network
