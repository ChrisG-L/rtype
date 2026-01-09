# Architecture d'Authentification et Sessions

## Problème Original

1. **Le serveur fait confiance au client** - Un client moddé peut téléporter son vaisseau
2. **Pas d'authentification UDP** - N'importe qui peut jouer sans login
3. **Un user peut avoir plusieurs sessions** - Pas de contrôle d'unicité
4. **TCP et UDP sont déconnectés** - Le login TCP ne protège pas l'accès UDP

## Solution : Authentification par Token

### Vue d'ensemble

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              FLUX D'AUTHENTIFICATION                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   CLIENT                           SERVEUR                                   │
│                                                                              │
│   ┌─────────┐    1. Login(email, pass)    ┌─────────────┐                   │
│   │   TCP   │ ──────────────────────────► │ TCPAuthServer│                   │
│   │ Client  │                             │             │                    │
│   │         │ ◄────────────────────────── │  Vérifie    │                    │
│   └─────────┘   2. LoginAck + TOKEN       │  credentials│                    │
│       │              (32 bytes)           └──────┬──────┘                    │
│       │                                          │                           │
│       │                                          ▼                           │
│       │                              ┌───────────────────┐                   │
│       │                              │  SessionManager   │                   │
│       │                              │                   │                   │
│       │                              │ - sessions[]      │                   │
│       │                              │ - par email       │                   │
│       │                              └─────────┬─────────┘                   │
│       │                                        │                             │
│       │                                        ▼                             │
│       │                              ┌───────────────────┐                   │
│       │                              │     MongoDB       │                   │
│       │                              │   (sessions)      │                   │
│       │                              └───────────────────┘                   │
│       │                                                                      │
│       ▼                                                                      │
│   ┌─────────┐   3. JoinGame + TOKEN   ┌─────────────┐                       │
│   │   UDP   │ ──────────────────────► │  UDPServer  │                       │
│   │ Client  │                         │             │                        │
│   │         │ ◄────────────────────── │ Valide token│                        │
│   └─────────┘   4. JoinAck + PlayerID │ via Manager │                        │
│       │                               └─────────────┘                        │
│       │                                                                      │
│       ▼                                                                      │
│   ┌─────────┐   5. Inputs (touches)   ┌─────────────┐                       │
│   │  Game   │ ──────────────────────► │  GameWorld  │                       │
│   │  Loop   │                         │             │                        │
│   │         │ ◄────────────────────── │ Simule mvt  │                        │
│   └─────────┘   6. Snapshot (états)   │ Authoritative│                       │
│                                       └─────────────┘                        │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Principes Clés

### 1. Email comme identifiant unique de session

- **Immutable** - L'email ne change pas
- **Unique garanti** - Validé à l'inscription
- **Clé primaire des sessions** - Un email = une session max

### 2. Token de session

- Généré après login TCP réussi
- 32 bytes (256 bits) crypto-random
- Requis pour rejoindre la partie UDP
- Expire après 5 minutes si non utilisé

### 3. Serveur autoritaire

- Le client envoie ses **inputs** (touches), pas sa position
- Le serveur **simule** le mouvement avec validation
- Le **Snapshot** est la source de vérité

---

## Schema MongoDB

### Collection `sessions`

```javascript
{
  _id: ObjectId,
  email: "user@example.com",        // Clé unique - ref users
  token: "abc123...",               // 64 hex chars (32 bytes)
  displayName: "PlayerOne",         // Pour affichage
  createdAt: ISODate,
  expiresAt: ISODate,               // TTL index
  udpEndpoint: "192.168.1.10:54321" | null,
  playerId: 0-3 | null,
  status: "pending" | "active" | "expired"
}

// Index
{ email: 1 }        // unique
{ token: 1 }        // unique
{ udpEndpoint: 1 }  // sparse
{ expiresAt: 1 }    // TTL, expireAfterSeconds: 0
```

### Collection `users` (enrichie)

```javascript
{
  _id: ObjectId,
  username: "player1",
  email: "user@example.com",        // unique
  passwordHash: "...",
  displayName: "Player One",        // défaut = username
  lastLogin: ISODate,
  createdAt: ISODate,
  stats: {
    gamesPlayed: 0,
    wins: 0,
    kills: 0,
    deaths: 0,
    totalPlayTime: 0
  }
}
```

---

## Protocole Réseau

### Nouveaux Messages

| Type | Value | Direction | Payload | Description |
|------|-------|-----------|---------|-------------|
| `JoinGame` | 0x0010 | C→S (UDP) | token (32B) | Authentifie la session UDP |
| `JoinGameAck` | 0x0011 | S→C (UDP) | player_id (1B) | Confirmation |
| `JoinGameNack` | 0x0012 | S→C (UDP) | reason (64B) | Refus |
| `PlayerInput` | 0x0061 | C→S (UDP) | keys (2B) | Inputs clavier |

### Structures

```cpp
constexpr size_t TOKEN_SIZE = 32;

struct SessionToken {
    uint8_t bytes[TOKEN_SIZE];
    static SessionToken generate();  // Crypto-random
};

struct JoinGame {
    static constexpr size_t WIRE_SIZE = TOKEN_SIZE;
    SessionToken token;
};

struct JoinGameAck {
    static constexpr size_t WIRE_SIZE = 1;
    uint8_t player_id;
};

struct PlayerInput {
    static constexpr size_t WIRE_SIZE = 2;
    uint16_t keys;  // Bitfield: UP|DOWN|LEFT|RIGHT|SHOOT
};

namespace InputKeys {
    constexpr uint16_t UP    = 0x0001;
    constexpr uint16_t DOWN  = 0x0002;
    constexpr uint16_t LEFT  = 0x0004;
    constexpr uint16_t RIGHT = 0x0008;
    constexpr uint16_t SHOOT = 0x0010;
}
```

### AuthResponse enrichi (TCP)

```cpp
struct AuthResponseWithToken {
    static constexpr size_t WIRE_SIZE = 1 + 64 + TOKEN_SIZE;
    uint8_t success;
    char message[64];
    uint8_t token[TOKEN_SIZE];  // Vide si success=0
};
```

---

## Composants

### ISessionRepository (Port)

```cpp
struct SessionData {
    std::string email;
    std::string token;
    std::string displayName;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
    std::optional<std::string> udpEndpoint;
    std::optional<uint8_t> playerId;
    std::string status;  // "pending", "active", "expired"
};

class ISessionRepository {
    virtual void create(const SessionData& session) = 0;
    virtual std::optional<SessionData> findByToken(const std::string& token) = 0;
    virtual std::optional<SessionData> findByEmail(const std::string& email) = 0;
    virtual std::optional<SessionData> findByEndpoint(const std::string& endpoint) = 0;
    virtual void updateUdpBinding(const std::string& token,
                                   const std::string& endpoint,
                                   uint8_t playerId) = 0;
    virtual void updateStatus(const std::string& token, const std::string& status) = 0;
    virtual void deleteByToken(const std::string& token) = 0;
    virtual void deleteByEmail(const std::string& email) = 0;
    virtual bool hasActiveSession(const std::string& email) = 0;
};
```

### SessionManager

```cpp
class SessionManager {
    // Après login TCP réussi
    std::optional<CreateSessionResult> createSession(const std::string& email);

    // Validation UDP
    std::optional<ValidateResult> validateAndBindUDP(const SessionToken& token,
                                                      const std::string& endpoint);

    // Lookup par endpoint
    std::optional<SessionData> getByEndpoint(const std::string& endpoint);

    // Assign playerId
    void assignPlayerId(const std::string& endpoint, uint8_t playerId);

    // Cleanup
    void endSession(const std::string& email);
    std::vector<uint8_t> cleanupExpiredSessions();
};
```

### GameWorld (modifié)

```cpp
class GameWorld {
    // SUPPRIMÉ: void movePlayer(uint8_t id, uint16_t x, uint16_t y);

    // NOUVEAU: Applique les inputs
    void applyPlayerInput(uint8_t playerId, uint16_t keys);

    // NOUVEAU: Simulation serveur
    void update(float deltaTime);

private:
    std::unordered_map<uint8_t, uint16_t> _playerInputs;
};
```

---

## Séquence Complète

```
1. Client → TCP: Login(email, password)
2. Serveur → Vérifie credentials via UserRepository
3. Serveur → Vérifie hasActiveSession(email) == false
4. Serveur → createSession(email) → génère token, insère en BDD
5. Serveur → TCP: LoginAck(success=true, token, displayName)

6. Client → UDP: JoinGame(token)
7. Serveur → validateAndBindUDP(token, endpoint)
8. Serveur → GameWorld.addPlayer() → obtient playerId
9. Serveur → assignPlayerId(endpoint, playerId)
10. Serveur → UDP: JoinAck(playerId)

11. Client → UDP: PlayerInput(keys) en boucle
12. Serveur → applyPlayerInput(playerId, keys)
13. Serveur → update(deltaTime) toutes les 50ms
14. Serveur → UDP: Snapshot (positions calculées serveur)

15. Déconnexion → endSession(email) → supprime de BDD
```

---

## Sécurité

| Menace | Protection |
|--------|------------|
| Client moddé téléporte | Serveur calcule les positions |
| Accès UDP sans login | Token requis pour JoinGame |
| Multi-session même user | hasActiveSession() vérifie unicité |
| Token volé | Expiration 5min, lié à endpoint |
| Replay attack | Timestamp + sequence dans UDPHeader |

---

## Évolutions Futures

1. **Stats persistées** - Kills, wins, temps de jeu
2. **Historique des parties** - Collection game_history
3. **Leaderboard** - Classement basé sur stats
4. **Anti-cheat avancé** - Détection de patterns anormaux
5. **Multi-instance** - Plusieurs serveurs partagent la BDD
