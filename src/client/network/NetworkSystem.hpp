/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** NetworkSystem - Système ECS pour le réseau
*/

#pragma once

#include "../ecs/System.hpp"
#include "../ecs/Registry.hpp"
#include "../ecs/components/Components.hpp"
#include "UDPClient.hpp"
#include "Protocol.hpp"
#include <deque>
#include <chrono>
#include <mutex>
#include <queue>

namespace rtype::network {

/**
 * @brief Entrée d'input pour la réconciliation serveur
 */
struct InputEntry {
    std::uint32_t tick;
    std::uint8_t inputs;
    float x;  // Position après application
    float y;
};

/**
 * @brief Système réseau pour la communication client/serveur
 *
 * Gère l'envoi des inputs, la réception du game state,
 * et la réconciliation serveur.
 */
class NetworkSystem : public ecs::ISystem {
public:
    NetworkSystem();

    void update(ecs::Registry& registry, float dt) override;

    [[nodiscard]] int priority() const override { return 5; } // Après InputSystem

    /**
     * @brief Connecte au serveur
     */
    bool connect(const std::string& host, std::uint16_t port, const std::string& playerName);

    /**
     * @brief Déconnecte du serveur
     */
    void disconnect();

    /**
     * @brief Vérifie si connecté
     */
    [[nodiscard]] bool isConnected() const { return m_client.isConnected(); }

    /**
     * @brief Retourne l'ID du joueur local
     */
    [[nodiscard]] std::uint32_t getPlayerId() const { return m_playerId; }

    /**
     * @brief Retourne le tick serveur actuel
     */
    [[nodiscard]] std::uint32_t getServerTick() const { return m_serverTick; }

    /**
     * @brief Retourne le RTT
     */
    [[nodiscard]] float getRTT() const { return m_client.getRTT(); }

private:
    void handlePacket(const std::uint8_t* data, std::size_t size);
    void handleAccept(const AcceptPacket& packet);
    void handleGameState(const std::uint8_t* data, std::size_t size, ecs::Registry& registry);
    void handleSpawn(const SpawnPacket& packet, ecs::Registry& registry);
    void handleDespawn(const DespawnPacket& packet, ecs::Registry& registry);
    void handleEvent(const EventPacket& packet, ecs::Registry& registry);

    void sendInput(ecs::Registry& registry);
    void serverReconciliation(ecs::Registry& registry, std::uint32_t lastProcessedInput);

    ecs::Entity findEntityByNetworkId(ecs::Registry& registry, std::uint32_t serverId);
    ecs::Entity createNetworkEntity(ecs::Registry& registry, const EntityState& state);

private:
    UDPClient m_client;

    // État de connexion
    std::uint32_t m_playerId = 0;
    std::uint32_t m_serverTick = 0;
    float m_tickRate = 60.0f;
    std::uint32_t m_clientTick = 0;

    // Buffer d'inputs pour la réconciliation
    std::deque<InputEntry> m_inputHistory;
    static constexpr std::size_t MAX_INPUT_HISTORY = 128;

    // Timing
    float m_sendTimer = 0.0f;
    float m_sendRate = 1.0f / 60.0f; // 60 Hz

    // Pointeur vers le registry (pour le callback)
    ecs::Registry* m_currentRegistry = nullptr;

    // Mutex pour la synchronisation thread-safe
    mutable std::mutex m_registryMutex;

    // Queue de paquets reçus (thread-safe)
    std::queue<std::vector<std::uint8_t>> m_packetQueue;
    mutable std::mutex m_packetMutex;
};

} // namespace rtype::network
