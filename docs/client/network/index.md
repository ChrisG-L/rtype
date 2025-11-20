# Network - Communication Réseau Client

Le module **Network** gère toute la communication réseau du client R-Type avec le serveur, principalement via TCP pour les messages critiques et UDP pour le gameplay temps réel (planifié).

## Vue d'Ensemble

```mermaid
graph LR
    A[Client] -->|TCP Auth/Commands| B[Server]
    A -->|UDP Gameplay| B
    B -->|TCP Responses| A
    B -->|UDP State Updates| A
```

## Composants

### [TCPClient](tcp-client.md) - Client TCP Asynchrone
**Communication fiable avec le serveur**

**Fonctionnalités:**
- Connexion asynchrone via Boost.ASIO
- Envoi/Réception non-bloquant
- Callbacks pour événements réseau
- **Thread-safe** (corrections EPI-37)
- **Logging intégré** (NetworkLogger)

**Utilisation:**
```cpp
auto tcpClient = std::make_unique<TCPClient>();

// Callbacks
tcpClient->setOnConnected([]() {
    logger->info("Connected!");
});

tcpClient->setOnReceive([](const std::string& data) {
    logger->debug("Received: {}", data);
});

// Connexion
tcpClient->connect("127.0.0.1", 4123);

// Envoi
tcpClient->send("LOGIN user password");
```

**Fichiers:**
- `src/client/network/TCPClient.cpp`
- `src/client/include/network/TCPClient.hpp`

---

## Architecture Réseau

### Protocoles

| Protocole | Port | Utilisation | État |
|-----------|------|-------------|------|
| **TCP** | 4123 | Authentification, commandes | ✅ Implémenté |
| **UDP** | 4242 | Gameplay temps réel | 📋 Planifié |

### TCP vs UDP

**TCP (Implémenté):**
- ✅ Fiable (garantie de livraison)
- ✅ Ordre préservé
- ✅ Gestion connexion/déconnexion
- ❌ Plus de latence
- **Usage:** Login, commandes, chat

**UDP (Planifié):**
- ✅ Faible latence
- ✅ Moins d'overhead
- ❌ Pas de garantie livraison
- ❌ Ordre non garanti
- **Usage:** Positions, actions gameplay

## Système de Logging

Le module Network utilise **NetworkLogger** (niveau DEBUG):

```cpp
#include "core/Logger.hpp"

auto logger = client::logging::Logger::getNetworkLogger();
logger->debug("TCPClient created");
logger->info("Connecting to {}:{}...", host, port);
logger->warn("Slow response time: {}ms", latency);
logger->error("Connection failed: {}", error.message());
```

[Documentation complète du logging](../../development/logging.md)

## Thread Safety (EPI-37)

### Corrections Appliquées

#### 1. Race Condition disconnect() ✅
```cpp
// Avant (BUGUÉ)
void TCPClient::disconnect() {
    if (!_connected) return;  // ❌ Sans mutex
    _connected = false;
}

// Après (CORRIGÉ)
void TCPClient::disconnect() {
    std::scoped_lock lock(_mutex);  // ✅ Protégé
    if (!_connected) return;
    _connected = false;
}
```

#### 2. Migration std::jthread ✅
```cpp
// Avant
_ioThread = std::thread([this]() {
    _ioContext.run();
});
// Besoin de join() manuel

// Après
_ioThread = std::jthread([this, logger]() {
    logger->debug("IO thread started");
    _ioContext.run();
    logger->debug("IO thread terminated");
});
// Auto-join RAII
```

#### 3. Callbacks const-correct ✅
```cpp
// Avant
void setOnConnected(OnConnectedCallback callback);

// Après
void setOnConnected(const OnConnectedCallback& callback);
// Évite copie inutile
```

### Problèmes Restants ⚠️

**Dangling Reference asyncWrite() (ligne 167):**
```cpp
const std::string &message = _sendQueue.front();  // ⚠️ DANGLING
boost::asio::async_write(
    _socket,
    boost::asio::buffer(message),  // ⚠️ Référence après unlock
    [this](const auto& error, std::size_t) {
        handleWrite(error);
    }
);
```

**Solution planifiée:**
```cpp
std::string message = _sendQueue.front();  // Copie
```

## Diagramme de Séquence

### Connexion

```mermaid
sequenceDiagram
    participant Client
    participant TCPClient
    participant IOThread
    participant Server

    Client->>TCPClient: connect(host, port)
    TCPClient->>IOThread: start()
    TCPClient->>Server: TCP SYN
    Server->>TCPClient: TCP SYN-ACK
    TCPClient->>Server: TCP ACK
    TCPClient->>Client: onConnected()

    loop Communication
        Client->>TCPClient: send(data)
        TCPClient->>Server: TCP Data
        Server->>TCPClient: TCP Response
        TCPClient->>Client: onReceive(data)
    end

    Client->>TCPClient: disconnect()
    TCPClient->>Server: TCP FIN
    TCPClient->>IOThread: stop()
```

### Envoi de Message

```mermaid
sequenceDiagram
    participant App
    participant TCPClient
    participant IOContext
    participant Network

    App->>TCPClient: send("data")
    TCPClient->>TCPClient: _sendQueue.push(data)
    TCPClient->>TCPClient: asyncWrite()
    TCPClient->>IOContext: async_write()
    IOContext->>Network: TCP Send
    Network->>IOContext: Completion
    IOContext->>TCPClient: handleWrite()
    TCPClient->>TCPClient: _sendQueue.pop()
```

## Performance

### Optimisations

- **Asynchrone non-bloquant**: Pas de blocage du thread principal
- **File d'envoi**: Envois séquentiels garantis
- **IOContext dédié**: Thread séparé pour I/O
- **Logging conditionnel**: DEBUG seulement si activé

### Métriques Typiques

| Opération | Latence | Notes |
|-----------|---------|-------|
| connect() | ~50ms | Locale (127.0.0.1) |
| send() | <1ms | Mise en queue |
| receive | Variable | Dépend serveur |
| disconnect() | ~10ms | Fermeture propre |

## État d'Implémentation

| Composant | État | Complétude | Priorité |
|-----------|------|------------|----------|
| TCPClient | ✅ Implémenté | 85% | - |
| Thread Safety | ✅ Amélioré | 70% | Haute |
| Logging | ✅ Intégré | 100% | - |
| UDP Client | 📋 Planifié | 0% | Haute |
| Protocol Buffers | 📋 Planifié | 0% | Moyenne |
| Compression | 📋 Planifié | 0% | Basse |

## Gestion d'Erreurs

### Erreurs de Connexion

```cpp
tcpClient->setOnError([](const std::string& error) {
    auto logger = client::logging::Logger::getNetworkLogger();
    logger->error("Network error: {}", error);

    // Tentative de reconnexion?
    std::this_thread::sleep_for(std::chrono::seconds(5));
    tcpClient->connect("127.0.0.1", 4123);
});
```

### Timeout

```cpp
// Boost.ASIO deadline timer (à implémenter)
boost::asio::steady_timer timer(ioContext, std::chrono::seconds(30));
timer.async_wait([this](const boost::system::error_code& ec) {
    if (!ec && !_connected) {
        logger->error("Connection timeout");
        disconnect();
    }
});
```

## Debugging

### Logs Réseau

```cpp
// Activer DEBUG pour voir tous les paquets
auto logger = client::logging::Logger::getNetworkLogger();
logger->set_level(spdlog::level::debug);

// Output:
// [2025-11-20 22:45:12.345] [debug] [Network] TCPClient created
// [2025-11-20 22:45:12.456] [info] [Network] Connecting to 127.0.0.1:4123...
// [2025-11-20 22:45:12.567] [debug] [Network] IO thread started
// [2025-11-20 22:45:12.678] [info] [Network] Connected successfully
// [2025-11-20 22:45:12.789] [debug] [Network] Sending 15 bytes
```

### Wireshark

```bash
# Capturer trafic TCP port 4123
sudo wireshark -i lo -f "tcp port 4123"
```

## Protocole Application (Planifié)

### Format Messages

```
[4 bytes: Length][N bytes: Payload]
```

### Commandes (Exemple)

```
// Authentification
LOGIN username password

// Rejoindre partie
JOIN_GAME game_id

// Action gameplay
ACTION move 100 200

// Déconnexion
LOGOUT
```

### Réponses Serveur

```
// Succès
OK data

// Erreur
ERROR message

// State update
STATE player_positions enemy_positions
```

## Prochaines Étapes

### Court Terme
1. **Corriger asyncWrite dangling ref** (EPI-37)
2. **Protéger _readBuffer** avec mutex
3. **Tests unitaires** pour thread safety

### Moyen Terme
4. **UDP Client** pour gameplay
5. **Protocol Buffers** pour sérialisation
6. **Heartbeat/Keepalive** automatique

### Long Terme
7. **Reconnexion automatique** avec backoff
8. **Compression** (zlib/lz4)
9. **Encryption** (TLS/DTLS)

## Références

- [Boost.ASIO Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [Architecture Réseau Serveur](../../guides/network-architecture.md)
- [Système de Logging](../../development/logging.md)
- [Ticket EPI-36: Migration TCPClient Logging](https://linear.app/epitech-study/issue/EPI-36)
- [Ticket EPI-37: Thread Safety TCPClient](https://linear.app/epitech-study/issue/EPI-37)

## Voir Aussi

- [Core Client](../core/index.md)
- [Architecture Client](../architecture/overview.md)
- [Guide Réseau Serveur](../../guides/network-architecture.md)
