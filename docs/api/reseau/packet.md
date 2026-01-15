---
tags:
  - api
  - reseau
  - packet
---

# Packet

Classe de base pour tous les paquets réseau.

## Synopsis

```cpp
#include "network/Packet.hpp"

// Création
Packet packet(PacketType::Login);
packet.write(loginData);

// Envoi
auto bytes = packet.serialize();
socket.send(bytes);

// Réception
Packet received;
received.deserialize(bytes);
auto login = received.as<LoginPacket>();
```

---

## Déclaration

```cpp
namespace rtype::network {

class Packet {
public:
    // Constructors
    Packet();
    explicit Packet(PacketType type);

    // Header
    uint32_t magic() const;
    PacketType type() const;
    uint16_t size() const;

    // Payload access
    const std::vector<uint8_t>& payload() const;
    std::vector<uint8_t>& payload();

    // Type conversion
    template<typename T>
    T as() const;

    template<typename T>
    void write(const T& data);

    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    bool deserialize(const uint8_t* data, size_t length);

    // Validation
    bool isValid() const;
    static bool validateMagic(uint32_t magic);

private:
    uint32_t magic_ = Protocol::MAGIC;
    PacketType type_ = PacketType::Login;
    std::vector<uint8_t> payload_;
};

} // namespace rtype::network
```

---

## Structure Binaire

```
Offset  Size  Field
──────────────────────
0       4     magic (0x52545950)
4       1     type
5       2     payload_size
7       N     payload
```

---

## Méthodes

### `as<T>()`

```cpp
template<typename T>
T as() const;
```

Convertit le payload en structure typée.

**Template:**

| Type | Contrainte |
|------|------------|
| `T` | Doit avoir `TYPE` et `deserialize()` |

**Exemple:**

```cpp
void handlePacket(const Packet& packet) {
    switch (packet.type()) {
        case PacketType::Login:
            handleLogin(packet.as<LoginPacket>());
            break;

        case PacketType::Chat:
            handleChat(packet.as<ChatPacket>());
            break;
    }
}
```

---

### `write<T>()`

```cpp
template<typename T>
void write(const T& data);
```

Écrit une structure dans le payload.

**Exemple:**

```cpp
LoginPacket login;
std::strcpy(login.username, "player1");
std::strcpy(login.passwordHash, hash.c_str());

Packet packet(PacketType::Login);
packet.write(login);
```

---

### `serialize()`

```cpp
std::vector<uint8_t> serialize() const;
```

Sérialise le paquet complet.

**Retour:** Vecteur de bytes prêt à envoyer

**Format:**

```cpp
std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(7 + payload_.size());

    // Magic (4 bytes, little-endian)
    result.push_back(magic_ & 0xFF);
    result.push_back((magic_ >> 8) & 0xFF);
    result.push_back((magic_ >> 16) & 0xFF);
    result.push_back((magic_ >> 24) & 0xFF);

    // Type (1 byte)
    result.push_back(static_cast<uint8_t>(type_));

    // Size (2 bytes, little-endian)
    uint16_t size = static_cast<uint16_t>(payload_.size());
    result.push_back(size & 0xFF);
    result.push_back((size >> 8) & 0xFF);

    // Payload
    result.insert(result.end(), payload_.begin(), payload_.end());

    return result;
}
```

---

### `deserialize()`

```cpp
bool deserialize(const std::vector<uint8_t>& data);
bool deserialize(const uint8_t* data, size_t length);
```

Désérialise un paquet depuis des bytes.

**Retour:** `true` si valide, `false` sinon

**Validation:**

- Magic number correct
- Taille suffisante pour header
- Payload size cohérent

```cpp
bool Packet::deserialize(const uint8_t* data, size_t length) {
    if (length < 7) return false;  // Header minimum

    // Read magic
    magic_ = data[0] | (data[1] << 8) |
             (data[2] << 16) | (data[3] << 24);

    if (!validateMagic(magic_)) return false;

    // Read type
    type_ = static_cast<PacketType>(data[4]);

    // Read size
    uint16_t payloadSize = data[5] | (data[6] << 8);

    if (length < 7 + payloadSize) return false;

    // Read payload
    payload_.assign(data + 7, data + 7 + payloadSize);

    return true;
}
```

---

## Factory Pattern

```cpp
class PacketFactory {
public:
    template<typename T>
    static Packet create(const T& data) {
        Packet packet(T::TYPE);
        packet.write(data);
        return packet;
    }

    static Packet createLogin(const std::string& user,
                              const std::string& passHash) {
        LoginPacket login;
        std::strncpy(login.username, user.c_str(), 31);
        std::strncpy(login.passwordHash, passHash.c_str(), 63);
        return create(login);
    }

    static Packet createChat(uint32_t roomId,
                            const std::string& message) {
        ChatPacket chat;
        chat.roomId = roomId;
        std::strncpy(chat.message, message.c_str(), 255);
        return create(chat);
    }
};
```

---

## Exemple Complet

```cpp
// Client: Send login
void Client::login(const std::string& user,
                   const std::string& password)
{
    auto hash = hashPassword(password);
    auto packet = PacketFactory::createLogin(user, hash);
    tcpClient_.send(packet.serialize());
}

// Server: Receive login
void Server::onData(const std::vector<uint8_t>& data) {
    Packet packet;
    if (!packet.deserialize(data)) {
        // Invalid packet
        return;
    }

    switch (packet.type()) {
        case PacketType::Login: {
            auto login = packet.as<LoginPacket>();
            handleLogin(login.username, login.passwordHash);
            break;
        }
        // ...
    }
}
```

---

## Thread Safety

`Packet` n'est **PAS** thread-safe. Chaque thread doit utiliser sa propre instance.
