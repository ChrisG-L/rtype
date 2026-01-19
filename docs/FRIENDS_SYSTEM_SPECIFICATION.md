# Spécification Complète : Système d'Amis R-Type

> **Version** : 1.1
> **Date** : 2026-01-19
> **Auteur** : Claude Code Analysis
> **Statut** : Protocole ✅ | Backend ✅ | Frontend ✅ | Tests ❌ À ajouter

---

## Table des Matières

1. [Résumé Exécutif](#1-résumé-exécutif)
2. [Fonctionnalités Complètes](#2-fonctionnalités-complètes)
3. [Protocole Réseau (Déjà Défini)](#3-protocole-réseau-déjà-défini)
4. [Architecture Backend](#4-architecture-backend)
5. [Collections MongoDB](#5-collections-mongodb)
6. [Interfaces Repository](#6-interfaces-repository)
7. [Handlers TCPAuthServer](#7-handlers-tcpauthserver)
8. [Synchronisation Temps Réel](#8-synchronisation-temps-réel)
9. [Architecture Client](#9-architecture-client)
10. [Scènes UI](#10-scènes-ui)
11. [Flux de Données Complets](#11-flux-de-données-complets)
12. [Sécurité et Validation](#12-sécurité-et-validation)
13. [Tests et Validation](#13-tests-et-validation)
14. [Plan d'Implémentation](#14-plan-dimplémentation)
15. [Fichiers à Créer/Modifier](#15-fichiers-à-créermodifier)

---

## 1. Résumé Exécutif

### État Actuel

| Composant | État | Détails |
|-----------|------|---------|
| **Protocol.hpp** | ✅ 100% | 33 MessageTypes (0x0600-0x0698), 15 structures wire |
| **Enums** | ✅ 100% | FriendOnlineStatus, FriendshipStatus, FriendErrorCode |
| **SessionManager** | ✅ 100% | Infrastructure callbacks, lookup par email |
| **TCPAuthServer** | ✅ 100% | Tous les handlers amis implémentés (~1300 lignes) |
| **MongoDB Repos** | ✅ 100% | 4 repositories créés (IFriendship, IFriendRequest, IBlockedUser, IPrivateMessage) |
| **FriendManager** | ✅ 100% | Gestionnaire temps réel avec callbacks thread-safe |
| **TCPClient** | ✅ 100% | 13 nouvelles méthodes ajoutées |
| **NetworkEvents** | ✅ 100% | 11 nouveaux événements ajoutés |
| **FriendsScene** | ✅ 100% | Scène UI complète (853 lignes) |
| **PrivateChatScene** | ✅ 100% | Scène messages privés (487 lignes) |
| **Tests** | ❌ 0% | Tests unitaires à ajouter |

### Estimation d'Effort

| Phase | Fichiers | Lignes de Code | Complexité |
|-------|----------|----------------|------------|
| Repositories | 8 | ~1200 | Moyenne |
| TCPAuthServer Handlers | 1 | ~800 | Élevée |
| SessionManager Callbacks | 1 | ~150 | Moyenne |
| TCPClient | 2 | ~400 | Faible |
| NetworkEvents | 1 | ~150 | Faible |
| FriendsScene | 2 | ~1500 | Élevée |
| PrivateChatScene | 2 | ~800 | Moyenne |
| Tests | 5 | ~600 | Moyenne |
| **Total** | ~22 | ~5600 | - |

---

## 2. Fonctionnalités Complètes

### 2.1 Gestion des Amis

| Fonctionnalité | Description | Priorité |
|----------------|-------------|----------|
| **Envoi de demande** | Par email exacte | P0 |
| **Réception demande** | Notification temps réel | P0 |
| **Accepter demande** | Ajoute aux deux listes | P0 |
| **Refuser demande** | Supprime la demande | P0 |
| **Supprimer ami** | Retire des deux listes | P0 |
| **Bloquer utilisateur** | Empêche toute interaction | P1 |
| **Débloquer utilisateur** | Restaure possibilité d'interaction | P1 |
| **Liste d'amis** | Avec statut temps réel | P0 |
| **Liste demandes** | Envoyées et reçues | P0 |
| **Liste bloqués** | Pour gestion | P1 |

### 2.2 Statuts en Temps Réel

```
┌─────────────────────────────────────────────────────────────┐
│                    FRIEND ONLINE STATUS                      │
├─────────────────────────────────────────────────────────────┤
│  ○ Offline   - Déconnecté                                   │
│  ● Online    - Connecté (menu principal)                    │
│  🎮 InGame   - En partie (roomCode visible)                 │
│  🚪 InLobby  - Dans un salon (roomCode visible)             │
└─────────────────────────────────────────────────────────────┘
```

**Transitions de Statut** :
```
Login           → Offline → Online
Join Room       → Online → InLobby
Start Game      → InLobby → InGame
Leave Room      → InLobby/InGame → Online
Game Over       → InGame → InLobby
Logout/Timeout  → * → Offline
```

### 2.3 Messages Privés

| Fonctionnalité | Description | Priorité |
|----------------|-------------|----------|
| **Envoyer message** | À un ami uniquement | P0 |
| **Recevoir message** | Notification temps réel | P0 |
| **Historique conversation** | Paginé (50 msg/page) | P0 |
| **Liste conversations** | Triées par date | P0 |
| **Marquer comme lu** | Par conversation | P1 |
| **Compteur non-lus** | Badge global + par conv | P0 |
| **Messages offline** | Stockés pour livraison | P1 |

### 2.4 Fonctionnalités Sociales

| Fonctionnalité | Description | Priorité |
|----------------|-------------|----------|
| **Rejoindre ami** | Si en lobby (bouton JOIN) | P1 |
| **Inviter ami** | Envoyer code room | P2 |
| **Voir profil ami** | Stats depuis leaderboard | P2 |
| **Recherche utilisateur** | Par email partielle | P2 |

---

## 3. Protocole Réseau (Déjà Défini)

### 3.1 Message Types (Protocol.hpp:124-157)

```cpp
enum class MessageType : uint16_t {
    // ═══════════════════════════════════════════════════════════
    // FRIENDS SYSTEM (0x060x - 0x068x)
    // ═══════════════════════════════════════════════════════════

    // Friend Requests
    SendFriendRequest       = 0x0600,  // C→S : Envoyer demande
    SendFriendRequestAck    = 0x0601,  // S→C : Confirmation/erreur
    FriendRequestReceived   = 0x0602,  // S→C : Notification (push)

    AcceptFriendRequest     = 0x0610,  // C→S : Accepter demande
    AcceptFriendRequestAck  = 0x0611,  // S→C : Confirmation
    FriendRequestAccepted   = 0x0612,  // S→C : Notif au demandeur (push)

    RejectFriendRequest     = 0x0620,  // C→S : Refuser demande
    RejectFriendRequestAck  = 0x0621,  // S→C : Confirmation

    // Friend Management
    RemoveFriend            = 0x0630,  // C→S : Supprimer ami
    RemoveFriendAck         = 0x0631,  // S→C : Confirmation
    FriendRemoved           = 0x0632,  // S→C : Notif à l'autre (push)

    // Block System
    BlockUser               = 0x0640,  // C→S : Bloquer
    BlockUserAck            = 0x0641,  // S→C : Confirmation
    UnblockUser             = 0x0650,  // C→S : Débloquer
    UnblockUserAck          = 0x0651,  // S→C : Confirmation

    // Lists
    GetFriendsList          = 0x0660,  // C→S : Demander liste amis
    FriendsListData         = 0x0661,  // S→C : Liste amis
    GetFriendRequests       = 0x0670,  // C→S : Demander demandes
    FriendRequestsData      = 0x0671,  // S→C : Liste demandes
    GetBlockedUsers         = 0x0672,  // C→S : Demander bloqués
    BlockedUsersData        = 0x0673,  // S→C : Liste bloqués

    // Real-time Status
    FriendStatusChanged     = 0x0680,  // S→C : Statut changé (push)

    // ═══════════════════════════════════════════════════════════
    // PRIVATE MESSAGING (0x069x)
    // ═══════════════════════════════════════════════════════════

    SendPrivateMessage      = 0x0690,  // C→S : Envoyer message
    SendPrivateMessageAck   = 0x0691,  // S→C : Confirmation + ID
    PrivateMessageReceived  = 0x0692,  // S→C : Message reçu (push)
    GetConversation         = 0x0693,  // C→S : Historique
    ConversationData        = 0x0694,  // S→C : Messages
    GetConversationsList    = 0x0695,  // C→S : Liste conversations
    ConversationsListData   = 0x0696,  // S→C : Résumés conversations
    MarkMessagesRead        = 0x0697,  // C→S : Marquer lu
    MarkMessagesReadAck     = 0x0698,  // S→C : Confirmation
};
```

### 3.2 Constantes (Protocol.hpp:3015-3017)

```cpp
static constexpr size_t MAX_EMAIL_LEN = 255;        // RFC 5321
static constexpr size_t MAX_USERNAME_LEN = 32;      // Display name
static constexpr size_t MAX_MESSAGE_LEN = 512;      // Private message
static constexpr size_t MAX_FRIENDS = 100;          // Limite amis
static constexpr size_t MAX_CONVERSATIONS = 50;     // Liste conversations
static constexpr size_t MAX_MESSAGES_PER_PAGE = 50; // Pagination
static constexpr size_t ROOM_CODE_LEN = 6;          // Code salon
```

### 3.3 Enums (Protocol.hpp:3020-3049)

```cpp
// Statut de connexion d'un ami
enum class FriendOnlineStatus : uint8_t {
    Offline = 0,  // Déconnecté
    Online  = 1,  // Connecté (menu)
    InGame  = 2,  // En partie
    InLobby = 3   // Dans salon
};

// Statut d'une relation
enum class FriendshipStatus : uint8_t {
    Pending  = 0,  // Demande en attente
    Accepted = 1,  // Amis confirmés
    Blocked  = 2   // Bloqué
};

// Codes d'erreur
enum class FriendErrorCode : uint8_t {
    Success              = 0,   // OK
    UserNotFound         = 1,   // Email n'existe pas
    AlreadyFriends       = 2,   // Déjà amis
    RequestAlreadySent   = 3,   // Demande déjà envoyée
    RequestAlreadyReceived = 4, // Demande reçue de cette personne
    IsBlocked            = 5,   // Vous avez bloqué cette personne
    BlockedByUser        = 6,   // Cette personne vous a bloqué
    CannotAddSelf        = 7,   // Auto-ajout interdit
    MaxFriendsReached    = 8,   // Limite 100 atteinte
    MaxRequestsReached   = 9,   // Trop de demandes en attente
    NotFriends           = 10,  // Pas amis (pour suppression)
    RequestNotFound      = 11,  // Demande non trouvée
    InternalError        = 99   // Erreur serveur
};
```

### 3.4 Structures Wire (Protocol.hpp:3052-3501)

#### FriendInfoWire (298 bytes)
```cpp
struct FriendInfoWire {
    char email[MAX_EMAIL_LEN];           // 255B - Email de l'ami
    char displayName[MAX_USERNAME_LEN];  // 32B  - Nom affiché
    uint8_t onlineStatus;                // 1B   - FriendOnlineStatus
    uint32_t lastSeen;                   // 4B   - Unix timestamp (network order)
    char roomCode[ROOM_CODE_LEN];        // 6B   - Si InGame/InLobby

    static constexpr size_t WIRE_SIZE = 298;

    void to_bytes(uint8_t* buf) const;
    static std::optional<FriendInfoWire> from_bytes(const void* data, size_t len);
};
```

#### FriendRequestInfoWire (292 bytes)
```cpp
struct FriendRequestInfoWire {
    char email[MAX_EMAIL_LEN];           // 255B - Email demandeur/cible
    char displayName[MAX_USERNAME_LEN];  // 32B  - Nom affiché
    uint32_t timestamp;                  // 4B   - Date demande (network order)
    uint8_t isIncoming;                  // 1B   - 1=reçue, 0=envoyée

    static constexpr size_t WIRE_SIZE = 292;
};
```

#### SendFriendRequestPayload (255 bytes)
```cpp
struct SendFriendRequestPayload {
    char targetEmail[MAX_EMAIL_LEN];     // 255B - Email cible

    static constexpr size_t WIRE_SIZE = 255;
};
```

#### FriendRequestAckPayload (256 bytes)
```cpp
struct FriendRequestAckPayload {
    uint8_t errorCode;                   // 1B   - FriendErrorCode
    char targetEmail[MAX_EMAIL_LEN];     // 255B - Email concerné

    static constexpr size_t WIRE_SIZE = 256;
};
```

#### FriendRequestReceivedPayload (287 bytes)
```cpp
struct FriendRequestReceivedPayload {
    char fromEmail[MAX_EMAIL_LEN];       // 255B - Qui a envoyé
    char fromDisplayName[MAX_USERNAME_LEN]; // 32B - Son nom

    static constexpr size_t WIRE_SIZE = 287;
};
```

#### RespondFriendRequestPayload (255 bytes)
```cpp
struct RespondFriendRequestPayload {
    char fromEmail[MAX_EMAIL_LEN];       // 255B - De qui vient la demande

    static constexpr size_t WIRE_SIZE = 255;
};
```

#### FriendRequestAcceptedPayload (288 bytes)
```cpp
struct FriendRequestAcceptedPayload {
    char friendEmail[MAX_EMAIL_LEN];     // 255B - Nouvel ami
    char friendDisplayName[MAX_USERNAME_LEN]; // 32B - Son nom
    uint8_t onlineStatus;                // 1B   - Statut actuel

    static constexpr size_t WIRE_SIZE = 288;
};
```

#### RemoveFriendPayload (255 bytes)
```cpp
struct RemoveFriendPayload {
    char friendEmail[MAX_EMAIL_LEN];     // 255B
    static constexpr size_t WIRE_SIZE = 255;
};
```

#### BlockUserPayload (255 bytes)
```cpp
struct BlockUserPayload {
    char targetEmail[MAX_EMAIL_LEN];     // 255B
    static constexpr size_t WIRE_SIZE = 255;
};
```

#### GetFriendsListPayload (2 bytes)
```cpp
struct GetFriendsListPayload {
    uint8_t offset;                      // 1B - Pagination offset
    uint8_t limit;                       // 1B - Max 50

    static constexpr size_t WIRE_SIZE = 2;
};
```

#### FriendStatusChangedPayload (262 bytes)
```cpp
struct FriendStatusChangedPayload {
    char friendEmail[MAX_EMAIL_LEN];     // 255B
    uint8_t newStatus;                   // 1B  - FriendOnlineStatus
    char roomCode[ROOM_CODE_LEN];        // 6B  - Si InGame/InLobby

    static constexpr size_t WIRE_SIZE = 262;
};
```

#### PrivateMessageWire (808 bytes)
```cpp
struct PrivateMessageWire {
    char senderEmail[MAX_EMAIL_LEN];     // 255B
    char senderDisplayName[MAX_USERNAME_LEN]; // 32B
    char message[MAX_MESSAGE_LEN];       // 512B
    uint64_t timestamp;                  // 8B  - Unix timestamp (network order)
    uint8_t isRead;                      // 1B  - 0/1

    static constexpr size_t WIRE_SIZE = 808;
};
```

#### SendPrivateMessagePayload (767 bytes)
```cpp
struct SendPrivateMessagePayload {
    char recipientEmail[MAX_EMAIL_LEN];  // 255B
    char message[MAX_MESSAGE_LEN];       // 512B

    static constexpr size_t WIRE_SIZE = 767;
};
```

#### SendPrivateMessageAckPayload (9 bytes)
```cpp
struct SendPrivateMessageAckPayload {
    uint8_t errorCode;                   // 1B - FriendErrorCode
    uint64_t messageId;                  // 8B - ID unique (network order)

    static constexpr size_t WIRE_SIZE = 9;
};
```

#### GetConversationPayload (257 bytes)
```cpp
struct GetConversationPayload {
    char otherEmail[MAX_EMAIL_LEN];      // 255B
    uint8_t offset;                      // 1B
    uint8_t limit;                       // 1B (max 50)

    static constexpr size_t WIRE_SIZE = 257;
};
```

#### ConversationSummaryWire (809 bytes)
```cpp
struct ConversationSummaryWire {
    char otherEmail[MAX_EMAIL_LEN];      // 255B
    char otherDisplayName[MAX_USERNAME_LEN]; // 32B
    char lastMessage[MAX_MESSAGE_LEN];   // 512B - Preview
    uint64_t lastTimestamp;              // 8B
    uint8_t unreadCount;                 // 1B
    uint8_t onlineStatus;                // 1B - FriendOnlineStatus

    static constexpr size_t WIRE_SIZE = 809;
};
```

#### MarkMessagesReadPayload (255 bytes)
```cpp
struct MarkMessagesReadPayload {
    char otherEmail[MAX_EMAIL_LEN];      // 255B

    static constexpr size_t WIRE_SIZE = 255;
};
```

---

## 4. Architecture Backend

### 4.1 Architecture Hexagonale

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              DOMAIN LAYER                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │ Friendship  │  │FriendRequest│  │  BlockedUser│  │PrivateMessage│   │
│  │   Entity    │  │   Entity    │  │   Entity    │  │   Entity    │    │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           APPLICATION LAYER                              │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                         USE CASES                                │   │
│  │  SendFriendRequest │ AcceptFriend │ SendPrivateMessage │ etc.   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      PORTS (Interfaces)                         │   │
│  │  IFriendshipRepository │ IFriendRequestRepository │ etc.        │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         INFRASTRUCTURE LAYER                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────┐  │
│  │   TCPAuthServer  │  │  SessionManager  │  │  MongoDB Repositories │  │
│  │   (Adapters/In)  │  │  (Callbacks)     │  │    (Adapters/Out)     │  │
│  └──────────────────┘  └──────────────────┘  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Flux de Données

```
Client                      TCPAuthServer                    MongoDB
  │                              │                              │
  │─ SendFriendRequest ─────────>│                              │
  │                              │─ validateRequest() ─────────>│
  │                              │<── UserData ─────────────────│
  │                              │─ checkBlocked() ────────────>│
  │                              │<── BlockedStatus ────────────│
  │                              │─ checkExistingFriend() ─────>│
  │                              │<── FriendshipStatus ─────────│
  │                              │─ createFriendRequest() ─────>│
  │                              │<── RequestCreated ───────────│
  │<─ SendFriendRequestAck ─────│                              │
  │                              │                              │
  │                              │─ notifyTarget() ────────────>│ (if online)
  │                              │   FriendRequestReceived      │
```

---

## 5. Collections MongoDB

### 5.1 Collection `friendships`

```javascript
// Schéma
{
  "_id": ObjectId,
  "user1_email": String,        // Toujours alphabétiquement premier
  "user2_email": String,        // Toujours alphabétiquement second
  "created_at": ISODate
}

// Index
{ "user1_email": 1, "user2_email": 1 }  // Unique
{ "user2_email": 1 }                    // Pour recherche inverse

// Exemple
{
  "_id": ObjectId("..."),
  "user1_email": "alice@example.com",
  "user2_email": "bob@example.com",
  "created_at": ISODate("2026-01-19T10:00:00Z")
}
```

**Note** : On stocke toujours `user1 < user2` alphabétiquement pour éviter les doublons.

### 5.2 Collection `friend_requests`

```javascript
// Schéma
{
  "_id": ObjectId,
  "from_email": String,
  "to_email": String,
  "from_display_name": String,
  "created_at": ISODate
}

// Index
{ "from_email": 1, "to_email": 1 }  // Unique
{ "to_email": 1 }                   // Pour liste demandes reçues
{ "from_email": 1 }                 // Pour liste demandes envoyées

// Exemple
{
  "_id": ObjectId("..."),
  "from_email": "alice@example.com",
  "to_email": "bob@example.com",
  "from_display_name": "Alice",
  "created_at": ISODate("2026-01-19T10:00:00Z")
}
```

### 5.3 Collection `blocked_users`

```javascript
// Schéma
{
  "_id": ObjectId,
  "blocker_email": String,      // Qui bloque
  "blocked_email": String,      // Qui est bloqué
  "blocked_display_name": String,
  "created_at": ISODate
}

// Index
{ "blocker_email": 1, "blocked_email": 1 }  // Unique
{ "blocked_email": 1 }                      // Pour vérifier si bloqué par

// Exemple
{
  "_id": ObjectId("..."),
  "blocker_email": "alice@example.com",
  "blocked_email": "troll@example.com",
  "blocked_display_name": "TrollUser",
  "created_at": ISODate("2026-01-19T10:00:00Z")
}
```

### 5.4 Collection `private_messages`

```javascript
// Schéma
{
  "_id": ObjectId,
  "conversation_key": String,   // "email1:email2" (alphabétique)
  "sender_email": String,
  "recipient_email": String,
  "sender_display_name": String,
  "message": String,
  "timestamp": ISODate,
  "is_read": Boolean
}

// Index
{ "conversation_key": 1, "timestamp": -1 }  // Pour historique
{ "recipient_email": 1, "is_read": 1 }      // Pour compteur non-lus
{ "timestamp": -1 }                         // Pour cleanup vieux messages

// Exemple
{
  "_id": ObjectId("507f1f77bcf86cd799439011"),
  "conversation_key": "alice@example.com:bob@example.com",
  "sender_email": "alice@example.com",
  "recipient_email": "bob@example.com",
  "sender_display_name": "Alice",
  "message": "Salut ! Une partie ?",
  "timestamp": ISODate("2026-01-19T10:05:00Z"),
  "is_read": false
}
```

### 5.5 Helper: Conversation Key

```cpp
// Génère une clé unique pour une conversation (ordre alphabétique)
std::string makeConversationKey(const std::string& email1, const std::string& email2) {
    if (email1 < email2) {
        return email1 + ":" + email2;
    }
    return email2 + ":" + email1;
}
```

---

## 6. Interfaces Repository

### 6.1 IFriendshipRepository

**Fichier** : `src/server/include/application/ports/out/persistence/IFriendshipRepository.hpp`

```cpp
#ifndef IFRIENDSHIPREPOSITORY_HPP_
#define IFRIENDSHIPREPOSITORY_HPP_

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace application::ports::out::persistence {

struct FriendshipData {
    std::string friendEmail;
    std::string friendDisplayName;
    std::chrono::system_clock::time_point createdAt;
};

class IFriendshipRepository {
public:
    virtual ~IFriendshipRepository() = default;

    /**
     * Ajoute une relation d'amitié (bidirectionnelle)
     * @param email1 Premier utilisateur
     * @param email2 Second utilisateur
     */
    virtual void addFriendship(const std::string& email1, const std::string& email2) = 0;

    /**
     * Supprime une relation d'amitié
     * @param email1 Premier utilisateur
     * @param email2 Second utilisateur
     */
    virtual void removeFriendship(const std::string& email1, const std::string& email2) = 0;

    /**
     * Vérifie si deux utilisateurs sont amis
     * @return true si amis
     */
    virtual bool areFriends(const std::string& email1, const std::string& email2) = 0;

    /**
     * Récupère la liste des amis d'un utilisateur
     * @param email Email de l'utilisateur
     * @param offset Pagination offset
     * @param limit Max résultats (default 50)
     * @return Liste des amis
     */
    virtual std::vector<std::string> getFriendEmails(
        const std::string& email,
        size_t offset = 0,
        size_t limit = 50) = 0;

    /**
     * Compte le nombre d'amis
     * @param email Email de l'utilisateur
     * @return Nombre d'amis
     */
    virtual size_t getFriendCount(const std::string& email) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IFRIENDSHIPREPOSITORY_HPP_ */
```

### 6.2 IFriendRequestRepository

**Fichier** : `src/server/include/application/ports/out/persistence/IFriendRequestRepository.hpp`

```cpp
#ifndef IFRIENDREQUESTREPOSITORY_HPP_
#define IFRIENDREQUESTREPOSITORY_HPP_

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace application::ports::out::persistence {

struct FriendRequestData {
    std::string fromEmail;
    std::string toEmail;
    std::string fromDisplayName;
    std::chrono::system_clock::time_point createdAt;
};

class IFriendRequestRepository {
public:
    virtual ~IFriendRequestRepository() = default;

    /**
     * Crée une demande d'ami
     * @param fromEmail Expéditeur
     * @param toEmail Destinataire
     * @param fromDisplayName Nom de l'expéditeur
     */
    virtual void createRequest(
        const std::string& fromEmail,
        const std::string& toEmail,
        const std::string& fromDisplayName) = 0;

    /**
     * Supprime une demande d'ami
     * @param fromEmail Expéditeur original
     * @param toEmail Destinataire original
     */
    virtual void deleteRequest(const std::string& fromEmail, const std::string& toEmail) = 0;

    /**
     * Vérifie si une demande existe
     * @return true si demande existe de from vers to
     */
    virtual bool requestExists(const std::string& fromEmail, const std::string& toEmail) = 0;

    /**
     * Récupère une demande spécifique
     * @return Données de la demande ou nullopt
     */
    virtual std::optional<FriendRequestData> getRequest(
        const std::string& fromEmail,
        const std::string& toEmail) = 0;

    /**
     * Récupère les demandes reçues par un utilisateur
     * @param email Destinataire
     * @return Liste des demandes reçues
     */
    virtual std::vector<FriendRequestData> getIncomingRequests(const std::string& email) = 0;

    /**
     * Récupère les demandes envoyées par un utilisateur
     * @param email Expéditeur
     * @return Liste des demandes envoyées
     */
    virtual std::vector<FriendRequestData> getOutgoingRequests(const std::string& email) = 0;

    /**
     * Compte les demandes en attente (reçues)
     * @param email Destinataire
     * @return Nombre de demandes
     */
    virtual size_t getPendingRequestCount(const std::string& email) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IFRIENDREQUESTREPOSITORY_HPP_ */
```

### 6.3 IBlockedUserRepository

**Fichier** : `src/server/include/application/ports/out/persistence/IBlockedUserRepository.hpp`

```cpp
#ifndef IBLOCKEDUSERREPOSITORY_HPP_
#define IBLOCKEDUSERREPOSITORY_HPP_

#include <string>
#include <vector>
#include <chrono>

namespace application::ports::out::persistence {

struct BlockedUserData {
    std::string blockedEmail;
    std::string blockedDisplayName;
    std::chrono::system_clock::time_point createdAt;
};

class IBlockedUserRepository {
public:
    virtual ~IBlockedUserRepository() = default;

    /**
     * Bloque un utilisateur
     * @param blockerEmail Qui bloque
     * @param blockedEmail Qui est bloqué
     * @param blockedDisplayName Nom du bloqué
     */
    virtual void blockUser(
        const std::string& blockerEmail,
        const std::string& blockedEmail,
        const std::string& blockedDisplayName) = 0;

    /**
     * Débloque un utilisateur
     */
    virtual void unblockUser(
        const std::string& blockerEmail,
        const std::string& blockedEmail) = 0;

    /**
     * Vérifie si blocker a bloqué blocked
     * @return true si bloqué
     */
    virtual bool isBlocked(
        const std::string& blockerEmail,
        const std::string& blockedEmail) = 0;

    /**
     * Vérifie si l'un des deux a bloqué l'autre
     * @return true si blocage dans un sens ou l'autre
     */
    virtual bool hasAnyBlock(const std::string& email1, const std::string& email2) = 0;

    /**
     * Récupère la liste des utilisateurs bloqués
     * @param blockerEmail Qui a bloqué
     * @return Liste des bloqués
     */
    virtual std::vector<BlockedUserData> getBlockedUsers(const std::string& blockerEmail) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IBLOCKEDUSERREPOSITORY_HPP_ */
```

### 6.4 IPrivateMessageRepository

**Fichier** : `src/server/include/application/ports/out/persistence/IPrivateMessageRepository.hpp`

```cpp
#ifndef IPRIVATEMESSAGEREPOSITORY_HPP_
#define IPRIVATEMESSAGEREPOSITORY_HPP_

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace application::ports::out::persistence {

struct PrivateMessageData {
    uint64_t id;                // MongoDB ObjectId as uint64 (truncated)
    std::string senderEmail;
    std::string recipientEmail;
    std::string senderDisplayName;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    bool isRead;
};

struct ConversationSummaryData {
    std::string otherEmail;
    std::string otherDisplayName;
    std::string lastMessage;
    std::chrono::system_clock::time_point lastTimestamp;
    uint8_t unreadCount;
};

class IPrivateMessageRepository {
public:
    virtual ~IPrivateMessageRepository() = default;

    /**
     * Sauvegarde un message privé
     * @return ID unique du message
     */
    virtual uint64_t saveMessage(
        const std::string& senderEmail,
        const std::string& recipientEmail,
        const std::string& senderDisplayName,
        const std::string& message) = 0;

    /**
     * Récupère l'historique d'une conversation
     * @param email1 Premier participant
     * @param email2 Second participant
     * @param offset Pagination
     * @param limit Max messages (default 50)
     * @return Messages triés par date décroissante
     */
    virtual std::vector<PrivateMessageData> getConversation(
        const std::string& email1,
        const std::string& email2,
        size_t offset = 0,
        size_t limit = 50) = 0;

    /**
     * Récupère la liste des conversations d'un utilisateur
     * @param email Utilisateur
     * @param limit Max conversations
     * @return Résumés triés par date du dernier message
     */
    virtual std::vector<ConversationSummaryData> getConversationsList(
        const std::string& email,
        size_t limit = 50) = 0;

    /**
     * Marque les messages d'une conversation comme lus
     * @param readerEmail Qui lit
     * @param senderEmail De qui viennent les messages
     */
    virtual void markAsRead(
        const std::string& readerEmail,
        const std::string& senderEmail) = 0;

    /**
     * Compte les messages non lus pour un utilisateur
     * @param email Destinataire
     * @return Nombre total de messages non lus
     */
    virtual size_t getUnreadCount(const std::string& email) = 0;

    /**
     * Compte les messages non lus d'un expéditeur spécifique
     * @param recipientEmail Destinataire
     * @param senderEmail Expéditeur
     * @return Nombre de messages non lus de cet expéditeur
     */
    virtual size_t getUnreadCountFrom(
        const std::string& recipientEmail,
        const std::string& senderEmail) = 0;
};

} // namespace application::ports::out::persistence

#endif /* !IPRIVATEMESSAGEREPOSITORY_HPP_ */
```

---

## 7. Handlers TCPAuthServer

### 7.1 Déclarations (TCPAuthServer.hpp)

```cpp
// Dans la class Session (après ligne 110)

// Friends System handlers
void handleSendFriendRequest(const std::vector<uint8_t>& payload);
void handleAcceptFriendRequest(const std::vector<uint8_t>& payload);
void handleRejectFriendRequest(const std::vector<uint8_t>& payload);
void handleRemoveFriend(const std::vector<uint8_t>& payload);
void handleBlockUser(const std::vector<uint8_t>& payload);
void handleUnblockUser(const std::vector<uint8_t>& payload);
void handleGetFriendsList(const std::vector<uint8_t>& payload);
void handleGetFriendRequests(const std::vector<uint8_t>& payload);
void handleGetBlockedUsers(const std::vector<uint8_t>& payload);

// Private Messaging handlers
void handleSendPrivateMessage(const std::vector<uint8_t>& payload);
void handleGetConversation(const std::vector<uint8_t>& payload);
void handleGetConversationsList(const std::vector<uint8_t>& payload);
void handleMarkMessagesRead(const std::vector<uint8_t>& payload);

// Friend notifications (push to specific user)
void notifyFriendRequestReceived(const std::string& fromEmail, const std::string& fromDisplayName);
void notifyFriendRequestAccepted(const std::string& friendEmail, const std::string& displayName, uint8_t onlineStatus);
void notifyFriendRemoved(const std::string& friendEmail);
void notifyFriendStatusChanged(const std::string& friendEmail, uint8_t newStatus, const std::string& roomCode);
void notifyPrivateMessageReceived(const PrivateMessageWire& message);

// Response writers
void do_write_friend_request_ack(uint8_t errorCode, const std::string& targetEmail);
void do_write_friends_list(const std::vector<FriendInfoWire>& friends);
void do_write_friend_requests(const std::vector<FriendRequestInfoWire>& requests);
void do_write_blocked_users(const std::vector<FriendInfoWire>& users);
void do_write_private_message_ack(uint8_t errorCode, uint64_t messageId);
void do_write_conversation(const std::vector<PrivateMessageWire>& messages);
void do_write_conversations_list(const std::vector<ConversationSummaryWire>& conversations);
```

### 7.2 Nouveaux Repositories (injection de dépendances)

```cpp
// Dans Session class, ajouter aux membres privés
std::shared_ptr<IFriendshipRepository> _friendshipRepository;
std::shared_ptr<IFriendRequestRepository> _friendRequestRepository;
std::shared_ptr<IBlockedUserRepository> _blockedUserRepository;
std::shared_ptr<IPrivateMessageRepository> _privateMessageRepository;
```

### 7.3 Dispatch (handle_command)

```cpp
// Dans Session::handle_command(), ajouter après les handlers existants (ligne ~380)

// Friends System
case MessageType::SendFriendRequest:
    handleSendFriendRequest(payload);
    return;
case MessageType::AcceptFriendRequest:
    handleAcceptFriendRequest(payload);
    return;
case MessageType::RejectFriendRequest:
    handleRejectFriendRequest(payload);
    return;
case MessageType::RemoveFriend:
    handleRemoveFriend(payload);
    return;
case MessageType::BlockUser:
    handleBlockUser(payload);
    return;
case MessageType::UnblockUser:
    handleUnblockUser(payload);
    return;
case MessageType::GetFriendsList:
    handleGetFriendsList(payload);
    return;
case MessageType::GetFriendRequests:
    handleGetFriendRequests(payload);
    return;
case MessageType::GetBlockedUsers:
    handleGetBlockedUsers(payload);
    return;

// Private Messaging
case MessageType::SendPrivateMessage:
    handleSendPrivateMessage(payload);
    return;
case MessageType::GetConversation:
    handleGetConversation(payload);
    return;
case MessageType::GetConversationsList:
    handleGetConversationsList(payload);
    return;
case MessageType::MarkMessagesRead:
    handleMarkMessagesRead(payload);
    return;
```

### 7.4 Implémentation Type (handleSendFriendRequest)

```cpp
void Session::handleSendFriendRequest(const std::vector<uint8_t>& payload) {
    auto logger = server::logging::Logger::getNetworkLogger();
    std::string myEmail = _user->getEmail().value();
    std::string myDisplayName = _user->getUsername().value();

    // 1. Parse payload
    auto reqOpt = SendFriendRequestPayload::from_bytes(payload.data(), payload.size());
    if (!reqOpt) {
        logger->warn("Invalid SendFriendRequest payload from {}", myEmail);
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::InternalError), "");
        return;
    }
    std::string targetEmail(reqOpt->targetEmail);

    // 2. Validate: can't add self
    if (targetEmail == myEmail) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::CannotAddSelf), targetEmail);
        return;
    }

    // 3. Check target exists
    auto targetUser = _userRepository->findByEmail(targetEmail);
    if (!targetUser) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::UserNotFound), targetEmail);
        return;
    }

    // 4. Check not blocked
    if (_blockedUserRepository->isBlocked(myEmail, targetEmail)) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::IsBlocked), targetEmail);
        return;
    }
    if (_blockedUserRepository->isBlocked(targetEmail, myEmail)) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::BlockedByUser), targetEmail);
        return;
    }

    // 5. Check not already friends
    if (_friendshipRepository->areFriends(myEmail, targetEmail)) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::AlreadyFriends), targetEmail);
        return;
    }

    // 6. Check request not already sent
    if (_friendRequestRepository->requestExists(myEmail, targetEmail)) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::RequestAlreadySent), targetEmail);
        return;
    }

    // 7. Check if reverse request exists (they already sent us one)
    if (_friendRequestRepository->requestExists(targetEmail, myEmail)) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::RequestAlreadyReceived), targetEmail);
        return;
    }

    // 8. Check max friends limit
    if (_friendshipRepository->getFriendCount(myEmail) >= MAX_FRIENDS) {
        do_write_friend_request_ack(
            static_cast<uint8_t>(FriendErrorCode::MaxFriendsReached), targetEmail);
        return;
    }

    // 9. Create the request
    _friendRequestRepository->createRequest(myEmail, targetEmail, myDisplayName);

    // 10. Send ACK to requester
    do_write_friend_request_ack(
        static_cast<uint8_t>(FriendErrorCode::Success), targetEmail);

    // 11. Notify target if online (via FriendManager callback)
    _friendManager->notifyFriendRequestReceived(targetEmail, myEmail, myDisplayName);

    logger->info("Friend request sent from {} to {}", myEmail, targetEmail);
}
```

---

## 8. Synchronisation Temps Réel

### 8.1 FriendManager (nouvelle classe)

**Fichier** : `src/server/include/infrastructure/social/FriendManager.hpp`

```cpp
#ifndef FRIENDMANAGER_HPP_
#define FRIENDMANAGER_HPP_

#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "Protocol.hpp"

namespace infrastructure::social {

// Callback types for friend notifications
using FriendRequestReceivedCallback = std::function<void(const std::string& fromEmail, const std::string& fromDisplayName)>;
using FriendRequestAcceptedCallback = std::function<void(const std::string& friendEmail, const std::string& displayName, uint8_t onlineStatus)>;
using FriendRemovedCallback = std::function<void(const std::string& friendEmail)>;
using FriendStatusChangedCallback = std::function<void(const std::string& friendEmail, uint8_t newStatus, const std::string& roomCode)>;
using PrivateMessageReceivedCallback = std::function<void(const PrivateMessageWire& message)>;

struct FriendCallbacks {
    FriendRequestReceivedCallback onRequestReceived;
    FriendRequestAcceptedCallback onRequestAccepted;
    FriendRemovedCallback onFriendRemoved;
    FriendStatusChangedCallback onStatusChanged;
    PrivateMessageReceivedCallback onPrivateMessage;
};

class FriendManager {
public:
    FriendManager() = default;
    ~FriendManager() = default;

    // Callback registration (called by TCPAuthServer Session)
    void registerCallbacks(const std::string& email, const FriendCallbacks& callbacks);
    void unregisterCallbacks(const std::string& email);

    // Notification methods (called by handlers)
    void notifyFriendRequestReceived(
        const std::string& targetEmail,
        const std::string& fromEmail,
        const std::string& fromDisplayName);

    void notifyFriendRequestAccepted(
        const std::string& targetEmail,
        const std::string& friendEmail,
        const std::string& displayName,
        uint8_t onlineStatus);

    void notifyFriendRemoved(
        const std::string& targetEmail,
        const std::string& friendEmail);

    void notifyFriendStatusChanged(
        const std::vector<std::string>& friendEmails,
        const std::string& changedEmail,
        uint8_t newStatus,
        const std::string& roomCode);

    void notifyPrivateMessage(
        const std::string& recipientEmail,
        const PrivateMessageWire& message);

private:
    mutable std::mutex _mutex;
    std::unordered_map<std::string, FriendCallbacks> _callbacks;
};

} // namespace infrastructure::social

#endif /* !FRIENDMANAGER_HPP_ */
```

### 8.2 Intégration SessionManager

```cpp
// Dans SessionManager.hpp, ajouter callback pour status changes

using FriendStatusUpdateCallback = std::function<void(
    const std::string& email,
    uint8_t newStatus,
    const std::string& roomCode)>;

void setFriendStatusUpdateCallback(FriendStatusUpdateCallback callback);

// Appelé automatiquement lors de :
// - createSession() → Online
// - setRoomCode() → InLobby
// - (game start via UDPServer) → InGame
// - removeSession() → Offline
```

### 8.3 Points d'Intégration

| Événement | Où | Action |
|-----------|-----|--------|
| Login réussi | `TCPAuthServer::onLoginSuccess()` | Statut → Online, notifier amis |
| Join Room | `TCPAuthServer::handleJoinRoomByCode()` | Statut → InLobby, notifier amis |
| Game Start | `UDPServer::handleJoinGame()` | Statut → InGame, notifier amis |
| Leave Room | `TCPAuthServer::handleLeaveRoom()` | Statut → Online, notifier amis |
| Disconnect | `TCPAuthServer::Session::~Session()` | Statut → Offline, notifier amis |

---

## 9. Architecture Client

### 9.1 TCPClient (nouvelles méthodes)

**Fichier** : `src/client/include/network/TCPClient.hpp` (ajouter après ligne 100)

```cpp
// ═══════════════════════════════════════════════════════════════════
// Friends System
// ═══════════════════════════════════════════════════════════════════

/**
 * Envoie une demande d'ami
 * @param targetEmail Email de la cible
 */
void sendFriendRequest(const std::string& targetEmail);

/**
 * Accepte une demande d'ami
 * @param fromEmail Email de l'expéditeur de la demande
 */
void acceptFriendRequest(const std::string& fromEmail);

/**
 * Refuse une demande d'ami
 * @param fromEmail Email de l'expéditeur de la demande
 */
void rejectFriendRequest(const std::string& fromEmail);

/**
 * Supprime un ami
 * @param friendEmail Email de l'ami à supprimer
 */
void removeFriend(const std::string& friendEmail);

/**
 * Bloque un utilisateur
 * @param targetEmail Email à bloquer
 */
void blockUser(const std::string& targetEmail);

/**
 * Débloque un utilisateur
 * @param targetEmail Email à débloquer
 */
void unblockUser(const std::string& targetEmail);

/**
 * Demande la liste des amis
 * @param offset Pagination offset
 * @param limit Max résultats (default 50)
 */
void getFriendsList(uint8_t offset = 0, uint8_t limit = 50);

/**
 * Demande la liste des demandes d'ami
 */
void getFriendRequests();

/**
 * Demande la liste des utilisateurs bloqués
 */
void getBlockedUsers();

// ═══════════════════════════════════════════════════════════════════
// Private Messaging
// ═══════════════════════════════════════════════════════════════════

/**
 * Envoie un message privé
 * @param recipientEmail Email du destinataire
 * @param message Contenu du message (max 512 chars)
 */
void sendPrivateMessage(const std::string& recipientEmail, const std::string& message);

/**
 * Demande l'historique d'une conversation
 * @param otherEmail Email de l'autre participant
 * @param offset Pagination offset
 * @param limit Max messages (default 50)
 */
void getConversation(const std::string& otherEmail, uint8_t offset = 0, uint8_t limit = 50);

/**
 * Demande la liste des conversations
 */
void getConversationsList();

/**
 * Marque les messages d'une conversation comme lus
 * @param otherEmail Email de l'autre participant
 */
void markMessagesRead(const std::string& otherEmail);
```

### 9.2 NetworkEvents (nouveaux événements)

**Fichier** : `src/client/include/network/NetworkEvents.hpp` (ajouter après ligne 162)

```cpp
// ═══════════════════════════════════════════════════════════════════
// Friends System Events
// ═══════════════════════════════════════════════════════════════════

// Réponse à l'envoi d'une demande d'ami
struct FriendRequestSentEvent {
    uint8_t errorCode;        // FriendErrorCode
    std::string targetEmail;
};

// Notification: demande d'ami reçue (push)
struct FriendRequestReceivedEvent {
    std::string fromEmail;
    std::string fromDisplayName;
};

// Réponse à l'acceptation d'une demande
struct FriendRequestAcceptAckEvent {
    uint8_t errorCode;
};

// Notification: notre demande a été acceptée (push)
struct FriendRequestAcceptedEvent {
    std::string friendEmail;
    std::string friendDisplayName;
    uint8_t onlineStatus;
};

// Réponse au rejet d'une demande
struct FriendRequestRejectAckEvent {
    uint8_t errorCode;
};

// Réponse à la suppression d'un ami
struct FriendRemovedAckEvent {
    uint8_t errorCode;
};

// Notification: un ami nous a supprimé (push)
struct FriendRemovedEvent {
    std::string friendEmail;
};

// Réponse au blocage
struct BlockUserAckEvent {
    uint8_t errorCode;
};

// Réponse au déblocage
struct UnblockUserAckEvent {
    uint8_t errorCode;
};

// Liste des amis
struct FriendsListEvent {
    std::vector<FriendInfoWire> friends;
    uint8_t totalCount;
};

// Liste des demandes d'ami
struct FriendRequestsEvent {
    std::vector<FriendRequestInfoWire> incoming;   // Reçues
    std::vector<FriendRequestInfoWire> outgoing;   // Envoyées
};

// Liste des utilisateurs bloqués
struct BlockedUsersEvent {
    std::vector<FriendInfoWire> blockedUsers;
};

// Notification: statut d'un ami changé (push)
struct FriendStatusChangedEvent {
    std::string friendEmail;
    uint8_t newStatus;        // FriendOnlineStatus
    std::string roomCode;     // Si InGame/InLobby
};

// ═══════════════════════════════════════════════════════════════════
// Private Messaging Events
// ═══════════════════════════════════════════════════════════════════

// Réponse à l'envoi d'un message
struct PrivateMessageSentEvent {
    uint8_t errorCode;
    uint64_t messageId;
};

// Notification: message reçu (push)
struct PrivateMessageReceivedEvent {
    std::string senderEmail;
    std::string senderDisplayName;
    std::string message;
    uint64_t timestamp;
};

// Historique d'une conversation
struct ConversationEvent {
    std::string otherEmail;
    std::vector<PrivateMessageWire> messages;
    bool hasMore;             // Pour pagination
};

// Liste des conversations
struct ConversationsListEvent {
    std::vector<ConversationSummaryWire> conversations;
};

// Réponse au marquage comme lu
struct MessagesReadAckEvent {
    uint8_t errorCode;
};
```

### 9.3 Mise à jour TCPEvent variant

```cpp
// Dans NetworkEvents.hpp, mettre à jour le variant TCPEvent (ligne 179)
using TCPEvent = std::variant<
    // ... événements existants ...

    // Friends System Events
    FriendRequestSentEvent,
    FriendRequestReceivedEvent,
    FriendRequestAcceptAckEvent,
    FriendRequestAcceptedEvent,
    FriendRequestRejectAckEvent,
    FriendRemovedAckEvent,
    FriendRemovedEvent,
    BlockUserAckEvent,
    UnblockUserAckEvent,
    FriendsListEvent,
    FriendRequestsEvent,
    BlockedUsersEvent,
    FriendStatusChangedEvent,

    // Private Messaging Events
    PrivateMessageSentEvent,
    PrivateMessageReceivedEvent,
    ConversationEvent,
    ConversationsListEvent,
    MessagesReadAckEvent
>;
```

---

## 10. Scènes UI

### 10.1 FriendsScene

**Fichiers** :
- `src/client/include/scenes/FriendsScene.hpp`
- `src/client/src/scenes/FriendsScene.cpp`

```
┌─────────────────────────────────────────────────────────────────┐
│                        R-TYPE FRIENDS                           │
├─────────────────────────────────────────────────────────────────┤
│  [FRIENDS]  [REQUESTS (3)]  [BLOCKED]  [MESSAGES (5)]          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ 🟢 Alice          Online              [MSG] [JOIN]      │  │
│   │ 🎮 Bob            Playing - ROOM123   [MSG] [JOIN]      │  │
│   │ 🚪 Charlie        In Lobby - ABCDEF   [MSG] [JOIN]      │  │
│   │ ⚪ David          Last seen: 2h ago   [MSG]             │  │
│   │ ⚪ Eve            Last seen: 1d ago   [MSG]             │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│   ┌───────────────────────────────────────┐                    │
│   │ Add Friend: [________________] [SEND] │                    │
│   └───────────────────────────────────────┘                    │
│                                                                 │
│                                            [BACK]              │
└─────────────────────────────────────────────────────────────────┘
```

**Onglet Requests** :
```
┌─────────────────────────────────────────────────────────────────┐
│  RECEIVED REQUESTS:                                             │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ Frank           2 hours ago     [ACCEPT] [REJECT]       │  │
│   │ Grace           1 day ago       [ACCEPT] [REJECT]       │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│  SENT REQUESTS:                                                 │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ Henry           3 days ago      [CANCEL]                │  │
│   └─────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

**Onglet Messages** :
```
┌─────────────────────────────────────────────────────────────────┐
│  CONVERSATIONS:                                                 │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ 🟢 Alice (3)    "Hey, want to play?"        2 min ago   │  │
│   │ 🎮 Bob          "GG!"                       1 hour ago  │  │
│   │ ⚪ Charlie      "See you tomorrow"          1 day ago   │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│  Click on a conversation to open chat                          │
└─────────────────────────────────────────────────────────────────┘
```

### 10.2 PrivateChatScene

**Fichiers** :
- `src/client/include/scenes/PrivateChatScene.hpp`
- `src/client/src/scenes/PrivateChatScene.cpp`

```
┌─────────────────────────────────────────────────────────────────┐
│  < BACK                 Alice (🟢 Online)                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                                       ┌─────────────────────┐  │
│                                       │ Hey!                │  │
│                                       │            10:05 AM │  │
│                                       └─────────────────────┘  │
│  ┌─────────────────────┐                                       │
│  │ Hi Alice!           │                                       │
│  │ 10:06 AM            │                                       │
│  └─────────────────────┘                                       │
│                                       ┌─────────────────────┐  │
│                                       │ Want to play?       │  │
│                                       │            10:07 AM │  │
│                                       └─────────────────────┘  │
│  ┌─────────────────────┐                                       │
│  │ Sure! Creating room │                                       │
│  │ 10:08 AM            │                                       │
│  └─────────────────────┘                                       │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│  [_________________________________] [SEND]                     │
└─────────────────────────────────────────────────────────────────┘
```

### 10.3 Composants UI Réutilisables

```cpp
// Bouton ami (src/client/include/ui/FriendListItem.hpp)
class FriendListItem : public UIComponent {
    std::string email;
    std::string displayName;
    FriendOnlineStatus status;
    std::string roomCode;

    void render(IWindow& window, float x, float y);
    bool isHovered(float mouseX, float mouseY);

    enum class Action { None, Message, Join, Remove, Block };
    Action getClickedAction(float mouseX, float mouseY);
};

// Badge notification (src/client/include/ui/NotificationBadge.hpp)
class NotificationBadge : public UIComponent {
    int count;
    void render(IWindow& window, float x, float y);
};

// Input texte (déjà existant dans LobbyScene, réutiliser)
```

---

## 11. Flux de Données Complets

### 11.1 Envoi de Demande d'Ami

```
┌─────────┐                    ┌─────────────┐                    ┌─────────┐
│ Client  │                    │   Server    │                    │ MongoDB │
└────┬────┘                    └──────┬──────┘                    └────┬────┘
     │                                │                                │
     │─ SendFriendRequest(bob@...) ──>│                                │
     │                                │                                │
     │                                │─ findByEmail(bob@...) ────────>│
     │                                │<─ UserData ────────────────────│
     │                                │                                │
     │                                │─ isBlocked(me, bob) ──────────>│
     │                                │<─ false ───────────────────────│
     │                                │                                │
     │                                │─ isBlocked(bob, me) ──────────>│
     │                                │<─ false ───────────────────────│
     │                                │                                │
     │                                │─ areFriends(me, bob) ─────────>│
     │                                │<─ false ───────────────────────│
     │                                │                                │
     │                                │─ requestExists(me, bob) ──────>│
     │                                │<─ false ───────────────────────│
     │                                │                                │
     │                                │─ requestExists(bob, me) ──────>│
     │                                │<─ false ───────────────────────│
     │                                │                                │
     │                                │─ createRequest(me, bob) ──────>│
     │                                │<─ OK ──────────────────────────│
     │                                │                                │
     │<─ SendFriendRequestAck(OK) ────│                                │
     │                                │                                │
     │                                │─ getSessionByEmail(bob) ──────>│
     │                                │                                │
     │                                │   [Bob is online]              │
     │                                │──> FriendRequestReceived ─────>│ Bob
     │                                │                                │
```

### 11.2 Acceptation de Demande

```
┌─────────┐                    ┌─────────────┐                    ┌─────────┐
│  Bob    │                    │   Server    │                    │  Alice  │
└────┬────┘                    └──────┬──────┘                    └────┬────┘
     │                                │                                │
     │─ AcceptFriendRequest(alice) ──>│                                │
     │                                │                                │
     │                                │─ DB: getRequest(alice,bob) ───>│
     │                                │<─ RequestData ─────────────────│
     │                                │                                │
     │                                │─ DB: addFriendship(alice,bob) >│
     │                                │<─ OK ──────────────────────────│
     │                                │                                │
     │                                │─ DB: deleteRequest(alice,bob) >│
     │                                │<─ OK ──────────────────────────│
     │                                │                                │
     │<─ AcceptFriendRequestAck(OK) ──│                                │
     │                                │                                │
     │                                │─ getSession(alice) ───────────>│
     │                                │                                │
     │                                │   [Alice is online]            │
     │                                │──> FriendRequestAccepted ─────>│
     │                                │    (bob, Online)               │
```

### 11.3 Changement de Statut (Join Room)

```
┌─────────┐                    ┌─────────────┐                    ┌─────────┐
│  Alice  │                    │   Server    │                    │ Friends │
└────┬────┘                    └──────┬──────┘                    └────┬────┘
     │                                │                                │
     │─ JoinRoomByCode(ROOM123) ─────>│                                │
     │                                │                                │
     │                                │─ [Join logic...] ─────────────>│
     │                                │                                │
     │<─ JoinRoomAck(OK) ─────────────│                                │
     │                                │                                │
     │                                │─ DB: getFriendEmails(alice) ──>│
     │                                │<─ [bob, charlie, david] ───────│
     │                                │                                │
     │                                │─ For each friend online: ─────>│
     │                                │   FriendStatusChanged          │
     │                                │   (alice, InLobby, ROOM123)    │
     │                                │                                │
```

### 11.4 Message Privé

```
┌─────────┐                    ┌─────────────┐                    ┌─────────┐
│  Alice  │                    │   Server    │                    │   Bob   │
└────┬────┘                    └──────┬──────┘                    └────┬────┘
     │                                │                                │
     │─ SendPrivateMessage(bob,"Hi")─>│                                │
     │                                │                                │
     │                                │─ DB: areFriends(alice,bob) ───>│
     │                                │<─ true ────────────────────────│
     │                                │                                │
     │                                │─ DB: hasAnyBlock(alice,bob) ──>│
     │                                │<─ false ───────────────────────│
     │                                │                                │
     │                                │─ DB: saveMessage(...) ────────>│
     │                                │<─ messageId=12345 ─────────────│
     │                                │                                │
     │<─ SendPrivateMessageAck(OK,ID)─│                                │
     │                                │                                │
     │                                │─ getSession(bob) ─────────────>│
     │                                │                                │
     │                                │   [Bob is online]              │
     │                                │──> PrivateMessageReceived ────>│
     │                                │    (alice, "Alice", "Hi")      │
```

---

## 12. Sécurité et Validation

### 12.1 Règles de Validation

| Règle | Implémentation |
|-------|----------------|
| **Email valide** | Regex + vérification existence |
| **Auto-ajout interdit** | `if (target == self)` → CannotAddSelf |
| **Blocage bidirectionnel** | Vérifier dans les deux sens |
| **Messages aux amis uniquement** | `if (!areFriends)` → NotFriends |
| **Limite amis** | `if (count >= 100)` → MaxFriendsReached |
| **Longueur message** | `if (msg.len > 512)` → tronquer ou rejeter |
| **Rate limiting** | Max 10 demandes/minute (TODO) |
| **Sanitization** | Échapper HTML/JS dans messages |

### 12.2 Validation Côté Client

```cpp
bool validateFriendRequest(const std::string& email) {
    // Email non vide
    if (email.empty()) return false;

    // Format email basique
    if (email.find('@') == std::string::npos) return false;

    // Pas soi-même
    if (email == _tcpClient->getEmail()) return false;

    // Longueur max
    if (email.length() > MAX_EMAIL_LEN - 1) return false;

    return true;
}

bool validatePrivateMessage(const std::string& message) {
    // Non vide
    if (message.empty()) return false;

    // Longueur max
    if (message.length() > MAX_MESSAGE_LEN - 1) return false;

    return true;
}
```

### 12.3 Validation Côté Serveur

```cpp
// Ordre de vérification pour SendFriendRequest
1. Parse payload valide
2. targetEmail != myEmail (CannotAddSelf)
3. User exists (UserNotFound)
4. !isBlocked(me, target) (IsBlocked)
5. !isBlocked(target, me) (BlockedByUser)
6. !areFriends(me, target) (AlreadyFriends)
7. !requestExists(me, target) (RequestAlreadySent)
8. !requestExists(target, me) (RequestAlreadyReceived)
9. friendCount < MAX_FRIENDS (MaxFriendsReached)
```

---

## 13. Tests et Validation

### 13.1 Tests Unitaires Repository

**Fichier** : `tests/server/infrastructure/persistence/FriendshipRepositoryTest.cpp`

```cpp
TEST(FriendshipRepository, AddAndCheck) {
    auto repo = createTestRepo();
    repo->addFriendship("alice@test.com", "bob@test.com");

    EXPECT_TRUE(repo->areFriends("alice@test.com", "bob@test.com"));
    EXPECT_TRUE(repo->areFriends("bob@test.com", "alice@test.com")); // Bidirectionnel
}

TEST(FriendshipRepository, RemoveFriend) {
    auto repo = createTestRepo();
    repo->addFriendship("alice@test.com", "bob@test.com");
    repo->removeFriendship("alice@test.com", "bob@test.com");

    EXPECT_FALSE(repo->areFriends("alice@test.com", "bob@test.com"));
}

TEST(FriendshipRepository, FriendCount) {
    auto repo = createTestRepo();
    repo->addFriendship("alice@test.com", "bob@test.com");
    repo->addFriendship("alice@test.com", "charlie@test.com");

    EXPECT_EQ(repo->getFriendCount("alice@test.com"), 2);
}
```

### 13.2 Tests d'Intégration

```cpp
TEST(FriendsSystem, FullFriendRequestFlow) {
    // Setup
    auto server = createTestServer();
    auto alice = createTestClient("alice@test.com");
    auto bob = createTestClient("bob@test.com");

    // Alice envoie demande
    alice.sendFriendRequest("bob@test.com");
    auto ack = alice.waitForEvent<FriendRequestSentEvent>();
    EXPECT_EQ(ack.errorCode, FriendErrorCode::Success);

    // Bob reçoit notification
    auto notif = bob.waitForEvent<FriendRequestReceivedEvent>();
    EXPECT_EQ(notif.fromEmail, "alice@test.com");

    // Bob accepte
    bob.acceptFriendRequest("alice@test.com");
    auto acceptAck = bob.waitForEvent<FriendRequestAcceptAckEvent>();
    EXPECT_EQ(acceptAck.errorCode, FriendErrorCode::Success);

    // Alice reçoit notification d'acceptation
    auto acceptNotif = alice.waitForEvent<FriendRequestAcceptedEvent>();
    EXPECT_EQ(acceptNotif.friendEmail, "bob@test.com");

    // Vérifier qu'ils sont amis
    alice.getFriendsList();
    auto friends = alice.waitForEvent<FriendsListEvent>();
    EXPECT_EQ(friends.friends.size(), 1);
    EXPECT_STREQ(friends.friends[0].email, "bob@test.com");
}
```

### 13.3 Tests de Sécurité

```cpp
TEST(FriendsSystem, CannotAddSelf) {
    auto alice = createTestClient("alice@test.com");
    alice.sendFriendRequest("alice@test.com");

    auto ack = alice.waitForEvent<FriendRequestSentEvent>();
    EXPECT_EQ(ack.errorCode, FriendErrorCode::CannotAddSelf);
}

TEST(FriendsSystem, CannotMessageNonFriend) {
    auto alice = createTestClient("alice@test.com");
    alice.sendPrivateMessage("stranger@test.com", "Hello");

    auto ack = alice.waitForEvent<PrivateMessageSentEvent>();
    EXPECT_EQ(ack.errorCode, FriendErrorCode::NotFriends);
}

TEST(FriendsSystem, BlockedUserCannotSendRequest) {
    auto alice = createTestClient("alice@test.com");
    auto bob = createTestClient("bob@test.com");

    // Alice bloque Bob
    alice.blockUser("bob@test.com");
    alice.waitForEvent<BlockUserAckEvent>();

    // Bob essaie d'envoyer une demande
    bob.sendFriendRequest("alice@test.com");
    auto ack = bob.waitForEvent<FriendRequestSentEvent>();
    EXPECT_EQ(ack.errorCode, FriendErrorCode::BlockedByUser);
}
```

---

## 14. Plan d'Implémentation

### Phase 1 : Backend Core (Semaine 1)

| Jour | Tâche | Fichiers |
|------|-------|----------|
| 1 | Créer interfaces repository | 4 fichiers .hpp |
| 2 | Implémenter MongoDBFriendshipRepository | 2 fichiers |
| 3 | Implémenter MongoDBFriendRequestRepository | 2 fichiers |
| 4 | Implémenter MongoDBBlockedUserRepository | 2 fichiers |
| 5 | Implémenter MongoDBPrivateMessageRepository | 2 fichiers |

### Phase 2 : Backend Handlers (Semaine 2)

| Jour | Tâche | Fichiers |
|------|-------|----------|
| 1-2 | FriendManager + callbacks | 2 fichiers |
| 3-4 | TCPAuthServer handlers (friends) | TCPAuthServer.cpp |
| 5 | TCPAuthServer handlers (messages) | TCPAuthServer.cpp |

### Phase 3 : Client Network (Semaine 3)

| Jour | Tâche | Fichiers |
|------|-------|----------|
| 1 | NetworkEvents nouveaux types | NetworkEvents.hpp |
| 2-3 | TCPClient nouvelles méthodes | TCPClient.hpp/cpp |
| 4-5 | Tests d'intégration client-serveur | tests/ |

### Phase 4 : Client UI (Semaine 4)

| Jour | Tâche | Fichiers |
|------|-------|----------|
| 1-2 | FriendsScene base | 2 fichiers |
| 3 | FriendsScene tabs (Requests, Blocked) | FriendsScene.cpp |
| 4-5 | PrivateChatScene | 2 fichiers |

### Phase 5 : Polish & Tests (Semaine 5)

| Jour | Tâche |
|------|-------|
| 1-2 | Tests unitaires complets |
| 3 | Tests d'intégration |
| 4 | Bug fixes |
| 5 | Documentation |

---

## 15. Fichiers à Créer/Modifier

### 15.1 Nouveaux Fichiers (22 fichiers)

```
src/server/
├── include/
│   ├── application/ports/out/persistence/
│   │   ├── IFriendshipRepository.hpp        [NEW]
│   │   ├── IFriendRequestRepository.hpp     [NEW]
│   │   ├── IBlockedUserRepository.hpp       [NEW]
│   │   └── IPrivateMessageRepository.hpp    [NEW]
│   └── infrastructure/
│       ├── adapters/out/persistence/
│       │   ├── MongoDBFriendshipRepository.hpp      [NEW]
│       │   ├── MongoDBFriendRequestRepository.hpp   [NEW]
│       │   ├── MongoDBBlockedUserRepository.hpp     [NEW]
│       │   └── MongoDBPrivateMessageRepository.hpp  [NEW]
│       └── social/
│           └── FriendManager.hpp            [NEW]
├── infrastructure/
│   ├── adapters/out/persistence/
│   │   ├── MongoDBFriendshipRepository.cpp      [NEW]
│   │   ├── MongoDBFriendRequestRepository.cpp   [NEW]
│   │   ├── MongoDBBlockedUserRepository.cpp     [NEW]
│   │   └── MongoDBPrivateMessageRepository.cpp  [NEW]
│   └── social/
│       └── FriendManager.cpp                [NEW]

src/client/
├── include/scenes/
│   ├── FriendsScene.hpp                     [NEW]
│   └── PrivateChatScene.hpp                 [NEW]
└── src/scenes/
    ├── FriendsScene.cpp                     [NEW]
    └── PrivateChatScene.cpp                 [NEW]

tests/
├── server/infrastructure/persistence/
│   ├── FriendshipRepositoryTest.cpp         [NEW]
│   ├── FriendRequestRepositoryTest.cpp      [NEW]
│   └── PrivateMessageRepositoryTest.cpp     [NEW]
└── integration/
    └── FriendsSystemTest.cpp                [NEW]
```

### 15.2 Fichiers à Modifier (8 fichiers)

```
src/common/protocol/Protocol.hpp             [DÉJÀ FAIT - 100%]

src/server/include/infrastructure/
├── adapters/in/network/TCPAuthServer.hpp    [+40 lignes]
├── session/SessionManager.hpp               [+20 lignes]

src/server/infrastructure/
├── adapters/in/network/TCPAuthServer.cpp    [+800 lignes]
├── session/SessionManager.cpp               [+50 lignes]

src/client/include/network/
├── TCPClient.hpp                            [+30 lignes]
└── NetworkEvents.hpp                        [+100 lignes]

src/client/src/network/
└── TCPClient.cpp                            [+200 lignes]
```

---

## Annexes

### A. Codes d'Erreur Complets

| Code | Enum | Description | Contexte |
|------|------|-------------|----------|
| 0 | Success | Opération réussie | Tous |
| 1 | UserNotFound | Email inexistant | SendFriendRequest |
| 2 | AlreadyFriends | Déjà amis | SendFriendRequest |
| 3 | RequestAlreadySent | Demande déjà envoyée | SendFriendRequest |
| 4 | RequestAlreadyReceived | Demande reçue de cette personne | SendFriendRequest |
| 5 | IsBlocked | Vous avez bloqué cette personne | SendFriendRequest, SendPrivateMessage |
| 6 | BlockedByUser | Cette personne vous a bloqué | SendFriendRequest, SendPrivateMessage |
| 7 | CannotAddSelf | Auto-ajout interdit | SendFriendRequest |
| 8 | MaxFriendsReached | Limite 100 amis atteinte | SendFriendRequest, AcceptFriendRequest |
| 9 | MaxRequestsReached | Trop de demandes en attente | SendFriendRequest |
| 10 | NotFriends | Pas amis | RemoveFriend, SendPrivateMessage |
| 11 | RequestNotFound | Demande non trouvée | AcceptFriendRequest, RejectFriendRequest |
| 99 | InternalError | Erreur serveur | Tous |

### B. Tailles Mémoire

| Structure | Taille | Nombre Max | Total Max |
|-----------|--------|------------|-----------|
| FriendInfoWire | 298B | 100 amis | 29.8 KB |
| FriendRequestInfoWire | 292B | 50 demandes | 14.6 KB |
| PrivateMessageWire | 808B | 50 messages | 40.4 KB |
| ConversationSummaryWire | 809B | 50 conversations | 40.45 KB |

### C. Index MongoDB Recommandés

```javascript
// friendships
db.friendships.createIndex({ "user1_email": 1, "user2_email": 1 }, { unique: true });
db.friendships.createIndex({ "user2_email": 1 });

// friend_requests
db.friend_requests.createIndex({ "from_email": 1, "to_email": 1 }, { unique: true });
db.friend_requests.createIndex({ "to_email": 1 });
db.friend_requests.createIndex({ "from_email": 1 });

// blocked_users
db.blocked_users.createIndex({ "blocker_email": 1, "blocked_email": 1 }, { unique: true });
db.blocked_users.createIndex({ "blocked_email": 1 });

// private_messages
db.private_messages.createIndex({ "conversation_key": 1, "timestamp": -1 });
db.private_messages.createIndex({ "recipient_email": 1, "is_read": 1 });
db.private_messages.createIndex({ "timestamp": -1 });  // Pour cleanup
```

---

**Document généré le 2026-01-19 par Claude Code Analysis**
