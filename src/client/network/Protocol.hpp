/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Protocol - Définition du protocole binaire UDP
*/

#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

namespace rtype::network {

/**
 * @brief Types de paquets
 */
enum class PacketType : std::uint8_t {
    // Client -> Serveur
    ClientConnect = 0x01,     // Demande de connexion
    ClientDisconnect = 0x02,  // Déconnexion
    ClientInput = 0x03,       // Inputs du joueur
    ClientPing = 0x04,        // Ping pour mesurer la latence

    // Serveur -> Client
    ServerAccept = 0x10,      // Connexion acceptée
    ServerReject = 0x11,      // Connexion refusée
    ServerGameState = 0x12,   // État du jeu (positions, etc.)
    ServerSpawn = 0x13,       // Nouvelle entité
    ServerDespawn = 0x14,     // Entité détruite
    ServerPong = 0x15,        // Réponse au ping
    ServerEvent = 0x16        // Événement de jeu (tir, dégâts, etc.)
};

/**
 * @brief Types d'entités réseau
 */
enum class EntityType : std::uint8_t {
    Player = 0,
    Enemy = 1,
    Missile = 2,
    Powerup = 3,
    Boss = 4
};

/**
 * @brief Types d'événements de jeu
 */
enum class GameEvent : std::uint8_t {
    PlayerShoot = 0,
    PlayerHit = 1,
    EnemyDestroyed = 2,
    PowerupCollected = 3,
    LevelComplete = 4
};

// Force l'alignement sur 1 byte pour le réseau
#pragma pack(push, 1)

/**
 * @brief En-tête commun à tous les paquets
 */
struct PacketHeader {
    PacketType type;          // Type de paquet
    std::uint16_t sequence;   // Numéro de séquence
    std::uint32_t ack;        // Dernier paquet reçu
    std::uint32_t ackBitfield; // Bitfield des 32 paquets précédents

    PacketHeader() = default;

    PacketHeader(PacketType t, std::uint16_t seq = 0)
        : type(t), sequence(seq), ack(0), ackBitfield(0) {}
};

/**
 * @brief Paquet d'input du client
 */
struct InputPacket {
    PacketHeader header;
    std::uint32_t clientTick;  // Tick client pour la réconciliation
    std::uint8_t inputs;       // Bitfield des inputs (UP|DOWN|LEFT|RIGHT|SHOOT)

    InputPacket() : header(PacketType::ClientInput) {}
};

/**
 * @brief État d'une entité dans le GameState
 */
struct EntityState {
    std::uint32_t entityId;   // ID serveur de l'entité
    EntityType type;          // Type d'entité
    float x;                  // Position X
    float y;                  // Position Y
    float vx;                 // Vélocité X
    float vy;                 // Vélocité Y
    std::uint8_t health;      // Points de vie (0-255)
    std::uint8_t flags;       // Flags (bit 0 = isAlive, etc.)
};

/**
 * @brief Paquet d'état du jeu (serveur -> client)
 */
struct GameStatePacket {
    PacketHeader header;
    std::uint32_t serverTick;  // Tick serveur
    std::uint32_t lastProcessedInput; // Dernier input traité (pour réconciliation)
    std::uint8_t entityCount;  // Nombre d'entités dans ce paquet
    // Suivi par entityCount * EntityState

    GameStatePacket() : header(PacketType::ServerGameState), entityCount(0) {}
};

/**
 * @brief Paquet de spawn d'entité
 */
struct SpawnPacket {
    PacketHeader header;
    std::uint32_t entityId;
    EntityType type;
    float x;
    float y;
    std::uint8_t health;

    SpawnPacket() : header(PacketType::ServerSpawn) {}
};

/**
 * @brief Paquet de despawn d'entité
 */
struct DespawnPacket {
    PacketHeader header;
    std::uint32_t entityId;

    DespawnPacket() : header(PacketType::ServerDespawn) {}
};

/**
 * @brief Paquet de connexion client
 */
struct ConnectPacket {
    PacketHeader header;
    char playerName[32];  // Nom du joueur (null-terminated)

    ConnectPacket() : header(PacketType::ClientConnect) {
        std::memset(playerName, 0, sizeof(playerName));
    }
};

/**
 * @brief Paquet d'acceptation serveur
 */
struct AcceptPacket {
    PacketHeader header;
    std::uint32_t playerId;    // ID assigné au joueur
    std::uint32_t serverTick;  // Tick serveur actuel
    float tickRate;            // Taux de tick du serveur

    AcceptPacket() : header(PacketType::ServerAccept) {}
};

/**
 * @brief Paquet de ping/pong
 */
struct PingPacket {
    PacketHeader header;
    std::uint64_t timestamp;   // Timestamp en microsecondes

    PingPacket() : header(PacketType::ClientPing) {}
};

/**
 * @brief Paquet d'événement de jeu
 */
struct EventPacket {
    PacketHeader header;
    GameEvent event;
    std::uint32_t entityId;    // Entité concernée
    std::uint32_t targetId;    // Cible (optionnel)
    std::int16_t value;        // Valeur (dégâts, score, etc.)

    EventPacket() : header(PacketType::ServerEvent) {}
};

#pragma pack(pop)

/**
 * @brief Classe utilitaire pour sérialiser/désérialiser les paquets
 */
class PacketSerializer {
public:
    /**
     * @brief Sérialise un paquet en bytes
     */
    template<typename T>
    static std::vector<std::uint8_t> serialize(const T& packet) {
        std::vector<std::uint8_t> data(sizeof(T));
        std::memcpy(data.data(), &packet, sizeof(T));
        return data;
    }

    /**
     * @brief Sérialise un GameStatePacket avec les entités
     */
    static std::vector<std::uint8_t> serializeGameState(
        const GameStatePacket& packet,
        const std::vector<EntityState>& entities)
    {
        std::size_t totalSize = sizeof(GameStatePacket) + entities.size() * sizeof(EntityState);
        std::vector<std::uint8_t> data(totalSize);

        // Copie l'en-tête
        std::memcpy(data.data(), &packet, sizeof(GameStatePacket));

        // Copie les entités
        if (!entities.empty()) {
            std::memcpy(
                data.data() + sizeof(GameStatePacket),
                entities.data(),
                entities.size() * sizeof(EntityState)
            );
        }

        return data;
    }

    /**
     * @brief Désérialise un paquet depuis des bytes
     */
    template<typename T>
    static bool deserialize(const std::uint8_t* data, std::size_t size, T& packet) {
        if (size < sizeof(T)) return false;
        std::memcpy(&packet, data, sizeof(T));
        return true;
    }

    /**
     * @brief Désérialise un GameStatePacket avec les entités
     */
    static bool deserializeGameState(
        const std::uint8_t* data,
        std::size_t size,
        GameStatePacket& packet,
        std::vector<EntityState>& entities)
    {
        if (size < sizeof(GameStatePacket)) return false;

        std::memcpy(&packet, data, sizeof(GameStatePacket));

        std::size_t expectedSize = sizeof(GameStatePacket) + packet.entityCount * sizeof(EntityState);
        if (size < expectedSize) return false;

        entities.resize(packet.entityCount);
        if (packet.entityCount > 0) {
            std::memcpy(
                entities.data(),
                data + sizeof(GameStatePacket),
                packet.entityCount * sizeof(EntityState)
            );
        }

        return true;
    }

    /**
     * @brief Extrait le type de paquet depuis les données brutes
     */
    static PacketType getPacketType(const std::uint8_t* data, std::size_t size) {
        if (size < sizeof(PacketType)) return static_cast<PacketType>(0);
        return static_cast<PacketType>(data[0]);
    }
};

} // namespace rtype::network
