/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** UDPClient - Client UDP pour la communication avec le serveur
*/

#pragma once

#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include "Protocol.hpp"

namespace rtype::network {

using boost::asio::ip::udp;

/**
 * @brief Client UDP asynchrone pour la communication avec le serveur de jeu
 */
class UDPClient {
public:
    using ReceiveCallback = std::function<void(const std::uint8_t*, std::size_t)>;

    UDPClient();
    ~UDPClient();

    // Non-copiable
    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;

    /**
     * @brief Connecte au serveur
     * @param host Adresse IP ou nom d'hôte
     * @param port Port UDP
     * @return true si la connexion est établie
     */
    bool connect(const std::string& host, std::uint16_t port);

    /**
     * @brief Déconnecte du serveur
     */
    void disconnect();

    /**
     * @brief Vérifie si connecté
     */
    [[nodiscard]] bool isConnected() const { return m_connected; }

    /**
     * @brief Envoie un paquet au serveur
     */
    template<typename T>
    void send(const T& packet) {
        auto data = PacketSerializer::serialize(packet);
        sendRaw(data);
    }

    /**
     * @brief Envoie des données brutes
     */
    void sendRaw(const std::vector<std::uint8_t>& data);

    /**
     * @brief Définit le callback de réception
     */
    void setReceiveCallback(ReceiveCallback callback);

    /**
     * @brief Récupère le RTT (Round Trip Time) en millisecondes
     */
    [[nodiscard]] float getRTT() const { return m_rtt; }

    /**
     * @brief Récupère le numéro de séquence actuel
     */
    [[nodiscard]] std::uint16_t getSequence() const { return m_sequence; }

    /**
     * @brief Incrémente et retourne le nouveau numéro de séquence
     */
    std::uint16_t nextSequence() { return ++m_sequence; }

    /**
     * @brief Récupère le dernier ACK reçu
     */
    [[nodiscard]] std::uint16_t getLastAck() const { return m_lastAck; }

    /**
     * @brief Récupère le bitfield des ACKs
     */
    [[nodiscard]] std::uint32_t getAckBitfield() const { return m_ackBitfield; }

    /**
     * @brief Met à jour les ACKs après réception d'un paquet
     */
    void updateAcks(std::uint16_t receivedSequence);

private:
    void startReceive();
    void handleReceive(const boost::system::error_code& error, std::size_t bytesReceived);
    void runIOContext();

private:
    boost::asio::io_context m_ioContext;
    udp::socket m_socket;
    udp::endpoint m_serverEndpoint;

    std::thread m_networkThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    // Buffer de réception
    static constexpr std::size_t BUFFER_SIZE = 1024;
    std::array<std::uint8_t, BUFFER_SIZE> m_receiveBuffer;

    // Callback de réception
    ReceiveCallback m_receiveCallback = nullptr;
    std::mutex m_callbackMutex;

    // Séquençage et ACKs
    std::uint16_t m_sequence = 0;
    std::uint16_t m_lastAck = 0;  // Même type que receivedSequence pour éviter les conversions
    std::uint32_t m_ackBitfield = 0;

    // RTT
    float m_rtt = 0.0f;
};

} // namespace rtype::network
