# PRIORITE 1 - Plan d'Implementation Complet TLS/SSL + CSPRNG

## Resume Executif

| Critere RNCP | Objectif | Impact |
|--------------|----------|--------|
| **C7** | Securite - Implementation TLS 1.3 + CSPRNG | +15 points |
| **C6** | Etude comparative documentee | +8 points |
| **C8** | Evaluation prototypes mesurable | +10 points |
| **Total** | | **+33 points** |

**Objectif** : Passer de 77/100 a 100+ en securisant le canal TCP d'authentification.

---

# PARTIE 1 : ANALYSE COMPLETE DU CODE EXISTANT

## 1.1 Architecture Reseau Actuelle

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              SERVEUR R-Type                                  │
│                                                                              │
│  ┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐        │
│  │   TCPAuthServer   │  │     UDPServer     │  │  VoiceUDPServer   │        │
│  │    Port 4125      │  │    Port 4124      │  │    Port 4126      │        │
│  │   (PLAINTEXT!)    │  │   (Game Data)     │  │   (Voice Opus)    │        │
│  │                   │  │                   │  │                   │        │
│  │ - tcp::socket     │  │ - udp::socket     │  │ - udp::socket     │        │
│  │ - tcp::acceptor   │  │                   │  │                   │        │
│  └─────────┬─────────┘  └─────────┬─────────┘  └─────────┬─────────┘        │
│            │                      │                      │                  │
│            └──────────────────────┼──────────────────────┘                  │
│                                   │                                         │
│                      ┌────────────┴────────────┐                            │
│                      │     SessionManager      │                            │
│                      │                         │                            │
│                      │  Token Generation:      │                            │
│                      │  mt19937_64 (FAIBLE!)   │                            │
│                      │  _rng() x 4 iterations  │                            │
│                      └─────────────────────────┘                            │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ════════════════╪════════════════  RESEAU NON CHIFFRE
                                    │
┌──────────────────────────────────────────────────────────────────────────────┐
│                               CLIENT R-Type                                  │
│                                                                              │
│  ┌───────────────────┐                                                      │
│  │     TCPClient     │                                                      │
│  │                   │  LoginMessage {                                      │
│  │  _socket:         │    username[64],   // En CLAIR !                     │
│  │  tcp::socket      │    password[64]    // En CLAIR !                     │
│  │  (PLAINTEXT!)     │  }                                                   │
│  │                   │  ──────────────────────────────────►                 │
│  └───────────────────┘                                                      │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 1.2 Vulnerabilite CWE-319 : Cleartext Transmission

### Localisation Exacte : `src/client/src/network/TCPClient.cpp`

**Lignes 673-704** - Fonction `sendLoginData()` :

```cpp
void TCPClient::sendLoginData(const std::string& username, const std::string& password) {
    // Stockage des credentials pour reconnexion automatique
    {
        std::scoped_lock lock(_mutex);
        _storedUsername = username;  // Stocke en memoire
        _storedPassword = password;  // Stocke en memoire
    }

    LoginMessage login;
    // Copie du username dans le buffer (64 bytes)
    std::strncpy(login.username, username.c_str(), sizeof(login.username) - 1);
    login.username[sizeof(login.username) - 1] = '\0';
    // Copie du password dans le buffer (64 bytes)
    std::strncpy(login.password, password.c_str(), sizeof(login.password) - 1);
    login.password[sizeof(login.password) - 1] = '\0';

    Header head = {
        .isAuthenticated = false,
        .type = static_cast<uint16_t>(MessageType::Login),
        .payload_size = sizeof(login)
    };

    const size_t totalSize = Header::WIRE_SIZE + sizeof(login);
    auto buf = std::make_shared<std::vector<uint8_t>>(totalSize);

    head.to_bytes(buf->data());
    login.to_bytes(buf->data() + Header::WIRE_SIZE);

    // VULNERABILITE : Envoi en CLAIR sur TCP !
    boost::asio::async_write(
        _socket,  // tcp::socket NON CHIFFRE
        boost::asio::buffer(buf->data(), totalSize),
        [this, buf](const boost::system::error_code &error, std::size_t) {
            if (error && _onError) {
                _onError("Write error: " + error.message());
            }
        }
    );
}
```

**Impact** : Un attaquant avec `tcpdump` ou Wireshark peut lire :
- `username` : 64 bytes en clair
- `password` : 64 bytes en clair

**Preuve de concept** :
```bash
# Sur le serveur ou un MITM
sudo tcpdump -i any -X port 4125 | grep -A 20 "Login"
```

### Lignes 707-734 - Fonction `sendRegisterData()` (meme probleme)

```cpp
void TCPClient::sendRegisterData(const std::string& username, const std::string& email, const std::string& password) {
    RegisterMessage registerUser;
    std::strncpy(registerUser.username, username.c_str(), sizeof(registerUser.username) - 1);
    registerUser.username[sizeof(registerUser.username) - 1] = '\0';
    std::strncpy(registerUser.email, email.c_str(), sizeof(registerUser.email) - 1);
    registerUser.email[sizeof(registerUser.email) - 1] = '\0';
    std::strncpy(registerUser.password, password.c_str(), sizeof(registerUser.password) - 1);
    registerUser.password[sizeof(registerUser.password) - 1] = '\0';

    // ... envoi en CLAIR aussi
}
```

## 1.3 Vulnerabilite CWE-338 : Weak PRNG

### Localisation Exacte : `src/server/infrastructure/session/SessionManager.cpp`

**Lignes 13-23** - Fonction `generateToken()` :

```cpp
SessionToken SessionManager::generateToken() {
    SessionToken token;
    // Generate 32 bytes of random data using mt19937_64
    // Each call to _rng() produces 8 bytes
    for (size_t i = 0; i < TOKEN_SIZE; i += 8) {
        uint64_t val = _rng();  // mt19937_64 - PREDICTIBLE !
        size_t remaining = std::min(static_cast<size_t>(8), TOKEN_SIZE - i);
        std::memcpy(token.bytes + i, &val, remaining);
    }
    return token;
}
```

### Declaration du RNG : `src/server/include/infrastructure/session/SessionManager.hpp`

**Lignes 222-224** (fin de la classe SessionManager) :

```cpp
private:
    // ... autres membres ...

    std::random_device _rd;
    std::mt19937_64 _rng{_rd()};  // Mersenne Twister 64-bit
```

**Probleme** : Mersenne Twister (`mt19937_64`) n'est PAS un CSPRNG :
- Etat interne de 624 x 64 bits = 19968 bits
- Apres avoir observe ~624 outputs, l'etat peut etre reconstitue
- Un attaquant peut predire les futurs tokens

## 1.4 Etat Actuel des Dependances

### vcpkg.json (ligne 1-26)

```json
{
  "name": "rtype",
  "version": "0.0.1",
  "builtin-baseline": "b8a81820356e90917e5eb5dfe0092bfac57dbb12",
  "dependencies": [
    "boost-asio",       // Inclut boost::asio::ssl
    "gtest",
    "mongo-cxx-driver",
    "openssl",          // DEJA PRESENT ! Pas besoin d'ajouter
    "protobuf",
    "sdl2",
    "sdl2-image",
    "sdl2-ttf",
    "sdl2-mixer",
    "sfml",
    "spdlog",
    "opus",
    "portaudio"
  ]
}
```

### CMakeLists Serveur (lignes 193-200)

```cmake
target_link_libraries(rtype_server PRIVATE
    Boost::system
    OpenSSL::SSL       # DEJA LINKE !
    OpenSSL::Crypto    # DEJA LINKE !
    spdlog::spdlog
    # ... mongo ...
)
```

### CMakeLists Client (lignes 373-392)

```cmake
target_link_libraries(rtype_client PRIVATE
    rtype_client_common
    Boost::system
    OpenSSL::SSL       # DEJA LINKE !
    OpenSSL::Crypto    # DEJA LINKE !
    # ... autres ...
)
```

**Conclusion** : OpenSSL est deja lie aux deux executables, pas de modification CMake necessaire !

---

# PARTIE 2 : ANALYSE DETAILLEE DES FICHIERS A MODIFIER

## 2.1 TCPAuthServer (Serveur)

### Header : `src/server/include/infrastructure/adapters/in/network/TCPAuthServer.hpp`

**Structure actuelle de Session** (lignes 50-140) :

```cpp
class Session: public std::enable_shared_from_this<Session> {
    private:
        tcp::socket _socket;              // LIGNE A MODIFIER
        char _readBuffer[BUFFER_SIZE];
        std::vector<uint8_t> _accumulator;
        std::atomic<bool> _isAuthenticated;
        // ... 15 autres membres ...
        boost::asio::steady_timer _timeoutTimer;

    public:
        Session(tcp::socket socket,       // LIGNE A MODIFIER
            std::shared_ptr<IUserRepository> userRepository,
            std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
            std::shared_ptr<IIdGenerator> idGenerator,
            std::shared_ptr<ILogger> logger,
            std::shared_ptr<SessionManager> sessionManager,
            std::shared_ptr<RoomManager> roomManager);
        ~Session();

        void start();
    // ... methodes privees ...
};
```

**Structure actuelle de TCPAuthServer** (lignes 143-165) :

```cpp
class TCPAuthServer {
    private:
        boost::asio::io_context& _io_ctx;
        // MANQUE: ssl::context _sslContext;
        std::shared_ptr<IUserRepository> _userRepository;
        std::shared_ptr<IUserSettingsRepository> _userSettingsRepository;
        std::shared_ptr<IIdGenerator> _idGenerator;
        std::shared_ptr<ILogger> _logger;
        std::shared_ptr<SessionManager> _sessionManager;
        std::shared_ptr<RoomManager> _roomManager;
        tcp::acceptor _acceptor;

        void start_accept();
        // MANQUE: void initSSLContext();

    public:
        TCPAuthServer(
            boost::asio::io_context& io_ctx,
            // MANQUE: const std::string& certFile,
            // MANQUE: const std::string& keyFile,
            std::shared_ptr<IUserRepository> userRepository,
            std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
            std::shared_ptr<IIdGenerator> idGenerator,
            std::shared_ptr<ILogger> logger,
            std::shared_ptr<SessionManager> sessionManager,
            std::shared_ptr<RoomManager> roomManager);
        void start();
        void run();
        void stop();
        // ...
};
```

### Implementation : `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp`

**Constructeur Session actuel** (lignes 22-38) :

```cpp
Session::Session(
    tcp::socket socket,
    std::shared_ptr<IUserRepository> userRepository,
    std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
    std::shared_ptr<IIdGenerator> idGenerator,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SessionManager> sessionManager,
    std::shared_ptr<RoomManager> roomManager)
: _socket(std::move(socket)), _isAuthenticated(false),
  _userRepository(userRepository), _userSettingsRepository(userSettingsRepository),
  _idGenerator(idGenerator), _logger(logger),
  _sessionManager(sessionManager), _roomManager(roomManager),
  _timeoutTimer(_socket.get_executor())
{
    _onAuthSuccess = [this](const User& user) { onLoginSuccess(user); };
    _lastActivity = std::chrono::steady_clock::now();
}
```

**Constructeur TCPAuthServer actuel** (lignes 501-516) :

```cpp
TCPAuthServer::TCPAuthServer(
    boost::asio::io_context& io_ctx,
    std::shared_ptr<IUserRepository> userRepository,
    std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
    std::shared_ptr<IIdGenerator> idGenerator,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SessionManager> sessionManager,
    std::shared_ptr<RoomManager> roomManager)
    : _io_ctx(io_ctx), _userRepository(userRepository),
      _userSettingsRepository(userSettingsRepository),
      _idGenerator(idGenerator), _logger(logger),
      _sessionManager(sessionManager), _roomManager(roomManager),
      _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4125)) {
    auto networkLogger = server::logging::Logger::getNetworkLogger();
    networkLogger->info("TCP Auth Server started on port 4125");
}
```

**Methode start_accept() actuelle** (lignes 530-546) :

```cpp
void TCPAuthServer::start_accept() {
    auto networkLogger = server::logging::Logger::getNetworkLogger();

    _acceptor.async_accept(
        [this, networkLogger](boost::system::error_code ec, tcp::socket socket) {
            if (ec) {
                if (ec != boost::asio::error::operation_aborted) {
                    networkLogger->error("Accept error: {}", ec.message());
                }
                return;
            }
            networkLogger->info("TCP New connection accepted!");
            // PROBLEME: Pas de handshake TLS !
            std::make_shared<Session>(
                std::move(socket),  // Socket TCP brut
                _userRepository,
                _userSettingsRepository,
                _idGenerator,
                _logger,
                _sessionManager,
                _roomManager
            )->start();
            start_accept();
        }
    );
}
```

## 2.2 TCPClient (Client)

### Header : `src/client/include/network/TCPClient.hpp`

**Membres actuels** (lignes 118-125) :

```cpp
private:
    // ... methodes ...

    boost::asio::io_context _ioContext;
    tcp::socket _socket;                    // LIGNE A MODIFIER
    std::jthread _ioThread;
    boost::asio::steady_timer _heartbeatTimer;
    // MANQUE: ssl::context _sslContext;
```

### Implementation : `src/client/src/network/TCPClient.cpp`

**Constructeur actuel** (lignes 22-26) :

```cpp
TCPClient::TCPClient()
    : _socket(_ioContext),                  // Socket TCP brut
      _heartbeatTimer(_ioContext),
      _isAuthenticated(false),
      _isWriting(false)
{
    client::logging::Logger::getNetworkLogger()->debug("TCPClient created");
}
```

**Methode connect() actuelle** (lignes 56-112) - Extrait pertinent :

```cpp
void TCPClient::connect(const std::string &host, std::uint16_t port)
{
    // ... validation ...

    _ioContext.restart();

    // Recreer le socket si necessaire
    if (_socket.is_open()) {
        boost::system::error_code ec;
        _socket.shutdown(tcp::socket::shutdown_both, ec);
        _socket.close(ec);
    }
    _socket = tcp::socket(_ioContext);  // Nouveau socket TCP brut

    try {
        tcp::resolver resolver(_ioContext);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        asyncConnect(endpoints);  // Lance la connexion
        // ...
    }
}
```

**Methode asyncConnect() actuelle** (lignes 176-185) :

```cpp
void TCPClient::asyncConnect(tcp::resolver::results_type endpoints)
{
    boost::asio::async_connect(
        _socket,  // Socket TCP brut, pas de TLS
        endpoints,
        [this](const boost::system::error_code &error, const tcp::endpoint &) {
            handleConnect(error);  // Directement, sans handshake TLS
        }
    );
}
```

**Methode disconnect() actuelle** (lignes 114-164) - Extrait :

```cpp
void TCPClient::disconnect()
{
    // ... flags ...

    _heartbeatTimer.cancel();
    _ioContext.stop();

    boost::system::error_code ec;
    _socket.shutdown(tcp::socket::shutdown_both, ec);  // Shutdown TCP
    _socket.close(ec);

    // ... cleanup ...

    _ioContext.restart();
    _socket = tcp::socket(_ioContext);  // Recreer socket TCP

    // ...
}
```

## 2.3 GameBootstrap (Point d'entree serveur)

### Fichier : `src/server/infrastructure/bootstrap/GameBootstrap.hpp`

**Instantiation actuelle de TCPAuthServer** (lignes 92-94) :

```cpp
// Start TCP Auth Server on port 4125
TCPAuthServer tcpAuthServer(
    io_ctx,
    userRepo,
    userSettingsRepo,
    idGenerator,
    logger,
    sessionManager,
    roomManager
);
tcpAuthServer.start();
```

---

# PARTIE 3 : IMPLEMENTATION TLS COMPLETE

## 3.1 Modifications TCPAuthServer.hpp

### Ajouts en debut de fichier (apres ligne 10)

```cpp
// AJOUTER apres les includes existants
#include <boost/asio/ssl.hpp>
```

### Modifications dans le namespace (apres ligne 36)

```cpp
namespace infrastructure::adapters::in::network {
    using boost::asio::ip::tcp;
    namespace ssl = boost::asio::ssl;  // AJOUTER cette ligne
```

### Modification de la classe Session

**Remplacer ligne 52** :
```cpp
// AVANT:
tcp::socket _socket;

// APRES:
ssl::stream<tcp::socket> _socket;
```

**Modifier le constructeur (ligne 131-140)** :
```cpp
// AVANT:
Session(tcp::socket socket, ...);

// APRES:
Session(ssl::stream<tcp::socket> socket, ...);
```

### Modification de la classe TCPAuthServer

**Ajouter apres `_io_ctx` (ligne 145)** :
```cpp
private:
    boost::asio::io_context& _io_ctx;
    ssl::context _sslContext;      // AJOUTER
    std::string _certFile;         // AJOUTER
    std::string _keyFile;          // AJOUTER
    std::shared_ptr<IUserRepository> _userRepository;
    // ... reste inchange ...
```

**Ajouter methode privee (avant start_accept)** :
```cpp
    void initSSLContext();  // AJOUTER
    void start_accept();
```

**Modifier le constructeur public** :
```cpp
public:
    TCPAuthServer(
        boost::asio::io_context& io_ctx,
        const std::string& certFile,    // AJOUTER
        const std::string& keyFile,     // AJOUTER
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SessionManager> sessionManager,
        std::shared_ptr<RoomManager> roomManager);
```

## 3.2 Modifications TCPAuthServer.cpp

### Ajouter include (apres ligne 15)

```cpp
#include <openssl/ssl.h>
```

### Nouveau constructeur Session (remplace lignes 22-38)

```cpp
Session::Session(
    ssl::stream<tcp::socket> socket,  // Type modifie
    std::shared_ptr<IUserRepository> userRepository,
    std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
    std::shared_ptr<IIdGenerator> idGenerator,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SessionManager> sessionManager,
    std::shared_ptr<RoomManager> roomManager)
: _socket(std::move(socket)),
  _isAuthenticated(false),
  _userRepository(userRepository),
  _userSettingsRepository(userSettingsRepository),
  _idGenerator(idGenerator),
  _logger(logger),
  _sessionManager(sessionManager),
  _roomManager(roomManager),
  _timeoutTimer(_socket.get_executor())  // get_executor() fonctionne avec ssl::stream
{
    _onAuthSuccess = [this](const User& user) { onLoginSuccess(user); };
    _lastActivity = std::chrono::steady_clock::now();
}
```

### Modifier destructeur Session (lignes 40-72)

Ajouter avant la fermeture du socket :
```cpp
Session::~Session()
{
    auto logger = server::logging::Logger::getNetworkLogger();
    if (_isAuthenticated && _user.has_value()) {
        // ... code existant pour notify player leave ...
    }

    // AJOUTER: Shutdown SSL proprement
    boost::system::error_code ec;
    _socket.shutdown(ec);  // SSL shutdown
    if (ec && ec != boost::asio::error::eof && ec != boost::asio::ssl::error::stream_truncated) {
        logger->debug("SSL shutdown notice: {}", ec.message());
    }
}
```

### Nouveau constructeur TCPAuthServer (remplace lignes 501-516)

```cpp
TCPAuthServer::TCPAuthServer(
    boost::asio::io_context& io_ctx,
    const std::string& certFile,
    const std::string& keyFile,
    std::shared_ptr<IUserRepository> userRepository,
    std::shared_ptr<IUserSettingsRepository> userSettingsRepository,
    std::shared_ptr<IIdGenerator> idGenerator,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SessionManager> sessionManager,
    std::shared_ptr<RoomManager> roomManager)
    : _io_ctx(io_ctx)
    , _sslContext(ssl::context::tlsv12_server)
    , _certFile(certFile)
    , _keyFile(keyFile)
    , _userRepository(userRepository)
    , _userSettingsRepository(userSettingsRepository)
    , _idGenerator(idGenerator)
    , _logger(logger)
    , _sessionManager(sessionManager)
    , _roomManager(roomManager)
    , _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 4125))
{
    initSSLContext();

    auto networkLogger = server::logging::Logger::getNetworkLogger();
    networkLogger->info("TCP Auth Server started on port 4125 with TLS 1.2+");
}
```

### Nouvelle methode initSSLContext() (ajouter avant start())

```cpp
void TCPAuthServer::initSSLContext() {
    auto networkLogger = server::logging::Logger::getNetworkLogger();

    // Desactiver les protocoles obsoletes
    _sslContext.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::no_sslv3 |
        ssl::context::no_tlsv1 |
        ssl::context::no_tlsv1_1 |
        ssl::context::single_dh_use
    );

    // Forcer TLS 1.2 minimum
    SSL_CTX_set_min_proto_version(_sslContext.native_handle(), TLS1_2_VERSION);

    // Charger certificat et cle privee
    try {
        _sslContext.use_certificate_chain_file(_certFile);
        networkLogger->info("TLS Certificate loaded: {}", _certFile);

        _sslContext.use_private_key_file(_keyFile, ssl::context::pem);
        networkLogger->info("TLS Private key loaded: {}", _keyFile);
    } catch (const std::exception& e) {
        networkLogger->error("Failed to load TLS certificates: {}", e.what());
        throw;
    }

    // Cipher suites modernes (AEAD uniquement, forward secrecy)
    SSL_CTX_set_cipher_list(_sslContext.native_handle(),
        "ECDHE-ECDSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-ECDSA-AES128-GCM-SHA256:"
        "ECDHE-RSA-AES128-GCM-SHA256:"
        "ECDHE-ECDSA-CHACHA20-POLY1305:"
        "ECDHE-RSA-CHACHA20-POLY1305"
    );

    networkLogger->info("TLS context initialized successfully");
}
```

### Nouvelle methode start_accept() (remplace lignes 530-546)

```cpp
void TCPAuthServer::start_accept() {
    auto networkLogger = server::logging::Logger::getNetworkLogger();

    // Creer un nouveau ssl::stream pour chaque connexion entrante
    auto sslSocket = std::make_shared<ssl::stream<tcp::socket>>(_io_ctx, _sslContext);

    _acceptor.async_accept(
        sslSocket->lowest_layer(),  // Accepter sur le socket TCP sous-jacent
        [this, sslSocket, networkLogger](boost::system::error_code ec) {
            if (ec) {
                if (ec != boost::asio::error::operation_aborted) {
                    networkLogger->error("Accept error: {}", ec.message());
                }
                // Continuer a accepter meme en cas d'erreur
                start_accept();
                return;
            }

            auto clientEndpoint = sslSocket->lowest_layer().remote_endpoint();
            networkLogger->debug("TCP connection from {}, starting TLS handshake...",
                clientEndpoint.address().to_string());

            // Handshake TLS asynchrone
            sslSocket->async_handshake(
                ssl::stream_base::server,
                [this, sslSocket, networkLogger, clientEndpoint](boost::system::error_code hsError) {
                    if (hsError) {
                        networkLogger->warn("TLS handshake failed from {}: {}",
                            clientEndpoint.address().to_string(), hsError.message());
                        // Ne pas creer de session si le handshake echoue
                        return;
                    }

                    networkLogger->info("TLS handshake successful from {}",
                        clientEndpoint.address().to_string());

                    // Creer la session avec le socket SSL securise
                    auto session = std::make_shared<Session>(
                        std::move(*sslSocket),
                        _userRepository,
                        _userSettingsRepository,
                        _idGenerator,
                        _logger,
                        _sessionManager,
                        _roomManager
                    );
                    session->start();
                }
            );

            // Continuer a accepter de nouvelles connexions
            start_accept();
        }
    );
}
```

## 3.3 Modifications TCPClient.hpp

### Ajouter include (apres ligne 11)

```cpp
#include <boost/asio/ssl.hpp>
```

### Ajouter namespace alias (apres ligne 26)

```cpp
using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;  // AJOUTER
```

### Modifier les membres prives (lignes 118-125)

```cpp
private:
    // ... methodes privees ...

    boost::asio::io_context _ioContext;
    ssl::context _sslContext;               // AJOUTER
    ssl::stream<tcp::socket> _socket;       // MODIFIER (etait tcp::socket)
    std::jthread _ioThread;
    boost::asio::steady_timer _heartbeatTimer;
```

### Ajouter declaration de methode

```cpp
private:
    void initSSLContext();  // AJOUTER
    void asyncConnect(tcp::resolver::results_type endpoints);
    // ... reste ...
```

## 3.4 Modifications TCPClient.cpp

### Ajouter include (apres ligne 15)

```cpp
#include <openssl/ssl.h>
```

### Nouveau constructeur (remplace lignes 22-26)

```cpp
TCPClient::TCPClient()
    : _ioContext()
    , _sslContext(ssl::context::tlsv12_client)
    , _socket(_ioContext, _sslContext)
    , _heartbeatTimer(_ioContext)
    , _isAuthenticated(false)
    , _isWriting(false)
{
    initSSLContext();
    client::logging::Logger::getNetworkLogger()->debug("TCPClient created with TLS support");
}
```

### Nouvelle methode initSSLContext() (ajouter apres constructeur)

```cpp
void TCPClient::initSSLContext() {
    auto logger = client::logging::Logger::getNetworkLogger();

    // Verifier le certificat du serveur (mode production)
    // Pour dev avec certificat auto-signe, utiliser verify_none
    _sslContext.set_verify_mode(ssl::verify_none);  // DEV: changer en verify_peer pour prod

    // Charger les certificats CA du systeme
    _sslContext.set_default_verify_paths();

    // Forcer TLS 1.2 minimum
    SSL_CTX_set_min_proto_version(_sslContext.native_handle(), TLS1_2_VERSION);

    logger->debug("SSL context initialized (TLS 1.2+)");
}
```

### Modifier connect() (lignes 56-112)

Remplacer la partie recreation du socket :

```cpp
void TCPClient::connect(const std::string &host, std::uint16_t port)
{
    auto logger = client::logging::Logger::getNetworkLogger();

    if (_connected.load() || _connecting.load()) {
        logger->warn("Already connected or connecting, disconnecting...");
        disconnect();
    }

    if (_ioThread.joinable()) {
        _ioThread.join();
    }

    logger->info("Connecting to {}:{} with TLS...", host, port);

    _lastHost = host;
    _lastPort = port;

    _disconnecting.store(false);
    _connecting.store(true);

    _ioContext.restart();

    // MODIFIER: Recreer le ssl::stream (ne peut pas etre reutilise)
    _socket = ssl::stream<tcp::socket>(_ioContext, _sslContext);

    try {
        tcp::resolver resolver(_ioContext);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        asyncConnect(endpoints);

        _ioThread = std::jthread([this, logger]() {
            logger->debug("IO thread started");
            _ioContext.run();
            logger->debug("IO thread terminated");
        });

        logger->info("TLS connection initiated");
    } catch (const std::exception &e) {
        logger->error("Resolution error: {}", e.what());
        _connecting.store(false);
        _eventQueue.push(TCPErrorEvent{std::string("Connexion echouee: ") + e.what()});
        if (_onError) {
            _onError(std::string("Connexion echouee: ") + e.what());
        }
    }
}
```

### Nouvelle methode asyncConnect() (remplace lignes 176-185)

```cpp
void TCPClient::asyncConnect(tcp::resolver::results_type endpoints)
{
    auto logger = client::logging::Logger::getNetworkLogger();

    // Etape 1: Connexion TCP sur le socket sous-jacent
    boost::asio::async_connect(
        _socket.lowest_layer(),  // Socket TCP sous-jacent
        endpoints,
        [this, logger](const boost::system::error_code &error, const tcp::endpoint &ep) {
            if (error) {
                handleConnect(error);
                return;
            }

            logger->debug("TCP connected to {}:{}, starting TLS handshake...",
                ep.address().to_string(), ep.port());

            // Etape 2: Handshake TLS
            _socket.async_handshake(
                ssl::stream_base::client,
                [this, logger](const boost::system::error_code &hsError) {
                    if (hsError) {
                        logger->error("TLS handshake failed: {}", hsError.message());
                        _connecting.store(false);
                        _eventQueue.push(TCPErrorEvent{"TLS handshake echoue: " + hsError.message()});
                        if (_onError) {
                            _onError("TLS handshake echoue: " + hsError.message());
                        }
                        disconnect();
                        return;
                    }

                    logger->info("TLS handshake successful");
                    handleConnect(boost::system::error_code{});
                }
            );
        }
    );
}
```

### Modifier disconnect() (lignes 114-164)

```cpp
void TCPClient::disconnect()
{
    if (_disconnecting.exchange(true)) {
        return;
    }

    bool wasConnected = _connected.load();
    bool wasConnecting = _connecting.load();

    if (!wasConnected && !wasConnecting) {
        _disconnecting.store(false);
        return;
    }

    auto logger = client::logging::Logger::getNetworkLogger();
    logger->info("Disconnecting TCP+TLS...");

    _connecting.store(false);
    _connected.store(false);

    _heartbeatTimer.cancel();

    // AJOUTER: SSL shutdown avant de fermer le socket
    boost::system::error_code ec;
    _socket.shutdown(ec);
    if (ec && ec != boost::asio::error::eof && ec != boost::asio::ssl::error::stream_truncated) {
        logger->debug("SSL shutdown: {}", ec.message());
    }

    _ioContext.stop();

    // Fermer le socket TCP sous-jacent
    _socket.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
    _socket.lowest_layer().close(ec);

    _accumulator.clear();

    if (wasConnected) {
        _eventQueue.push(TCPDisconnectedEvent{});
        if (_onDisconnected) {
            _onDisconnected();
        }
    }

    _ioContext.restart();

    // MODIFIER: Recreer le ssl::stream pour future connexion
    _socket = ssl::stream<tcp::socket>(_ioContext, _sslContext);

    while (!_sendQueue.empty()) {
        _sendQueue.pop();
    }
    _isWriting = false;

    logger->info("TCP+TLS disconnected successfully");
}
```

## 3.5 Fix CSPRNG - SessionManager

### Modifier SessionManager.cpp (lignes 7-23)

**Ajouter include** :
```cpp
#include "infrastructure/session/SessionManager.hpp"
#include <algorithm>
#include <openssl/rand.h>  // AJOUTER
```

**Remplacer generateToken()** :
```cpp
SessionToken SessionManager::generateToken() {
    SessionToken token;

    // Utiliser le CSPRNG d'OpenSSL (cryptographiquement securise)
    if (RAND_bytes(token.bytes, TOKEN_SIZE) != 1) {
        // Echec extremement rare - le systeme n'a pas assez d'entropie
        throw std::runtime_error("CSPRNG failure: cannot generate secure session token");
    }

    return token;
}
```

### Modifier SessionManager.hpp (lignes 222-224)

**Supprimer ces lignes** :
```cpp
// SUPPRIMER:
// std::random_device _rd;
// std::mt19937_64 _rng{_rd()};
```

## 3.6 Modifier GameBootstrap.hpp (lignes 92-94)

```cpp
// Start TCP Auth Server on port 4125 with TLS
const char* certFile = std::getenv("TLS_CERT_FILE");
const char* keyFile = std::getenv("TLS_KEY_FILE");

TCPAuthServer tcpAuthServer(
    io_ctx,
    certFile ? certFile : "certs/server.crt",
    keyFile ? keyFile : "certs/server.key",
    userRepo,
    userSettingsRepo,
    idGenerator,
    logger,
    sessionManager,
    roomManager
);
tcpAuthServer.start();
```

---

# PARTIE 4 : GENERATION DES CERTIFICATS

## 4.1 Script de Generation

**Creer fichier** : `scripts/generate_dev_certs.sh`

```bash
#!/bin/bash
#===============================================================================
# R-Type - Generation de certificats TLS pour le developpement
#===============================================================================

set -e

CERT_DIR="${1:-certs}"
DAYS_VALID=365
KEY_SIZE=4096

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║     R-Type - Generation de Certificats TLS (Developpement)   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "Repertoire de sortie: $CERT_DIR"
echo "Validite: $DAYS_VALID jours"
echo "Taille cle RSA: $KEY_SIZE bits"
echo ""

# Creer le repertoire
mkdir -p "$CERT_DIR"

# Generer la cle privee RSA
echo "[1/3] Generation de la cle privee RSA..."
openssl genrsa -out "$CERT_DIR/server.key" $KEY_SIZE 2>/dev/null

# Generer le certificat auto-signe avec SAN
echo "[2/3] Generation du certificat auto-signe..."
openssl req -new -x509 \
    -key "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.crt" \
    -days $DAYS_VALID \
    -subj "/CN=localhost/O=R-Type Development/OU=Game Server/C=FR" \
    -addext "subjectAltName=DNS:localhost,DNS:rtype.local,IP:127.0.0.1,IP:::1" \
    -addext "basicConstraints=critical,CA:FALSE" \
    -addext "keyUsage=digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth"

# Verifier le certificat
echo "[3/3] Verification du certificat..."
echo ""
openssl x509 -in "$CERT_DIR/server.crt" -noout -subject -dates -ext subjectAltName

# Definir les permissions
chmod 600 "$CERT_DIR/server.key"
chmod 644 "$CERT_DIR/server.crt"

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                  Certificats generes !                       ║"
echo "╠══════════════════════════════════════════════════════════════╣"
echo "║  Certificat: $CERT_DIR/server.crt"
echo "║  Cle privee: $CERT_DIR/server.key"
echo "╠══════════════════════════════════════════════════════════════╣"
echo "║  IMPORTANT: Ces certificats sont pour le DEVELOPPEMENT      ║"
echo "║  En production, utilisez Let's Encrypt ou une CA reconnue   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
```

## 4.2 Utilisation

```bash
# Rendre executable
chmod +x scripts/generate_dev_certs.sh

# Generer les certificats
./scripts/generate_dev_certs.sh

# Les fichiers sont crees dans certs/
ls -la certs/
# -rw------- server.key
# -rw-r--r-- server.crt

# Lancer le serveur
./artifacts/server/linux/rtype_server

# Lancer le client
./artifacts/client/linux/rtype_client
```

---

# PARTIE 5 : TESTS DE VALIDATION

## 5.1 Test TLS avec OpenSSL s_client

```bash
# Test connexion TLS basique
openssl s_client -connect localhost:4125 -tls1_2

# Verifier TLS 1.3 (si supporte)
openssl s_client -connect localhost:4125 -tls1_3

# Ces commandes DOIVENT echouer apres implementation:
openssl s_client -connect localhost:4125 -ssl3   # SSLv3 refuse
openssl s_client -connect localhost:4125 -tls1   # TLS 1.0 refuse
openssl s_client -connect localhost:4125 -tls1_1 # TLS 1.1 refuse
```

## 5.2 Test avec Wireshark

```bash
# Capturer le trafic AVANT TLS
sudo tcpdump -i lo -w before_tls.pcap port 4125

# Se connecter et s'authentifier

# Analyser: Les credentials sont VISIBLES en clair

# Capturer le trafic APRES TLS
sudo tcpdump -i lo -w after_tls.pcap port 4125

# Analyser: On voit "TLS Handshake" puis "Application Data" chiffre
```

## 5.3 Tests Unitaires CSPRNG

**Fichier** : `tests/server/session/SessionManagerCryptoTest.cpp`

```cpp
#include <gtest/gtest.h>
#include "infrastructure/session/SessionManager.hpp"
#include <set>
#include <cmath>

using namespace infrastructure::session;

// Test: Les tokens sont uniques
TEST(SessionManagerCryptoTest, TokensAreUnique) {
    SessionManager sm;
    std::set<std::string> tokens;

    for (int i = 0; i < 10000; ++i) {
        auto result = sm.createSession(
            "user" + std::to_string(i) + "@test.com",
            "User" + std::to_string(i)
        );
        ASSERT_TRUE(result.has_value());

        std::string hex = result->token.toHex();
        ASSERT_EQ(tokens.count(hex), 0) << "Token collision at iteration " << i;
        tokens.insert(hex);

        // Nettoyer la session pour eviter "already has session"
        sm.removeSession("user" + std::to_string(i) + "@test.com");
    }
}

// Test: Les tokens ont une bonne entropie
TEST(SessionManagerCryptoTest, TokenEntropy) {
    SessionManager sm;

    auto result = sm.createSession("entropy@test.com", "Test");
    ASSERT_TRUE(result.has_value());

    // Compter les bits a 1
    int bitsSet = 0;
    for (size_t i = 0; i < TOKEN_SIZE; ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            if (result->token.bytes[i] & (1 << bit)) {
                bitsSet++;
            }
        }
    }

    // Un bon CSPRNG devrait avoir ~50% de bits a 1
    // Avec 256 bits, on attend ~128 +/- 16 (3 sigma)
    int totalBits = TOKEN_SIZE * 8;
    int expected = totalBits / 2;
    int tolerance = static_cast<int>(3 * std::sqrt(totalBits / 4.0));

    EXPECT_GE(bitsSet, expected - tolerance)
        << "Too few 1-bits: " << bitsSet << " (expected ~" << expected << ")";
    EXPECT_LE(bitsSet, expected + tolerance)
        << "Too many 1-bits: " << bitsSet << " (expected ~" << expected << ")";
}

// Test: Les tokens ne sont pas tous zeros ou tous uns
TEST(SessionManagerCryptoTest, TokenNotTrivial) {
    SessionManager sm;

    auto result = sm.createSession("trivial@test.com", "Test");
    ASSERT_TRUE(result.has_value());

    bool allZero = true, allOnes = true;
    for (size_t i = 0; i < TOKEN_SIZE; ++i) {
        if (result->token.bytes[i] != 0x00) allZero = false;
        if (result->token.bytes[i] != 0xFF) allOnes = false;
    }

    EXPECT_FALSE(allZero) << "Token is all zeros!";
    EXPECT_FALSE(allOnes) << "Token is all ones!";
}
```

---

# PARTIE 6 : RESUME DES FICHIERS

## 6.1 Fichiers a Modifier

| Fichier | Modifications | Complexite |
|---------|---------------|------------|
| `src/server/include/.../TCPAuthServer.hpp` | +ssl namespace, changer types | Moyenne |
| `src/server/infrastructure/.../TCPAuthServer.cpp` | +initSSLContext(), modifier start_accept() | Haute |
| `src/client/include/network/TCPClient.hpp` | +ssl namespace, changer types | Moyenne |
| `src/client/src/network/TCPClient.cpp` | +initSSLContext(), modifier connect/disconnect | Haute |
| `src/server/include/.../SessionManager.hpp` | Supprimer _rd, _rng | Faible |
| `src/server/infrastructure/.../SessionManager.cpp` | Utiliser RAND_bytes() | Faible |
| `src/server/infrastructure/bootstrap/GameBootstrap.hpp` | Ajouter params certFile/keyFile | Faible |

## 6.2 Fichiers a Creer

| Fichier | Description |
|---------|-------------|
| `scripts/generate_dev_certs.sh` | Script generation certificats dev |
| `certs/server.crt` | Certificat serveur (genere) |
| `certs/server.key` | Cle privee serveur (genere) |
| `tests/server/session/SessionManagerCryptoTest.cpp` | Tests CSPRNG |

## 6.3 Ordre d'Implementation

1. **Etape 1** : Creer et executer `scripts/generate_dev_certs.sh`
2. **Etape 2** : Modifier `SessionManager.hpp` et `SessionManager.cpp` (CSPRNG)
3. **Etape 3** : Modifier `TCPAuthServer.hpp` et `TCPAuthServer.cpp`
4. **Etape 4** : Modifier `GameBootstrap.hpp`
5. **Etape 5** : Compiler et tester le serveur avec `openssl s_client`
6. **Etape 6** : Modifier `TCPClient.hpp` et `TCPClient.cpp`
7. **Etape 7** : Test integration client-serveur
8. **Etape 8** : Capture Wireshark pour validation

---

# PARTIE 7 : CHECKLIST RNCP

## C6 - Etude Comparative

- [ ] Creer `docs/RNCP/C6_ETUDE_COMPARATIVE.md`
- [ ] Comparer TLS 1.2 vs TLS 1.3 vs DTLS
- [ ] Comparer cipher suites (AES-GCM vs ChaCha20)
- [ ] Comparer mt19937 vs CSPRNG vs hardware RNG
- [ ] Documenter le choix final avec justification

## C7 - Audit Securite

- [ ] Creer `docs/RNCP/C7_AUDIT_SECURITE.md`
- [ ] Documenter CWE-319 (cleartext) avec code avant/apres
- [ ] Documenter CWE-338 (weak PRNG) avec code avant/apres
- [ ] Inclure captures Wireshark avant/apres TLS
- [ ] Inclure resultat `openssl s_client`
- [ ] Lister les recommandations futures

## C8 - Evaluation Prototypes

- [ ] Creer `docs/RNCP/C8_EVALUATION_PROTOTYPES.md`
- [ ] Mesurer latence handshake TLS
- [ ] Mesurer impact CPU du chiffrement
- [ ] Comparer avec baseline non-chiffre
- [ ] Documenter les metriques du voice chat
- [ ] Documenter l'architecture multi-backend graphique

---

**Document genere le** : 2025-01-14
**Version** : 2.0 (Complete avec analyse de code)
**Auteur** : Analyse Claude Code
