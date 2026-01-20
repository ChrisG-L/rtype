# RAPPORT D'INTEGRATION TIERS - R-Type

**Competences couvertes:** 12 (composants tiers), 14 (integration et erreurs)

---

## Resume Executif

R-Type utilise **13 dependances externes** organisees selon les besoins du serveur, du client et des tests. Les bibliotheques tierces couvrent :
- **Networking** : Boost.ASIO (async I/O), OpenSSL (TLS/SSL)
- **Graphics** : SFML 3, SDL2 (deux backends)
- **Audio** : PortAudio (capture/playback), Opus (codec)
- **Compression** : LZ4 (optimisation reseau)
- **Persistance** : MongoDB C++ Driver
- **Testing** : Google Test
- **Logging** : spdlog

---

## 1. DEPENDANCES COMPLETES

### Fichier: `vcpkg.json`

```json
"dependencies": [
  "boost-asio" (version 1.89.0 - override),
  "gtest",
  "mongo-cxx-driver",
  "openssl",
  "sdl2", "sdl2-image", "sdl2-ttf", "sdl2-mixer",
  "sfml",
  "spdlog",
  "opus",
  "portaudio",
  "lz4"
]
```

---

## 2. BOOST.ASIO - Networking Asynchrone

### Utilisation

| Aspect | Details |
|--------|---------|
| Version | 1.89.0 (override vcpkg) |
| Ports | UDP 4124 (game), TCP 4125 (auth), UDP 4126 (voice), TCP 4127 (admin) |
| Architectures | `io_context`, `ssl::stream<tcp::socket>`, UDP sockets |

### Fichiers Cles

1. **UDPServer** (`src/server/infrastructure/adapters/in/network/UDPServer.cpp:53-64`)
   ```cpp
   UDPServer::UDPServer(boost::asio::io_context& io_ctx, ...)
       : _io_ctx(io_ctx),
         _socket(io_ctx, udp::endpoint(udp::v4(), 4124)),
         _instanceManager(io_ctx),
         _broadcastTimer(io_ctx),
         _statsTimer(io_ctx),
         _autoSaveTimer(io_ctx)
   ```
   - Creation du socket UDP sur le port 4124
   - 6 timers asio pour gestion des delais

2. **TCPAuthServer** (`src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp:26-52`)
   ```cpp
   Session::Session(ssl::stream<tcp::socket> socket, ...)
       : _socket(std::move(socket)),
         _timeoutTimer(_socket.get_executor())
   ```
   - Stream SSL/TLS pour authentification
   - Timeout management via Boost.ASIO executor

### Gestion des Erreurs

| Type | Code | Exemple |
|------|------|---------|
| Windows ICMP | `UDPServer.cpp:66-73` | `WSAIoctl(SIO_UDP_CONNRESET)` desactive Port Unreachable |
| Read Error | `TCPAuthServer.cpp:170-186` | `boost::asio::error::operation_aborted` ignore (fermeture intentionnelle) |
| SSL Shutdown | `TCPAuthServer.cpp:112-117` | `_socket.shutdown(ec)` gere `eof` et `stream_truncated` |

---

## 3. SFML 3 - Graphics Backend #1

### Utilisation

| Aspect | Details |
|--------|---------|
| Version | 3.x (via vcpkg ou pkg-config) |
| Components | Graphics, Window, System |
| Fichiers | `lib/sfml/src/` |

### Fichiers Cles

1. **SFMLWindow** (`src/client/lib/sfml/src/SFMLWindow.cpp:95-100`)
   ```cpp
   SFMLWindow::SFMLWindow(Vec2u winSize, const std::string& name)
   {
       _window.create(sf::VideoMode({winSize.x, winSize.y}), name);
       // Letterboxing pour taille initiale
   }
   ```

2. **Key Mapping** (`SFMLWindow.cpp:25-92`)
   - 51 touches mappees: `sf::Keyboard::Scancode` -> `events::Key`

### CMake Integration

**`src/client/CMakeLists.txt:152-183`**
```cmake
find_package(SFML 3 COMPONENTS Graphics Window System CONFIG QUIET)
if(TARGET SFML::Graphics)
    target_link_libraries(rtype_sfml PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
    )
else()
    # Fallback pkg-config
```

### Gestion des Erreurs

| Erreur | Code | Gestion |
|--------|------|---------|
| Texture non trouvee | `SFMLRenderer.cpp:52-54` | `throw std::runtime_error("Texture not found")` |

---

## 4. SDL2 - Graphics Backend #2

### Utilisation

| Aspect | Details |
|--------|---------|
| Version | 2.x (SDL2, SDL2_image, SDL2_ttf, SDL2_mixer) |
| Fichiers | `lib/sdl2/src/` |
| Alternative | Meme interface que SFML via abstraction |

### Fichiers Cles

1. **SDL2Window** (`src/client/lib/sdl2/src/SDL2Window.cpp:85-100`)
   ```cpp
   SDL2Window::SDL2Window(Vec2u winSize, const std::string& name)
   {
       SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
       SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");  // 1:1 pixels

       if (SDL_Init(SDL_INIT_VIDEO) < 0) {
           throw std::runtime_error("Failed: " + std::string(SDL_GetError()));
       }
   ```

2. **DPI Awareness** (Windows-specific)
   - Desactive scaling pour avoir pixels 1:1

### Gestion des Erreurs

| Erreur | Ligne | Gestion |
|--------|-------|---------|
| Init failed | `SDL2Window.cpp:92-93` | `throw std::runtime_error(SDL_GetError())` |

---

## 5. OPUS - Audio Codec

### Utilisation

| Aspect | Details |
|--------|---------|
| Version | 1.5.2 (via vcpkg) |
| Taux d'echantillonnage | 48000 Hz (optimal Opus) |
| Canaux | 1 (mono) |
| Bitrate | 32000 bps (32 kbps) |
| Frame | 960 samples = 20ms @ 48kHz |
| MaxPacket | 4000 bytes |

### Fichiers Cles

1. **OpusCodec Init** (`src/client/src/audio/OpusCodec.cpp:46-89`)
   ```cpp
   bool OpusCodec::init() {
       int error;
       _encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS,
                                      OPUS_APPLICATION_VOIP, &error);
       if (error != OPUS_OK || !_encoder) {
           logger->error("Failed to create Opus encoder: {}",
                        opus_strerror(error));
           return false;
       }

       opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(BITRATE));
       opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
       opus_encoder_ctl(_encoder, OPUS_SET_VBR(1));  // Variable bitrate
       opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(5));  // Medium
   ```

2. **Encode** (`OpusCodec.cpp:103-125`)
   ```cpp
   std::vector<uint8_t> OpusCodec::encode(const float* pcmData, int frameSize) {
       if (!_initialized || !_encoder) return {};

       std::vector<uint8_t> encoded(MAX_PACKET_SIZE);
       int bytesWritten = opus_encode_float(
           _encoder, pcmData, frameSize,
           encoded.data(), encoded.size()
       );

       if (bytesWritten < 0) {
           logger->error("Opus encode failed: {}",
                        opus_strerror(bytesWritten));
           return {};
       }
   ```

### Gestion des Erreurs

| Type | Code | Gestion |
|------|------|---------|
| Creation encoder | `OpusCodec.cpp:65-67` | `opus_strerror(error)` + logger |
| Creation decoder | `OpusCodec.cpp:77-82` | `opus_strerror()` + destruction encoder |
| Encode erreur | `OpusCodec.cpp:118-120` | `opus_strerror()` + return empty |
| Decode erreur | `OpusCodec.cpp:143-145` | Check `samplesDecoded <= 0` |

---

## 6. PORTAUDIO - Audio I/O Cross-Platform

### Utilisation

| Aspect | Details |
|--------|---------|
| Selection device | Priorite: user -> hardware -> system default |
| Init | `Pa_Initialize()` |
| Device detection | `Pa_GetDeviceCount()`, `Pa_GetDeviceInfo()` |
| Stream | Callback-based |

### Fichiers Cles

1. **Init** (`src/client/src/audio/VoiceChatManager.cpp:39-58`)
   ```cpp
   bool VoiceChatManager::init() {
       PaError err = Pa_Initialize();
       if (err != paNoError) {
           logger->error("PortAudio init failed: {}", Pa_GetErrorText(err));
           return false;
       }

       if (!_codec.init()) {
           logger->error("Opus codec init failed");
           Pa_Terminate();  // Cleanup on failure
           return false;
       }
   ```

2. **Virtual Device Filtering** (`VoiceChatManager.cpp:102-122`)
   ```cpp
   auto isVirtualDevice = [](const std::string& name) -> bool {
       if (name.find("WEBRTC") != std::string::npos) return true;
       if (name.find("Firefox") != std::string::npos) return true;
       if (name.find("Microsoft Sound Mapper") != std::string::npos) return true;
       if (name.find("VB-Audio") != std::string::npos) return true;
       // ... 15+ virtual device patterns
       return false;
   };
   ```

### Gestion des Erreurs

| Type | Code | Gestion |
|------|------|---------|
| Pa_Initialize fail | `VoiceChatManager.cpp:48-50` | `Pa_GetErrorText()` + return false |
| Device not found | `VoiceChatManager.cpp:83-84` | Log warn + fallback |
| Stream open fail | `PortAudioTest.cpp:96-101` | Retry avec canaux reduits |
| Shutdown | `VoiceChatManager.cpp:30-36` | Try-catch destructeur (RAII) |

---

## 7. LZ4 - Network Compression

### Utilisation

| Aspect | Details |
|--------|---------|
| Fonction | Reduction bande UDP GameSnapshot (800-2000B -> 40-60% reduction) |
| Min taille | 128 bytes (overhead considere) |
| Max taille | 65535 bytes |
| Algo | LZ4 fast (par defaut) |

### Fichiers Cles

1. **Compress** (`src/common/compression/Compression.hpp:27-58`)
   ```cpp
   inline std::vector<uint8_t> compress(const uint8_t* src, size_t srcSize) {
       if (srcSize == 0 || src == nullptr) return {};

       int maxDstSize = LZ4_compressBound(static_cast<int>(srcSize));
       if (maxDstSize <= 0) return {};

       std::vector<uint8_t> compressed(maxDstSize);
       int compressedSize = LZ4_compress_default(
           reinterpret_cast<const char*>(src),
           reinterpret_cast<char*>(compressed.data()),
           static_cast<int>(srcSize),
           maxDstSize
       );

       if (compressedSize <= 0) return {};

       // Only use if it reduces size
       if (static_cast<size_t>(compressedSize) >= srcSize) {
           return {};  // Compression not worth it
       }
   ```

### Gestion des Erreurs

| Type | Code | Gestion |
|------|------|---------|
| NULL pointer | `Compression.hpp:28-30` | Return empty vector |
| Invalid compressed | `Compression.hpp:85` | Check `decompressedSize < 0` |
| Size mismatch | `Compression.hpp:85-86` | Return `std::nullopt` |
| Compression not worth | `Compression.hpp:52-54` | Return empty (no compression) |

---

## 8. MONGODB C++ DRIVER

### Utilisation

| Aspect | Details |
|--------|---------|
| Version | 4.0.0 (via vcpkg) |
| Collections | users, user_settings, leaderboards, chat_messages, friendships, friend_requests, blocked_users, private_messages |
| Thread-safety | Connection pool (acquireClient) |

### Gestion des Erreurs

| Operation | Exception | Gestion |
|-----------|-----------|---------|
| Duplicate username | `UsernameAlreadyExistsException` | Application-level handling |
| Duplicate email | `EmailAlreadyExistsException` | Application-level handling |
| Connection fail | (Implicite mongocxx) | Pool management |
| Document casting | `bsoncxx::view` methods | Safe string conversions |

---

## 9. GOOGLE TEST - Testing Framework

### Utilisation

| Aspect | Details |
|--------|---------|
| Version | Latest (vcpkg CONFIG) |
| Serveur Tests | 55+ fichiers source |
| Client Tests | 8 fichiers (audio, config, UI) |
| Common Tests | CompressionTest, VersionSystemTest |

### CMake Integration

**`tests/server/CMakeLists.txt:161-201`**
```cmake
find_package(GTest CONFIG REQUIRED)

target_link_libraries(server_tests PRIVATE
    Boost::system
    GTest::gtest
    GTest::gtest_main
    OpenSSL::SSL
    spdlog::spdlog
    lz4::lz4
)

# Enregistrer avec CTest
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(server_tests)
endif()
```

---

## 10. SPDLOG - Structured Logging

### Utilisation

| Aspect | Details |
|--------|---------|
| Loggers | Network, Audio, Core, Game |
| Format | Structured avec timestamps |
| Multi-sink | File + Console |

### Usage
```cpp
auto logger = server::logging::Logger::getNetworkLogger();
logger->error("Failed to create Opus encoder: {}",
             opus_strerror(error));
logger->info("Session closed - removing user '{}'", username);
logger->debug("TCP session read error: {}", ec.message());
```

---

## 11. OPENSSL - TLS/SSL

### Utilisation

| Aspect | Details |
|--------|---------|
| Libs | SSL, Crypto |
| Serveur TCP | Port 4125 (TCPAuthServer) |
| Stream | `ssl::stream<tcp::socket>` |

### Gestion des Erreurs

**SSL Shutdown** (`TCPAuthServer.cpp:112-117`)
```cpp
boost::system::error_code ec;
_socket.shutdown(ec);
if (ec && ec != boost::asio::error::eof &&
    ec != boost::asio::ssl::error::stream_truncated) {
    logger->debug("SSL shutdown notice: {}", ec.message());
}
```

---

## RESUME DE LA GESTION DES ERREURS PAR LIB

### Pattern General

```cpp
// Pattern 1: Exceptions Domain
try {
    // Operation
} catch (const domain::exceptions::DomainException& e) {
    logger->error("Operation failed: {}", e.what());
}

// Pattern 2: Return codes (C API)
int error;
obj = opus_encoder_create(..., &error);
if (error != OPUS_OK) {
    logger->error("Failed: {}", opus_strerror(error));
    return false;
}

// Pattern 3: Optional/Nullopt
auto result = compression::decompress(...);
if (!result) {
    logger->error("Decompression failed");
}

// Pattern 4: Boost.ASIO error_code
boost::system::error_code ec;
_socket.shutdown(ec);
if (ec && ec != boost::asio::error::eof) {
    logger->debug("Error: {}", ec.message());
}
```

### Statistiques Gestion Erreurs

| Lib | Try-Catch | Return bool/opt | C-Error codes | Exceptions |
|-----|-----------|-----------------|---------------|-----------|
| Boost.ASIO | 10+ | N/A | boost::system::error_code | boost::system::system_error |
| SFML | 1+ | N/A | N/A | std::runtime_error |
| SDL2 | 1+ | N/A | SDL_GetError() | std::runtime_error |
| Opus | 0 | Vector vide | opus_strerror() | N/A |
| PortAudio | 1+ | Vector vide | Pa_GetErrorText() | std::runtime_error |
| LZ4 | 0 | std::optional | return codes | N/A |
| MongoDB | 0 | N/A | N/A | domain::exceptions::* |
| GTest | N/A | ASSERT/EXPECT | N/A | gtest assertions |
| spdlog | 0 | N/A | N/A | N/A |
| OpenSSL | Via Boost | Via Boost.ASIO | N/A | N/A |

---

## CONCLUSION

R-Type utilise ses dependances tierces **correctement et de facon coherente** :

**Points forts**
- Abstraction bien pensee (Graphics plugins)
- Gestion erreurs contextualisee par lib
- Tests a jour pour composants critiques
- CMake support vcpkg + fallback pkg-config

**A ameliorer**
- MongoDB const_cast threading
- Device selection PortAudio hardcode
- MinGW linking flags a auditer

Le codebase est **production-ready** pour les libs principales (Boost, Opus, PortAudio, MongoDB, LZ4).
