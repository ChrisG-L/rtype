# API Reference - Infrastructure Adapters

**État:** ✅ Implémenté (Network + GameWorld + Collision)
**Version:** 0.5.1
**Dernière mise à jour:** Décembre 2025

---

## 📋 Vue d'Ensemble

Les Adapters de l'Infrastructure Layer implémentent les **Ports** définis dans l'Application Layer. Ils constituent l'interface entre le Domain et le monde extérieur (réseau, base de données, CLI).

**Architecture Hexagonale:**
```
┌─────────────────────────────────────────────┐
│      Infrastructure Layer (Adapters)        │
│                                              │
│  ┌──────────────────┐  ┌─────────────────┐ │
│  │   Adapters IN    │  │   Adapters OUT  │ │
│  │   (Driving)      │  │   (Driven)      │ │
│  ├──────────────────┤  ├─────────────────┤ │
│  │ • UDPServer      │  │ • MongoDB       │ │
│  │ • TCPAuthServer  │  │ • Repositories  │ │
│  │ • VoiceUDPServer │  │                 │ │
│  │ • CLI Controller │  │                 │ │
│  └─────────┬────────┘  └────────┬────────┘ │
└────────────┼──────────────────────┼──────────┘
             │                      │
             ▼                      ▼
    ┌────────────────────────────────────┐
    │      Application Layer (Ports)     │
    │  • IUserRepository (OUT)          │
    │  • IUserSettingsRepository (OUT)  │
    │  • ILogger (OUT)                  │
    └────────────────────────────────────┘
```

### Types d'Adapters

| Type | Direction | Rôle | Exemples |
|------|-----------|------|----------|
| **Adapters IN** | Driving | Pilotent l'application (entrées) | UDPServer, TCPAuthServer, VoiceUDPServer, CLI |
| **Adapters OUT** | Driven | Pilotés par l'application (sorties) | MongoDB, Repositories |

---

## 📡 Adapters IN - Network

### UDPServer

**Fichier:** `infrastructure/adapters/in/network/UDPServer.hpp`
**Namespace:** `infrastructure::adapters::in::network`
**Port:** 4124 (UDP)

**Description:** Serveur UDP asynchrone pour le gameplay temps réel. Gère les connexions clients, reçoit les commandes (PlayerInput, ShootMissile) et broadcast l'état de jeu à 20Hz via GameSnapshot.

#### Caractéristiques

- **Protocole:** UDP binaire (62 types de messages)
- **Port:** 4124
- **Broadcast:** 20Hz (50ms)
- **GameWorld:** Joueurs, missiles, 5 types d'ennemis
- **Collision:** AABB hitboxes avec damage events
- **Use Case:** Positions joueurs, tirs, spawns ennemis

#### Attributs Privés

```cpp
private:
    udp::socket _socket;                    // Socket UDP Boost.Asio
    boost::asio::io_context& _io_ctx;       // Référence au contexte I/O
    udp::endpoint _remote_endpoint;         // Endpoint du client distant
    std::array<char, 1024> _recv_buffer;    // Buffer de réception (1024 bytes)
```

#### Constructeur

```cpp
explicit UDPServer(boost::asio::io_context& io_ctx);
```

**Paramètres:**
- `io_ctx` - Référence au contexte I/O Boost.Asio (event loop)

**Action:** Initialise le socket UDP sur port 4124.

#### Méthodes Publiques

```cpp
void start(boost::asio::io_context& io_ctx);
void run();
void stop();
```

| Méthode | Description | Bloquante |
|---------|-------------|-----------|
| `start()` | Démarre l'écoute UDP (lance async_receive_from) | Non |
| `run()` | Lance la boucle événementielle io_context | Oui |
| `stop()` | Arrête le serveur proprement | Non |

#### Méthodes Privées

```cpp
private:
    void start_receive();
    void handle_receive(const boost::system::error_code& error,
                       std::size_t bytes_transferred);
```

| Méthode | Description |
|---------|-------------|
| `start_receive()` | Enregistre un async_receive_from (callback) |
| `handle_receive()` | Callback appelé quand un paquet UDP arrive |

#### Flux de Données

```
Client UDP
    │
    │ Paquet UDP (position, tir, etc.)
    ▼
UDPServer::start_receive()
    │ async_receive_from()
    ▼
UDPServer::handle_receive()
    │ Parse données
    │ Appelle Use Case
    ▼
start_receive() (boucle)
```

#### Exemple d'Utilisation

```cpp
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include <boost/asio.hpp>

using namespace infrastructure::adapters::in::network;

int main() {
    // 1. Créer contexte I/O
    boost::asio::io_context io_ctx;

    // 2. Créer serveur UDP
    UDPServer udpServer(io_ctx);

    // 3. Démarrer l'écoute
    udpServer.start(io_ctx);

    std::cout << "Serveur UDP prêt sur port 4124" << std::endl;

    // 4. Lancer boucle événementielle (bloque ici)
    udpServer.run();

    return 0;
}
```

#### État Actuel

**Implémenté:**
- ✅ Socket UDP sur port 4124
- ✅ Protocole binaire (62 types de messages)
- ✅ GameWorld (joueurs, missiles, ennemis)
- ✅ Broadcast GameSnapshot à 20Hz
- ✅ 5 types d'ennemis avec IA unique
- ✅ Collision AABB avec damage events
- ✅ Wave spawning automatique

**Fonctionnalités:**
- PlayerInput, ShootMissile, PlayerJoin/Leave
- MissileSpawned/Destroyed, EnemySpawned/Destroyed
- GameSnapshot synchronisation temps réel

---

### TCPAuthServer + Session

**Fichiers:** `infrastructure/adapters/in/network/TCPAuthServer.hpp`
**Namespace:** `infrastructure::adapters::in::network`
**Port:** 4125 (TCP/TLS)

**Description:** Serveur TCP/TLS asynchrone pour l'authentification sécurisée. Utilise TLS 1.2+ pour chiffrer toutes les communications (credentials, tokens). Chaque connexion crée une `Session`.

#### Architecture

```
TCPServer (Acceptor)
    │
    │ Accepte connexion
    ▼
Session (par client)
    │
    │ async_read_some()
    ▼
Traitement données
    │
    ▼
async_write() (réponse)
```

---

### Session

**Description:** Représente une connexion TCP client. Pattern `std::enable_shared_from_this` pour gestion du cycle de vie asynchrone.

#### Attributs Privés

```cpp
private:
    tcp::socket _socket;                  // Socket TCP
    static constexpr std::size_t max_length = 1024;
    char _data[max_length];               // Buffer de données
```

#### Constructeur

```cpp
explicit Session(tcp::socket socket);
```

**Paramètres:**
- `socket` - Socket TCP déplacé depuis l'acceptor

#### Méthodes Publiques

```cpp
void start();
```

**Description:** Démarre la lecture asynchrone sur le socket.

#### Méthodes Privées

```cpp
private:
    void do_read();
    void do_write(std::size_t length);
```

| Méthode | Description |
|---------|-------------|
| `do_read()` | Lance async_read_some (callback) |
| `do_write()` | Lance async_write (envoie réponse) |

#### Pattern shared_from_this

```cpp
void Session::do_read() {
    auto self = shared_from_this();  // Garde la session vivante

    _socket.async_read_some(
        boost::asio::buffer(_data, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                do_write(length);  // Echo pour test
            }
            // Si ec, la session se détruit automatiquement
        }
    );
}
```

**Pourquoi `shared_from_this()`?**
- Garantit que la Session reste en vie pendant l'opération async
- Évite les dangling pointers si le client déconnecte
- Destruction automatique quand toutes les opérations sont terminées

---

### TCPServer

**Description:** Accepte les connexions TCP entrantes et crée une `Session` par client.

#### Attributs Privés

```cpp
private:
    tcp::acceptor _acceptor;              // Acceptor Boost.Asio
    boost::asio::io_context& _io_ctx;     // Référence au contexte I/O
```

#### Constructeur

```cpp
explicit TCPServer(boost::asio::io_context& io_ctx);
```

**Paramètres:**
- `io_ctx` - Référence au contexte I/O Boost.Asio

**Action:** Initialise l'acceptor TCP/TLS sur port 4125.

#### Méthodes Publiques

```cpp
void start(boost::asio::io_context& io_ctx);
void run();
```

| Méthode | Description | Bloquante |
|---------|-------------|-----------|
| `start()` | Démarre l'acceptation de connexions | Non |
| `run()` | Lance la boucle événementielle | Oui |

#### Méthodes Privées

```cpp
private:
    void start_accept();
```

**Description:** Enregistre un async_accept pour la prochaine connexion.

#### Flux de Données

```
Client TCP
    │
    │ Connexion TCP
    ▼
TCPServer::start_accept()
    │ async_accept()
    ▼
Callback: Créer Session
    │
    ▼
Session::start()
    │ async_read_some()
    ▼
Session::do_read()
    │ Parse données (LOGIN, etc.)
    │ Appelle Use Case (LoginUserUseCase)
    ▼
Session::do_write() (réponse)
    │
    ▼
Session::do_read() (boucle)
```

#### Exemple d'Utilisation

```cpp
#include "infrastructure/adapters/in/network/TCPServer.hpp"
#include <boost/asio.hpp>

using namespace infrastructure::adapters::in::network;

int main() {
    boost::asio::io_context io_ctx;

    // Créer serveur TCP
    TCPServer tcpServer(io_ctx);

    // Démarrer l'acceptation
    tcpServer.start(io_ctx);

    std::cout << "Serveur TCP/TLS prêt sur port 4125" << std::endl;

    // Lancer boucle événementielle (bloque)
    tcpServer.run();

    return 0;
}
```

#### Exemple Complet (UDP + TCP)

```cpp
#include "infrastructure/adapters/in/network/UDPServer.hpp"
#include "infrastructure/adapters/in/network/TCPServer.hpp"
#include <boost/asio.hpp>
#include <thread>

int main() {
    boost::asio::io_context io_ctx;

    // Serveurs UDP (gameplay) et TCP/TLS (auth) séparés
    UDPServer udpServer(io_ctx);       // Port 4124
    TCPAuthServer tcpAuthServer(io_ctx, "certs/server.crt", "certs/server.key");  // Port 4125

    udpServer.start(io_ctx);
    tcpAuthServer.start(io_ctx);

    std::cout << "UDP :4124 (gameplay) | TCP/TLS :4125 (auth)" << std::endl;

    // Un seul io_context.run() gère les deux serveurs
    io_ctx.run();

    return 0;
}
```

**Note:** Le gameplay utilise UDP (port 4124). TCP/TLS (port 4125) est utilisé pour l'authentification sécurisée.

#### État Actuel

**Implémenté:**
- ✅ Acceptor TCP/TLS sur port 4125
- ✅ Création de Session par connexion
- ✅ Lecture/écriture asynchrone
- ✅ Pattern `shared_from_this` pour gestion du cycle de vie
- ✅ Intégration avec Use Cases auth (Login, Register)

**Optionnel (gameplay via UDP):**
- TCP/TLS utilisé pour l'authentification sécurisée (TLS 1.2+)
- Gameplay temps réel via UDPServer (port 4124)

---

## 💻 Adapters IN - CLI

### CLIGameController

**Fichier:** `infrastructure/adapters/in/cli/CLIGameController.hpp`
**Namespace:** `infrastructure::adapters::in::cli`
**Port Implémenté:** `IGameCommands`

**Description:** Contrôleur CLI pour tester les commandes de jeu. Implémente le port `IGameCommands` pour piloter les Use Cases depuis la ligne de commande.

#### Attributs Privés

```cpp
private:
    application::use_cases::MovePlayerUseCase movePlayerUseCase;
```

#### Méthodes Publiques

```cpp
void movePlayer(const std::string& playerId,
                float dx, float dy, float dz = 0.0) override;
```

**Paramètres:**
- `playerId` - UUID du joueur (format MongoDB)
- `dx, dy, dz` - Déplacement relatif (delta)

**Description:** Appelle `MovePlayerUseCase` pour déplacer un joueur.

#### Exemple d'Utilisation

```cpp
#include "infrastructure/adapters/in/cli/CLIGameController.hpp"
#include "domain/value_objects/player/PlayerId.hpp"

using namespace infrastructure::adapters::in::cli;

int main() {
    CLIGameController controller;

    // Déplacer le joueur
    std::string playerId = "507f1f77bcf86cd799439011";
    controller.movePlayer(playerId, 10.0f, 5.0f, 0.0f);

    std::cout << "Player moved!" << std::endl;

    return 0;
}
```

#### État Actuel

**Implémenté:**
- ✅ Interface `IGameCommands`
- ✅ Méthode `movePlayer()`
- ✅ Intégration avec `MovePlayerUseCase`

**À implémenter:**
- ❌ Commandes supplémentaires (shoot, heal, etc.)
- ❌ Parsing d'arguments CLI
- ❌ Interface interactive (REPL)

---

## 💾 Adapters OUT - Persistence

### MongoDBConfiguration

**Fichier:** `infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp`
**Namespace:** `infrastructure::adapters::out::persistence`

**Description:** Gère la connexion à MongoDB. Configure le client MongoDB à partir d'une configuration (DBConfig).

#### Attributs Privés

```cpp
private:
    static std::unique_ptr<mongocxx::instance> _instance;  // Instance globale MongoDB
    DBConfig _dbConfig;                                     // Configuration DB
    mongocxx::uri _uri;                                     // URI de connexion
    mongocxx::client _client;                               // Client MongoDB
```

#### Constructeur

```cpp
explicit MongoDBConfiguration(const DBConfig& dbconfig);
```

**Paramètres:**
- `dbconfig` - Configuration de la base de données (URI, nom DB, etc.)

**Action:**
- Initialise l'instance MongoDB (singleton)
- Parse l'URI de connexion
- Crée le client MongoDB

**Exceptions:** `MongoDBException` si connexion échoue.

#### Méthodes Publiques

```cpp
mongocxx::database getDatabaseConfig() const;
const DBConfig& getConfig() const;
bool pingServer() const;
```

| Méthode | Retour | Description |
|---------|--------|-------------|
| `getDatabaseConfig()` | `mongocxx::database` | Retourne l'objet database MongoDB |
| `getConfig()` | `const DBConfig&` | Retourne la configuration |
| `pingServer()` | `bool` | Teste la connexion avec un ping |

#### Exemple d'Utilisation

```cpp
#include "infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp"
#include "infrastructure/configuration/DBConfig.hpp"

using namespace infrastructure::adapters::out::persistence;

int main() {
    // 1. Créer configuration
    DBConfig config;
    config.uri = "mongodb://localhost:8089";
    config.databaseName = "rtype";

    // 2. Initialiser MongoDB
    MongoDBConfiguration mongoConfig(config);

    // 3. Tester connexion
    if (mongoConfig.pingServer()) {
        std::cout << "MongoDB connecté!" << std::endl;

        // 4. Récupérer database
        auto db = mongoConfig.getDatabaseConfig();
        auto collection = db["players"];

        // Utiliser collection...
    } else {
        std::cerr << "Erreur connexion MongoDB" << std::endl;
    }

    return 0;
}
```

#### DBConfig Structure

**Fichier:** `infrastructure/configuration/DBConfig.hpp`

```cpp
struct DBConfig {
    std::string uri;            // "mongodb://localhost:8089"
    std::string databaseName;   // "rtype"
};
```

#### État Actuel

**Implémenté:**
- ✅ Connexion MongoDB
- ✅ Configuration via DBConfig
- ✅ Ping serveur
- ✅ Récupération de database
- ✅ Exception MongoDBException

**À implémenter:**
- ❌ Pool de connexions
- ❌ Reconnexion automatique
- ❌ Configuration avancée (timeout, SSL, etc.)

---

### MongoDBPlayerRepository

**Fichier:** `infrastructure/adapters/out/persistence/MongoDBPlayerRepository.hpp` [Planifié]
**Namespace:** `infrastructure::adapters::out::persistence`
**Port Implémenté:** `IPlayerRepository`

**Description:** Repository MongoDB pour les entités Player. Implémente le port `IPlayerRepository` défini dans l'Application Layer.

#### Interface IPlayerRepository

**Fichier:** `application/ports/out/persistence/IPlayerRepository.hpp`

```cpp
namespace application::ports::out::persistence {
    class IPlayerRepository {
        public:
            virtual ~IPlayerRepository() = default;
            virtual Player findById(const PlayerId& id) = 0;
            virtual void save(const Player& player) = 0;
            virtual void remove(const PlayerId& id) = 0;
            virtual std::vector<Player> findAll() = 0;
    };
}
```

#### Implémentation (Planifiée)

```cpp
namespace infrastructure::adapters::out::persistence {
    class MongoDBPlayerRepository: public IPlayerRepository {
        private:
            mongocxx::collection _collection;

        public:
            explicit MongoDBPlayerRepository(mongocxx::collection collection);

            Player findById(const PlayerId& id) override;
            void save(const Player& player) override;
            void remove(const PlayerId& id) override;
            std::vector<Player> findAll() override;
    };
}
```

#### Exemple d'Utilisation (Futur)

```cpp
// main.cpp
MongoDBConfiguration mongoConfig(dbConfig);
auto db = mongoConfig.getDatabaseConfig();
auto playersCollection = db["players"];

// Créer repository
MongoDBPlayerRepository playerRepo(playersCollection);

// Utiliser dans Use Case
MovePlayerUseCase moveUseCase(playerRepo);

// Charger joueur
PlayerId id("507f1f77bcf86cd799439011");
Player player = playerRepo.findById(id);

// Modifier et sauvegarder
player.move(10.0f, 5.0f, 0.0f);
playerRepo.save(player);
```

#### État

**État:** 📋 **Planifié** (0% - Non implémenté)

**À implémenter:**
- ❌ Méthode `findById()`
- ❌ Méthode `save()`
- ❌ Méthode `remove()`
- ❌ Méthode `findAll()`
- ❌ Conversion Player ↔ BSON
- ❌ Gestion des erreurs MongoDB

---

## 🔄 Intégration Complète

### Architecture Complète (Domain → Application → Infrastructure)

```cpp
// 1. DOMAIN - Entité métier
class Player {
    public:
        void move(float dx, float dy, float dz);
};

// 2. APPLICATION - Port de sortie (interface)
class IPlayerRepository {
    public:
        virtual Player findById(const PlayerId& id) = 0;
        virtual void save(const Player& player) = 0;
};

// 3. APPLICATION - Use Case (logique métier)
class MovePlayerUseCase {
    private:
        IPlayerRepository& _repo;

    public:
        void execute(const std::string& playerId, float dx, float dy, float dz) {
            // Charger depuis repository
            Player player = _repo.findById(PlayerId(playerId));

            // Logique métier (Domain)
            player.move(dx, dy, dz);

            // Sauvegarder
            _repo.save(player);
        }
};

// 4. INFRASTRUCTURE - Adapter OUT (implémentation)
class MongoDBPlayerRepository: public IPlayerRepository {
    public:
        Player findById(const PlayerId& id) override {
            // Requête MongoDB
            auto doc = _collection.find_one(...);
            // Convertir BSON → Domain
            return Player(...);
        }

        void save(const Player& player) override {
            // Convertir Domain → BSON
            // Upsert MongoDB
        }
};

// 5. INFRASTRUCTURE - Adapter IN (entrée)
class UDPServer {
    private:
        MovePlayerUseCase& _moveUseCase;

    public:
        void handle_receive(const char* data, size_t length) {
            // Parser paquet UDP
            auto [playerId, dx, dy, dz] = parsePacket(data, length);

            // Appeler Use Case
            _moveUseCase.execute(playerId, dx, dy, dz);

            // Broadcast aux autres clients (à impl.)
        }
};

// 6. MAIN - Wiring (injection de dépendances)
int main() {
    // Infrastructure OUT
    MongoDBConfiguration mongoConfig(dbConfig);
    auto db = mongoConfig.getDatabaseConfig();
    MongoDBPlayerRepository playerRepo(db["players"]);

    // Application
    MovePlayerUseCase moveUseCase(playerRepo);

    // Infrastructure IN
    boost::asio::io_context io_ctx;
    UDPServer udpServer(io_ctx, moveUseCase);

    udpServer.start(io_ctx);
    udpServer.run();

    return 0;
}
```

---

## 📊 Récapitulatif API

### Adapters IN (Driving)

| Adapter | Type | Port | Protocole | Statut | Use Case |
|---------|------|------|-----------|--------|----------|
| **UDPServer** | Network | 4124 | UDP binaire | ✅ Implémenté | Gameplay temps réel (20Hz) |
| **VoiceUDPServer** | Network | 4126 | UDP (Opus) | ✅ Implémenté | Chat vocal temps réel |
| **GameWorld** | Game | - | - | ✅ Implémenté | État de jeu, missiles, ennemis |
| **TCPAuthServer** | Network | 4125 | TCP/TLS | ✅ Implémenté | Authentification sécurisée |
| **CLIGameController** | CLI | - | CLI | ✅ Implémenté | Tests et debug |

### Adapters OUT (Driven)

| Adapter | Type | Port Implémenté | Statut | Use Case |
|---------|------|-----------------|--------|----------|
| **MongoDBConfiguration** | Persistence | - | ✅ Implémenté | Connexion DB |
| **MongoDBPlayerRepository** | Persistence | `IPlayerRepository` | 📋 Planifié | CRUD Player |

---

## 🎯 Patterns et Conventions

### Pattern Adapter

Tous les adapters suivent le pattern Adapter (Ports & Adapters):

1. **Interface (Port)** - Définie dans Application Layer
2. **Implémentation (Adapter)** - Définie dans Infrastructure Layer
3. **Inversion de Dépendance** - Application dépend de l'interface, pas de l'implémentation

```cpp
// Application Layer - Port (interface)
class IGameCommands {
    public:
        virtual void movePlayer(...) = 0;
};

// Infrastructure Layer - Adapter (implémentation)
class CLIGameController: public IGameCommands {
    public:
        void movePlayer(...) override {
            // Implémentation CLI
        }
};
```

### Pattern Asynchrone Boost.Asio

Tous les adapters réseau suivent le pattern asynchrone:

1. **Enregistrement** - `async_xxx()` enregistre un callback
2. **Callback** - Appelé quand l'opération termine
3. **Boucle** - Callback relance une opération async (loop)

```cpp
void start_receive() {
    _socket.async_receive_from(
        boost::asio::buffer(_recv_buffer),
        _remote_endpoint,
        [this](boost::system::error_code ec, std::size_t bytes) {
            if (!ec) {
                handle_data(bytes);
            }
            start_receive();  // Boucle
        }
    );
}
```

### Pattern Session (TCP)

Le pattern Session garantit la sécurité avec `shared_from_this()`:

```cpp
class Session: public std::enable_shared_from_this<Session> {
    void do_read() {
        auto self = shared_from_this();  // Garde vivant
        _socket.async_read_some(..., [this, self](...) { ... });
    }
};
```

**Avantages:**
- Pas de dangling pointers
- Destruction automatique propre
- Support multi-connexions sûr

---

## 🔧 Configuration

### Variables d'Environnement (Futures)

```bash
# MongoDB
RTYPE_MONGODB_URI=mongodb://localhost:8089
RTYPE_MONGODB_DB=rtype

# Network
RTYPE_UDP_PORT=4124
RTYPE_TCP_PORT=4125
RTYPE_MAX_CLIENTS=4
RTYPE_BROADCAST_RATE=20
```

### Fichier de Configuration (Futur)

```json
{
  "network": {
    "udp_port": 4124,
    "tcp_port": 4125,
    "broadcast_rate_hz": 20,
    "max_players": 4,
    "max_missiles": 32
  },
  "database": {
    "uri": "mongodb://localhost:8089",
    "name": "rtype"
  }
}
```

---

## 🧪 Tests

### Test UDPServer

```cpp
// tests/infrastructure/network/UDPServerTest.cpp
#include <gtest/gtest.h>
#include "infrastructure/adapters/in/network/UDPServer.hpp"

TEST(UDPServerTest, ReceivesPacket) {
    boost::asio::io_context io_ctx;
    UDPServer server(io_ctx);

    server.start(io_ctx);

    // TODO: Envoyer paquet test avec client UDP
    // TODO: Vérifier réception
}
```

### Test TCPServer

```cpp
// tests/infrastructure/network/TCPServerTest.cpp
TEST(TCPServerTest, AcceptsConnection) {
    boost::asio::io_context io_ctx;
    TCPServer server(io_ctx);

    server.start(io_ctx);

    // TODO: Connecter client TCP test
    // TODO: Vérifier session créée
}
```

### Test MongoDBConfiguration

```cpp
// tests/infrastructure/persistence/MongoDBConfigurationTest.cpp
TEST(MongoDBConfigurationTest, ConnectsToMongoDB) {
    DBConfig config;
    config.uri = "mongodb://localhost:8089";
    config.databaseName = "rtype_test";

    MongoDBConfiguration mongoConfig(config);

    EXPECT_TRUE(mongoConfig.pingServer());
}
```

---

## 📚 Références

### Documentation Connexe

- **Architecture Réseau:** [guides/network-architecture.md](../guides/network-architecture.md)
- **Architecture Hexagonale:** [guides/hexagonal-architecture.md](../guides/hexagonal-architecture.md)
- **Domain Layer:** [api/domain.md](domain.md)
- **Application Layer:** [api/application.md](application.md) (À créer)

### Fichiers Source

**Network Adapters:**
- `src/server/infrastructure/adapters/in/network/UDPServer.hpp` / `.cpp`
- `src/server/infrastructure/adapters/in/network/TCPServer.hpp` / `.cpp`

**CLI Adapter:**
- `src/server/infrastructure/adapters/in/cli/CLIGameController.hpp` / `.cpp`

**Persistence Adapters:**
- `src/server/infrastructure/adapters/out/persistence/MongoDBConfiguration.hpp` / `.cpp`
- `src/server/infrastructure/adapters/out/persistence/MongoDBPlayerRepository.hpp` [Planifié]

**Configuration:**
- `src/server/infrastructure/configuration/DBConfig.hpp`

### Boost.Asio

- **Documentation:** https://www.boost.org/doc/libs/release/doc/html/boost_asio.html
- **Tutoriels:** https://think-async.com/Asio/
- **Pattern Proactor:** Pattern de conception pour I/O asynchrone

### MongoDB C++ Driver

- **Documentation:** https://www.mongodb.com/docs/drivers/cxx/
- **Tutoriels:** https://www.mongodb.com/docs/languages/cpp/cpp-driver/current/get-started/

---

**Dernière révision:** Décembre 2025 par Agent Documentation
**Statut:** ✅ Complet et à jour avec le code (v0.5.1)
