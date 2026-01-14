# Système de Logging

## Vue d'Ensemble

Le projet R-Type utilise **spdlog**, une bibliothèque de logging haute performance pour C++, pour tous ses besoins de journalisation. Le système est conçu pour être thread-safe, performant, et facile à utiliser.

## Architecture

### Loggers Client (6 loggers spécialisés)

| Logger | Namespace | Niveau | Utilisation |
|--------|-----------|--------|-------------|
| **NetworkLogger** | `client::logging::Logger::getNetworkLogger()` | DEBUG | TCPClient, connexions réseau, paquets |
| **EngineLogger** | `client::logging::Logger::getEngineLogger()` | INFO | Engine, GameLoop, lifecycle |
| **GraphicsLogger** | `client::logging::Logger::getGraphicsLogger()` | INFO | Rendu, textures, AssetManager |
| **SceneLogger** | `client::logging::Logger::getSceneLogger()` | INFO | SceneManager, transitions, événements |
| **UILogger** | `client::logging::Logger::getUILogger()` | INFO | Composants UI, interactions |
| **BootLogger** | `client::logging::Logger::getBootLogger()` | INFO | Initialisation, démarrage |

### Loggers Serveur (6 loggers spécialisés)

| Logger | Namespace | Niveau | Utilisation |
|--------|-----------|--------|-------------|
| **NetworkLogger** | `server::logging::Logger::getNetworkLogger()` | DEBUG | TCPServer, UDPServer, connexions |
| **AuthLogger** | `server::logging::Logger::getAuthLogger()` | INFO | Authentification, sécurité |
| **DomainLogger** | `server::logging::Logger::getDomainLogger()` | INFO | Logique métier, entités |
| **PersistenceLogger** | `server::logging::Logger::getPersistenceLogger()` | INFO | MongoDB, repositories |
| **GameLogger** | `server::logging::Logger::getGameLogger()` | INFO | Sessions de jeu, gameplay |
| **MainLogger** | `server::logging::Logger::getMainLogger()` | INFO | Démarrage, arrêt serveur |

## Configuration

### Sinks (Sorties)

Chaque logger utilise **2 sinks** simultanés:

#### 1. Console Sink
```cpp
auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
console_sink->set_level(spdlog::level::trace);
console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
```

**Caractéristiques:**
- Couleurs automatiques selon niveau
- Thread-safe (`_mt` suffix)
- Tous niveaux affichés (trace à critical)

#### 2. File Sink (Rotating)
```cpp
// Client: 10MB, 3 fichiers archivés
auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    "logs/client.log", 1024 * 1024 * 10, 3);

// Serveur: 10MB, 5 fichiers archivés (volume plus élevé)
auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    "logs/server.log", 1024 * 1024 * 10, 5);
```

**Rotation automatique:**
- Taille max: 10MB par fichier
- Archives: 3 (client) ou 5 (serveur) fichiers gardés
- Ancien fichier supprimé automatiquement
- Empêche saturation disque

### Pattern de Formatage

```
[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [LoggerName] message
```

**Exemple:**
```
[2025-11-20 22:45:12.345] [info] [Network] Connecting to 127.0.0.1:4125...
[2025-11-20 22:45:12.456] [debug] [Network] IO thread started
[2025-11-20 22:45:12.567] [info] [Boot] Connected to server!
```

## Utilisation

### Initialisation

#### Client
```cpp
// main.cpp - au tout début
#include "core/Logger.hpp"

int main() {
    client::logging::Logger::init();

    try {
        // ... code application ...
    } catch (const std::exception& e) {
        auto logger = client::logging::Logger::getBootLogger();
        logger->error("Fatal error: {}", e.what());
    }

    client::logging::Logger::shutdown();
    return 0;
}
```

#### Serveur
```cpp
// main.cpp - au tout début
#include "infrastructure/logging/Logger.hpp"

int main() {
    server::logging::Logger::init();

    auto logger = server::logging::Logger::getMainLogger();
    logger->info("Server starting...");

    // ... code serveur ...

    logger->info("Server shutting down");
    server::logging::Logger::shutdown();
    return 0;
}
```

### Dans le Code

#### Exemple Network (Client)
```cpp
#include "core/Logger.hpp"

void TCPClient::connect(const std::string& host, uint16_t port) {
    auto logger = client::logging::Logger::getNetworkLogger();

    logger->info("Connecting to {}:{}...", host, port);

    try {
        // ... code connexion ...
        logger->info("Connected successfully");
    } catch (const std::exception& e) {
        logger->error("Connection failed: {}", e.what());
        throw;
    }
}
```

#### Exemple AssetManager (Client)
```cpp
#include "core/Logger.hpp"

bool AssetManager::registerTexture(const std::string& file) {
    auto logger = client::logging::Logger::getGraphicsLogger();

    if (textures.count(file)) {
        logger->debug("Texture '{}' already registered", file);
        return false;
    }

    sf::Texture newTexture;
    if (!newTexture.loadFromFile(file)) {
        logger->error("Failed to load texture from file: {}", file);
        return false;
    }

    textures.emplace(file, std::move(newTexture));
    logger->debug("Texture '{}' registered successfully", file);
    return true;
}
```

#### Exemple Authentification (Serveur)
```cpp
#include "infrastructure/logging/Logger.hpp"

bool authenticateUser(const std::string& username, const std::string& password) {
    auto logger = server::logging::Logger::getAuthLogger();

    logger->info("Authentication attempt for user: {}", username);

    // ... vérification ...

    if (success) {
        logger->info("User {} logged in successfully", username);
        return true;
    } else {
        logger->warn("Failed authentication attempt for user: {}", username);
        return false;
    }
}
```

## Niveaux de Log

### Hiérarchie

```
trace < debug < info < warn < error < critical
```

### Quand Utiliser

| Niveau | Utilisation | Exemple |
|--------|-------------|---------|
| **trace** | Debugging très détaillé | Chaque itération de boucle, états internes |
| **debug** | Debugging normal | Entrée/sortie de fonction, états importants |
| **info** | Événements normaux | Connexion établie, fichier chargé |
| **warn** | Situations anormales mais non critiques | Configuration par défaut, ressource absente |
| **error** | Erreurs récupérables | Échec connexion, fichier introuvable |
| **critical** | Erreurs fatales | Corruption mémoire, assertion failed |

### Exemples Concrets

```cpp
auto logger = client::logging::Logger::getNetworkLogger();

// Trace: détails internes
logger->trace("Packet buffer: {:02x}", buffer);

// Debug: flow du programme
logger->debug("TCPClient created");
logger->debug("IO thread started");

// Info: événements normaux
logger->info("Connecting to {}:{}", host, port);
logger->info("Connected successfully");

// Warn: situations inhabituelles
logger->warn("Slow response time: {}ms", latency);
logger->warn("Already connected, disconnecting...");

// Error: échecs
logger->error("Connection failed: {}", error.message());
logger->error("Failed to load texture from file: {}", file);

// Critical: fatal
logger->critical("Out of memory!");
logger->critical("Assertion failed: {}", condition);
```

## Formatage

### Syntaxe fmt

spdlog utilise la bibliothèque [fmt](https://fmt.dev/) pour le formatage:

```cpp
// Basique
logger->info("Hello {}", "World");  // Hello World

// Multiples paramètres
logger->info("User {} logged in from {}", username, ip);

// Types
logger->info("Count: {}", 42);              // Entier
logger->info("PI: {:.2f}", 3.14159);        // Float avec précision
logger->info("Hex: {:x}", 255);             // Hexadécimal: ff
logger->info("Binary: {:b}", 42);           // Binaire: 101010

// Alignement
logger->info("Name: {:>20}", name);         // Aligné à droite
logger->info("Value: {:<10}", value);       // Aligné à gauche

// Padding
logger->info("ID: {:05d}", id);             // 00042
```

### Exemples Avancés

```cpp
// Conteneurs (avec fmt)
std::vector<int> vec = {1, 2, 3};
logger->debug("Vector: {}", fmt::join(vec, ", "));  // Vector: 1, 2, 3

// Pointeurs
logger->debug("Pointer address: {:p}", static_cast<void*>(ptr));

// Chrono
auto duration = std::chrono::milliseconds(123);
logger->info("Duration: {}ms", duration.count());

// Exceptions
try {
    // ...
} catch (const std::exception& e) {
    logger->error("Exception caught: {}", e.what());
}
```

## Performance

### Overhead

- Logging **asynchrone** possible (non activé par défaut)
- Overhead synchrone: < 1% en mode INFO
- Mode DEBUG plus verbeux mais toujours acceptable
- `flush()` automatique seulement sur warn+ (pas à chaque log)

### Optimisations

```cpp
// ✅ Bon: formatage lazy
logger->debug("Value: {}", expensiveFunction());
// expensiveFunction() n'est appelée que si niveau DEBUG actif

// ❌ Mauvais: concaténation eagerly
logger->debug("Value: " + std::to_string(expensiveFunction()));
// expensiveFunction() appelée même si DEBUG désactivé
```

### Désactiver Niveaux

```cpp
// En production, désactiver debug/trace
s_networkLogger->set_level(spdlog::level::info);

// Compiler sans DEBUG logs (optionnel)
#ifdef NDEBUG
#define LOG_DEBUG(...)
#else
#define LOG_DEBUG(...) logger->debug(__VA_ARGS__)
#endif
```

## Thread Safety

### Garanties

- ✅ Tous les loggers sont **thread-safe** par défaut
- ✅ Sinks utilisent suffix `_mt` (multi-threaded)
- ✅ Pas de data races entre threads
- ✅ Ordre des messages préservé par logger

### Utilisation Multi-Thread

```cpp
// Thread 1
std::thread t1([]{
    auto logger = client::logging::Logger::getNetworkLogger();
    logger->info("Thread 1 message");
});

// Thread 2
std::thread t2([]{
    auto logger = client::logging::Logger::getNetworkLogger();
    logger->info("Thread 2 message");
});

// Pas de corruption, ordre préservé
```

## Fichiers de Log

### Emplacement

```
project_root/
├── logs/
│   ├── client.log          # Logs actuels client
│   ├── client.log.1        # Archive 1
│   ├── client.log.2        # Archive 2
│   ├── server.log          # Logs actuels serveur
│   ├── server.log.1        # Archive 1
│   └── ...                 # Plus d'archives serveur
└── ...
```

### Rotation

**Comportement:**
1. Fichier atteint 10MB
2. `client.log` → `client.log.1`
3. `client.log.1` → `client.log.2`
4. `client.log.2` → `client.log.3` (puis supprimé si max atteint)
5. Nouveau `client.log` créé

**Avantages:**
- Pas de saturation disque
- Historique récent conservé
- Analyse facilitée (fichiers de taille raisonnable)

### .gitignore

Le dossier `logs/` est ignoré par git:

```gitignore
# .gitignore
logs/
```

## Debugging

### Activer Trace

```cpp
// Temporairement pour debugging intense
s_networkLogger->set_level(spdlog::level::trace);
spdlog::set_level(spdlog::level::trace);
```

### Filtrer par Logger

```bash
# Voir uniquement logs Network
grep "\[Network\]" logs/client.log

# Voir uniquement erreurs
grep "\[error\]" logs/client.log

# Dernières 100 lignes
tail -100 logs/client.log
```

### Suivre en Temps Réel

```bash
# Tail logs
tail -f logs/client.log

# Avec filtre couleur
tail -f logs/client.log | grep --color "error\|warn"
```

## Bonnes Pratiques

### À Faire ✅

```cpp
// Contexte dans les messages
logger->info("User {} logged in from {}", username, ip);

// Logger avant operations potentiellement faillibles
logger->debug("Attempting to load texture: {}", filename);
if (!texture.loadFromFile(filename)) {
    logger->error("Failed to load texture");
}

// Logger dans catch blocks
try {
    // ...
} catch (const std::exception& e) {
    logger->error("Exception: {}", e.what());
    throw;
}

// Niveaux appropriés
logger->debug("Internal state: {}", state);     // Debug
logger->info("Connection established");         // Info
logger->warn("Using default configuration");    // Warn
logger->error("Failed to connect: {}", err);    // Error
```

### À Éviter ❌

```cpp
// Messages sans contexte
logger->info("Success");  // Success de quoi?

// Logging excessif dans boucles critiques
for (int i = 0; i < 1000000; i++) {
    logger->trace("Iteration {}", i);  // Trop verbeux!
}

// Exceptions sans logging
try {
    // ...
} catch (...) {
    // Erreur silencieuse!
}

// Niveau inapproprié
logger->critical("User clicked button");  // Pas critical!
```

## Dépannage

### Logger::init() échoue

**Symptômes:**
```
Logger initialization failed: ...
```

**Solutions:**
1. Vérifier que dossier `logs/` existe (créé par script compile.sh)
2. Vérifier permissions d'écriture
3. Vérifier espace disque disponible

### Logs non visibles

**Vérifications:**
1. `Logger::init()` appelé au démarrage?
2. Niveau de log approprié? (info vs debug)
3. Fichier `logs/client.log` créé?
4. Logger correct utilisé?

### Performance dégradée

**Solutions:**
1. Réduire niveau de log (debug → info)
2. Activer logging asynchrone (avancé)
3. Augmenter taille rotation (10MB → 50MB)

## Migration depuis std::cout

### Avant
```cpp
std::cout << "[TCPClient] Connexion à " << host << ":" << port << std::endl;
std::cerr << "[TCPClient] Erreur: " << error << std::endl;
```

### Après
```cpp
auto logger = client::logging::Logger::getNetworkLogger();
logger->info("Connecting to {}:{}", host, port);
logger->error("Error: {}", error);
```

### Avantages
- ✅ Thread-safe
- ✅ Fichiers de log persistants
- ✅ Niveaux de log
- ✅ Formatage type-safe
- ✅ Performance meilleure

## Ressources

- **spdlog GitHub**: https://github.com/gabime/spdlog
- **fmt Library**: https://fmt.dev/
- **Documentation officielle**: https://spdlog.docsforge.com/

## Voir Aussi

- [Architecture Client](../client/architecture/overview.md)
- [CI/CD](ci-cd.md)
- [Debugging](../reference/faq.md)
