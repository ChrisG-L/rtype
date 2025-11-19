/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** NetworkSystem - Implémentation du système réseau
*/

#include "network/NetworkSystem.hpp"
#include <iostream>
#include <cstring>

namespace rtype::network {

NetworkSystem::NetworkSystem() {
    // Configure le callback de réception (appelé depuis thread réseau)
    m_client.setReceiveCallback([this](const std::uint8_t* data, std::size_t size) {
        // Copie les données dans la queue (thread-safe)
        std::vector<std::uint8_t> packet(data, data + size);
        {
            std::lock_guard<std::mutex> lock(m_packetMutex);
            m_packetQueue.push(std::move(packet));
        }
    });
}

void NetworkSystem::update(ecs::Registry& registry, float dt) {
    if (!isConnected()) return;

    m_currentRegistry = &registry;

    // Traite les paquets reçus (depuis le thread principal)
    std::queue<std::vector<std::uint8_t>> packetsToProcess;
    {
        std::lock_guard<std::mutex> lock(m_packetMutex);
        std::swap(packetsToProcess, m_packetQueue);
    }

    while (!packetsToProcess.empty()) {
        auto& packet = packetsToProcess.front();
        handlePacket(packet.data(), packet.size());
        packetsToProcess.pop();
    }

    // Envoie les inputs périodiquement
    m_sendTimer += dt;
    if (m_sendTimer >= m_sendRate) {
        sendInput(registry);
        m_sendTimer = 0.0f;
    }

    m_clientTick++;
}

bool NetworkSystem::connect(const std::string& host, std::uint16_t port, const std::string& playerName) {
    if (!m_client.connect(host, port)) {
        return false;
    }

    // Envoie le paquet de connexion
    ConnectPacket packet;
    std::strncpy(packet.playerName, playerName.c_str(), sizeof(packet.playerName) - 1);
    packet.playerName[sizeof(packet.playerName) - 1] = '\0';  // Garantit null-termination
    packet.header.sequence = m_client.nextSequence();

    m_client.send(packet);
    return true;
}

void NetworkSystem::disconnect() {
    if (isConnected()) {
        // Envoie le paquet de déconnexion
        PacketHeader header(PacketType::ClientDisconnect, m_client.nextSequence());
        m_client.send(header);
    }

    m_client.disconnect();
    m_playerId = 0;
    m_inputHistory.clear();
}

void NetworkSystem::handlePacket(const std::uint8_t* data, std::size_t size) {
    if (size < sizeof(PacketHeader)) return;

    PacketType type = PacketSerializer::getPacketType(data, size);

    // Valide que le type de paquet est un type serveur valide
    std::uint8_t typeValue = static_cast<std::uint8_t>(type);
    if (typeValue < 0x10 || typeValue > 0x16) {
        // Type invalide ou type client (ne devrait pas être reçu)
        return;
    }

    switch (type) {
        case PacketType::ServerAccept: {
            AcceptPacket packet;
            if (PacketSerializer::deserialize(data, size, packet)) {
                handleAccept(packet);
            }
            break;
        }
        case PacketType::ServerGameState: {
            if (m_currentRegistry) {
                handleGameState(data, size, *m_currentRegistry);
            }
            break;
        }
        case PacketType::ServerSpawn: {
            SpawnPacket packet;
            if (PacketSerializer::deserialize(data, size, packet) && m_currentRegistry) {
                handleSpawn(packet, *m_currentRegistry);
            }
            break;
        }
        case PacketType::ServerDespawn: {
            DespawnPacket packet;
            if (PacketSerializer::deserialize(data, size, packet) && m_currentRegistry) {
                handleDespawn(packet, *m_currentRegistry);
            }
            break;
        }
        case PacketType::ServerEvent: {
            EventPacket packet;
            if (PacketSerializer::deserialize(data, size, packet) && m_currentRegistry) {
                handleEvent(packet, *m_currentRegistry);
            }
            break;
        }
        default:
            break;
    }

    // Met à jour les ACKs
    PacketHeader header;
    if (PacketSerializer::deserialize(data, size, header)) {
        m_client.updateAcks(header.sequence);
    }
}

void NetworkSystem::handleAccept(const AcceptPacket& packet) {
    m_playerId = packet.playerId;
    m_serverTick = packet.serverTick;
    m_tickRate = packet.tickRate;

    std::cout << "Connecté au serveur! ID: " << m_playerId
              << ", Tick: " << m_serverTick << std::endl;
}

void NetworkSystem::handleGameState(const std::uint8_t* data, std::size_t size, ecs::Registry& registry) {
    GameStatePacket packet;
    std::vector<EntityState> entities;

    if (!PacketSerializer::deserializeGameState(data, size, packet, entities)) {
        return;
    }

    m_serverTick = packet.serverTick;

    // Met à jour les entités
    for (const auto& state : entities) {
        ecs::Entity entity = findEntityByNetworkId(registry, state.entityId);

        if (entity == ecs::NULL_ENTITY) {
            // Nouvelle entité
            entity = createNetworkEntity(registry, state);
        }

        // Met à jour l'entité
        auto* netId = registry.tryGetComponent<ecs::components::NetworkId>(entity);
        if (!netId) continue;

        // Ne met pas à jour le joueur local directement (réconciliation)
        if (netId->isLocalPlayer) {
            serverReconciliation(registry, packet.lastProcessedInput);
            continue;
        }

        // Met à jour les autres entités (pour interpolation)
        auto* interp = registry.tryGetComponent<ecs::components::Interpolation>(entity);
        if (interp) {
            ecs::components::NetworkState netState;
            netState.x = state.x;
            netState.y = state.y;
            netState.vx = state.vx;
            netState.vy = state.vy;
            netState.tick = packet.serverTick;
            // Timestamp basé sur le tick serveur (en secondes)
            netState.timestamp = static_cast<float>(packet.serverTick) / m_tickRate;
            interp->addState(netState);
        } else {
            // Mise à jour directe si pas d'interpolation
            auto* transform = registry.tryGetComponent<ecs::components::Transform>(entity);
            if (transform) {
                transform->x = state.x;
                transform->y = state.y;
            }
        }

        // Met à jour la santé
        auto* health = registry.tryGetComponent<ecs::components::Health>(entity);
        if (health) {
            health->current = state.health;
        }
    }
}

void NetworkSystem::handleSpawn(const SpawnPacket& packet, ecs::Registry& registry) {
    // Vérifie si l'entité existe déjà
    ecs::Entity existing = findEntityByNetworkId(registry, packet.entityId);
    if (existing != ecs::NULL_ENTITY) return;

    // Crée l'entité
    ecs::Entity entity = registry.createEntity();

    // Ajoute les composants de base
    registry.addComponent<ecs::components::Transform>(entity, packet.x, packet.y);
    registry.addComponent<ecs::components::Velocity>(entity);

    // NetworkId
    auto entityType = static_cast<ecs::components::NetworkEntityType>(packet.type);
    bool isLocal = (packet.entityId == m_playerId && packet.type == EntityType::Player);
    registry.addComponent<ecs::components::NetworkId>(entity, packet.entityId, entityType, isLocal);

    // Santé
    registry.addComponent<ecs::components::Health>(entity, packet.health);

    // Interpolation pour les entités non-locales
    if (!isLocal) {
        registry.addComponent<ecs::components::Interpolation>(entity);
    }

    // PlayerInput pour le joueur local
    if (isLocal) {
        registry.addComponent<ecs::components::PlayerInput>(entity);
    }

    std::cout << "Spawn entité: " << packet.entityId
              << " type: " << static_cast<int>(packet.type) << std::endl;
}

void NetworkSystem::handleDespawn(const DespawnPacket& packet, ecs::Registry& registry) {
    ecs::Entity entity = findEntityByNetworkId(registry, packet.entityId);
    if (entity != ecs::NULL_ENTITY) {
        registry.scheduleDestroy(entity);
        std::cout << "Despawn entité: " << packet.entityId << std::endl;
    }
}

void NetworkSystem::handleEvent(const EventPacket& packet, ecs::Registry& registry) {
    // Traite les événements de jeu (à personnaliser)
    switch (packet.event) {
        case GameEvent::PlayerHit: {
            ecs::Entity entity = findEntityByNetworkId(registry, packet.entityId);
            if (entity != ecs::NULL_ENTITY) {
                auto* health = registry.tryGetComponent<ecs::components::Health>(entity);
                if (health) {
                    health->damage(packet.value);
                }
            }
            break;
        }
        default:
            break;
    }
}

void NetworkSystem::sendInput(ecs::Registry& registry) {
    // Trouve le joueur local
    ecs::Entity localPlayer = ecs::NULL_ENTITY;
    ecs::components::PlayerInput* input = nullptr;
    ecs::components::Transform* transform = nullptr;

    registry.view<ecs::components::PlayerInput, ecs::components::NetworkId, ecs::components::Transform>(
        [&](ecs::Entity entity, ecs::components::PlayerInput& pi,
            ecs::components::NetworkId& netId, ecs::components::Transform& t) {
            if (netId.isLocalPlayer) {
                localPlayer = entity;
                input = &pi;
                transform = &t;
            }
        });

    if (localPlayer == ecs::NULL_ENTITY || !input || !transform) return;

    // Crée le paquet d'input
    InputPacket packet;
    packet.header.sequence = m_client.nextSequence();
    packet.header.ack = m_client.getLastAck();
    packet.header.ackBitfield = m_client.getAckBitfield();
    packet.clientTick = m_clientTick;
    packet.inputs = input->toBitfield();

    // Envoie
    m_client.send(packet);

    // Sauvegarde dans l'historique pour la réconciliation
    InputEntry entry;
    entry.tick = m_clientTick;
    entry.inputs = packet.inputs;
    entry.x = transform->x;
    entry.y = transform->y;

    m_inputHistory.push_back(entry);

    // Limite la taille de l'historique
    while (m_inputHistory.size() > MAX_INPUT_HISTORY) {
        m_inputHistory.pop_front();
    }
}

void NetworkSystem::serverReconciliation(ecs::Registry& /*registry*/, std::uint32_t lastProcessedInput) {
    // Supprime les inputs déjà traités par le serveur
    while (!m_inputHistory.empty() && m_inputHistory.front().tick <= lastProcessedInput) {
        m_inputHistory.pop_front();
    }

    // TODO: Si la position serveur diffère trop de la position prédite,
    // réapplique les inputs non confirmés à partir de la position serveur.
    // Pour l'instant, on fait confiance au client-side prediction.
}

ecs::Entity NetworkSystem::findEntityByNetworkId(ecs::Registry& registry, std::uint32_t serverId) {
    ecs::Entity found = ecs::NULL_ENTITY;

    registry.view<ecs::components::NetworkId>(
        [&](ecs::Entity entity, ecs::components::NetworkId& netId) {
            if (netId.serverId == serverId) {
                found = entity;
            }
        });

    return found;
}

ecs::Entity NetworkSystem::createNetworkEntity(ecs::Registry& registry, const EntityState& state) {
    ecs::Entity entity = registry.createEntity();

    // Transform
    registry.addComponent<ecs::components::Transform>(entity, state.x, state.y);

    // Velocity
    registry.addComponent<ecs::components::Velocity>(entity, state.vx, state.vy);

    // NetworkId
    auto type = static_cast<ecs::components::NetworkEntityType>(state.type);
    bool isLocal = (state.entityId == m_playerId &&
                    state.type == EntityType::Player);
    registry.addComponent<ecs::components::NetworkId>(entity, state.entityId, type, isLocal);

    // Health
    registry.addComponent<ecs::components::Health>(entity, state.health);

    // Interpolation pour les autres joueurs/entités
    if (!isLocal) {
        registry.addComponent<ecs::components::Interpolation>(entity);
    }

    // PlayerInput pour le joueur local
    if (isLocal) {
        registry.addComponent<ecs::components::PlayerInput>(entity);
    }

    return entity;
}

} // namespace rtype::network
