# Compétence 11 : Intégrité des Données

> **Vérifier l'intégrité des données traitées en s'appuyant sur les techniques de vérification de données et dans le respect des normes de conformité et de sécurité afin de s'assurer que la donnée n'a pas été corrompue et de prévenir ainsi les dysfonctionnements du logiciel.**

---

## Observable 11.1 : Vérifications pour l'Intégrité et la Stabilité

### Validation des Messages Réseau

#### Pattern from_bytes() avec Validation

Chaque structure du protocole implémente une validation stricte lors du parsing.

**Fichier** : `src/common/protocol/Protocol.hpp`

```cpp
static std::optional<LoginMessage> from_bytes(const void* buf, size_t buf_len) {
    // 1. Vérification nullptr
    if (buf == nullptr) return std::nullopt;

    // 2. Vérification taille minimale
    if (buf_len < MAX_USERNAME_LEN + MAX_PASSWORD_LEN) return std::nullopt;

    LoginMessage msg;
    auto* ptr = static_cast<const uint8_t*>(buf);

    // 3. Copie sécurisée avec limites
    std::memcpy(msg.username, ptr, MAX_USERNAME_LEN);
    std::memcpy(msg.password, ptr + MAX_USERNAME_LEN, MAX_PASSWORD_LEN);

    // 4. Null-termination forcée
    msg.username[MAX_USERNAME_LEN - 1] = '\0';
    msg.password[MAX_PASSWORD_LEN - 1] = '\0';

    return msg;
}
```

#### Constantes de Limites

```cpp
static constexpr size_t MAX_USERNAME_LEN = 32;
static constexpr size_t MAX_PASSWORD_LEN = 64;
static constexpr size_t MAX_EMAIL_LEN = 255;    // RFC 5321
static constexpr size_t MAX_MESSAGE_LEN = 500;
```

### Validation des Headers TCP

**Fichier** : `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp:152-166`

```cpp
while (_accumulator.size() >= Header::WIRE_SIZE) {
    // 1. Parser le header
    auto headOpt = Header::from_bytes(_accumulator.data(), _accumulator.size());
    if (!headOpt) break;  // Header invalide

    Header head = *headOpt;

    // 2. Vérifier que le message complet est disponible
    size_t totalSize = Header::WIRE_SIZE + head.payload_size;
    if (_accumulator.size() < totalSize) break;  // Attendre plus de données

    // 3. Traiter le message complet
    handle_command(head);

    // 4. Retirer les données traitées
    _accumulator.erase(_accumulator.begin(), _accumulator.begin() + totalSize);
}
```

### Validation UDP avec Numéros de Séquence

```cpp
struct UDPHeader {
    uint16_t type;
    uint16_t sequence_num;   // Pour détection replay/réordonnancement
    uint64_t timestamp;      // Pour fraîcheur des données
};
```

**Vérifications possibles** :
- Rejet des paquets trop anciens (timestamp)
- Détection des paquets dupliqués (sequence_num)
- Réordonnancement si nécessaire

### Validation de la Compression

**Fichier** : `src/common/compression/Compression.hpp:67-90`

```cpp
inline std::optional<std::vector<uint8_t>> decompress(
    const uint8_t* src, size_t srcSize, size_t originalSize
) {
    // 1. Vérification des paramètres
    if (srcSize == 0 || src == nullptr || originalSize == 0) {
        return std::nullopt;
    }

    // 2. Limite de taille pour éviter DoS
    if (originalSize > MAX_UNCOMPRESSED_SIZE) {
        return std::nullopt;
    }

    std::vector<uint8_t> decompressed(originalSize);

    // 3. Décompression avec vérification
    int decompressedSize = LZ4_decompress_safe(
        reinterpret_cast<const char*>(src),
        reinterpret_cast<char*>(decompressed.data()),
        static_cast<int>(srcSize),
        static_cast<int>(originalSize)
    );

    // 4. Vérification taille exacte
    if (decompressedSize < 0 ||
        static_cast<size_t>(decompressedSize) != originalSize) {
        return std::nullopt;  // Corruption détectée
    }

    return decompressed;
}
```

### Validation des Entrées Utilisateur

#### Password Validation

**Fichier** : `src/server/domain/value_objects/user/Password.cpp`

```cpp
void Password::validate(const std::string& password) {
    if (password.length() < 6) {
        throw exceptions::user::PasswordException(password);
    }
    // Critères supplémentaires possibles : caractères spéciaux, chiffres, etc.
}
```

#### Email Validation

```cpp
// Vérification format basique (présence @)
bool isValidEmail(const std::string& email) {
    auto at = email.find('@');
    if (at == std::string::npos) return false;
    if (at == 0 || at == email.length() - 1) return false;
    return true;
}
```

### Gestion des Erreurs TLS

**Fichier** : `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp:926-930`

```cpp
sslSocket->async_handshake(
    ssl::stream_base::server,
    [networkLogger, clientEndpoint](boost::system::error_code hsError) {
        if (hsError) {
            networkLogger->warn("TLS handshake failed from {}: {}",
                clientEndpoint.address().to_string(), hsError.message());
            return;  // Pas de session si handshake échoue
        }
        // Session créée uniquement après TLS réussi
    }
);
```

---

## Observable 11.2 : Moyens pour l'Intégrité et la Confidentialité

### Chiffrement TLS 1.2+

#### Configuration Sécurisée

**Fichier** : `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp:819-840`

```cpp
// Protocoles obsolètes désactivés
ssl::context::no_sslv2 |
ssl::context::no_sslv3 |
ssl::context::no_tlsv1 |
ssl::context::no_tlsv1_1

// TLS 1.2 minimum
SSL_CTX_set_min_proto_version(_sslContext.native_handle(), TLS1_2_VERSION);

// Cipher suites avec Forward Secrecy + AEAD
"ECDHE-ECDSA-AES256-GCM-SHA384:"
"ECDHE-RSA-AES256-GCM-SHA384:"
"ECDHE-ECDSA-AES128-GCM-SHA256:"
"ECDHE-RSA-AES128-GCM-SHA256:"
"ECDHE-ECDSA-CHACHA20-POLY1305:"
"ECDHE-RSA-CHACHA20-POLY1305"
```

#### Garanties

| Propriété | Mécanisme |
|-----------|-----------|
| **Confidentialité** | AES-256-GCM / ChaCha20-Poly1305 |
| **Intégrité** | AEAD (Authenticated Encryption) |
| **Forward Secrecy** | ECDHE (clés éphémères) |
| **Authentification serveur** | Certificats X.509 |

### Hachage des Mots de Passe

**Fichier** : `src/server/domain/value_objects/user/PasswordUtils.cpp`

```cpp
std::string hashPassword(std::string password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
           password.length(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();  // 64 caractères hex
}
```

**Garanties** :
- Mot de passe **jamais stocké en clair**
- Hash SHA-256 (256 bits) irréversible
- Stockage uniquement du hash en base MongoDB

### Génération de Tokens Sécurisés

**Vulnérabilité corrigée** : CWE-338 (Weak PRNG)

```cpp
// AVANT (prédictible)
std::mt19937_64 rng;  // Mersenne Twister

// APRÈS (CSPRNG)
SessionToken generateToken() {
    unsigned char buffer[32];
    RAND_bytes(buffer, 32);  // OpenSSL CSPRNG (FIPS 140-2)
    // Conversion hex -> 64 caractères
}
```

**Propriétés des tokens** :
- 256 bits d'entropie
- Cryptographiquement imprévisibles
- Uniques (collision négligeable)

### Heartbeat et Timeout Anti-DoS

**Fichier** : `src/server/infrastructure/adapters/in/network/TCPAuthServer.cpp:22-23`

```cpp
static constexpr int CLIENT_TIMEOUT_MS = 5000;      // 5 secondes
static constexpr int TIMEOUT_CHECK_INTERVAL_MS = 1000;
```

**Implémentation** :
```cpp
void Session::scheduleTimeoutCheck() {
    _timeoutTimer.expires_after(std::chrono::milliseconds(TIMEOUT_CHECK_INTERVAL_MS));
    _timeoutTimer.async_wait([this](boost::system::error_code ec) {
        if (ec) return;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - _lastActivity
        ).count();

        if (elapsed > CLIENT_TIMEOUT_MS) {
            // Fermeture session inactive
            closeSession();
            return;
        }

        scheduleTimeoutCheck();  // Reschedule
    });
}
```

**Protection** : Empêche les connexions fantômes et limite l'épuisement des ressources.

### Byte Order Réseau (Endianness)

**Fichier** : `src/common/protocol/Protocol.hpp:17-19`

```cpp
inline uint64_t swap64(uint64_t v) { return __builtin_bswap64(v); }
inline uint32_t swap32(uint32_t v) { return __builtin_bswap32(v); }
inline uint16_t swap16(uint16_t v) { return __builtin_bswap16(v); }
```

**Usage** :
```cpp
void to_bytes(void* buf) const {
    uint16_t net_x = swap16(x);  // Host -> Network (big-endian)
    std::memcpy(buf, &net_x, 2);
}

static auto from_bytes(const void* buf, size_t len) {
    uint16_t net_x;
    std::memcpy(&net_x, buf, 2);
    x = swap16(net_x);  // Network -> Host
}
```

**Garantie** : Interopérabilité entre machines little-endian et big-endian.

### Tableau Récapitulatif des Protections

| Donnée | Menace | Protection | Fichier |
|--------|--------|------------|---------|
| Credentials transit | Interception | TLS 1.2+ | `TCPAuthServer.cpp` |
| Passwords stockés | Vol base | SHA-256 hash | `PasswordUtils.cpp` |
| Session tokens | Prédiction | CSPRNG 256-bit | `SessionManager.hpp` |
| Messages UDP | Rejeu | Sequence numbers | `Protocol.hpp` |
| Données compressées | Corruption | Vérification taille | `Compression.hpp` |
| Connexions | DoS | Timeout 5s | `TCPAuthServer.cpp` |
| Entrées utilisateur | Injection | Validation stricte | `from_bytes()` |

### Conformité Standards

| Standard | Exigence | Implémenté |
|----------|----------|------------|
| **OWASP A02** | Cryptographic Failures | TLS 1.2+, RAND_bytes |
| **OWASP A03** | Injection | Validation binaire |
| **OWASP A07** | Auth Failures | SHA-256, sessions |
| **CWE-319** | Cleartext Transmission | TLS obligatoire |
| **CWE-338** | Weak PRNG | OpenSSL CSPRNG |
| **NIST 800-52** | TLS Guidelines | TLS 1.2+, ECDHE, AEAD |

### Conclusion

R-Type garantit l'intégrité et la confidentialité des données via :

1. **Validation stricte** : `from_bytes()` avec vérification taille et null-termination
2. **Chiffrement transport** : TLS 1.2+ avec Forward Secrecy
3. **Protection stockage** : Hachage SHA-256 des mots de passe
4. **Tokens sécurisés** : CSPRNG OpenSSL (256 bits)
5. **Protection réseau** : Sequence numbers, timestamps, timeouts

Ces mécanismes assurent la stabilité du logiciel et la protection des données utilisateur.
