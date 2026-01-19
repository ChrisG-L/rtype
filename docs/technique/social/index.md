---
tags:
  - technique
  - social
  - friends
  - messaging
---

# Systeme Social

Architecture du systeme d'amis et de messagerie privee.

---

## Architecture

```mermaid
flowchart TB
    subgraph Client
        FS[FriendsScene]
        PCS[PrivateChatScene]
    end

    subgraph Server
        TCP[TCPAuthServer]
        FM[FriendManager<br/>Notifications temps reel]
    end

    subgraph Repositories
        IFR[IFriendshipRepository]
        IFRQ[IFriendRequestRepository]
        IBU[IBlockedUserRepository]
        IPM[IPrivateMessageRepository]
    end

    subgraph MongoDB
        C1[(friendships)]
        C2[(friend_requests)]
        C3[(blocked_users)]
        C4[(private_messages)]
    end

    FS & PCS -->|TCP| TCP
    TCP --> FM
    FM --> IFR & IFRQ & IBU & IPM
    IFR --> C1
    IFRQ --> C2
    IBU --> C3
    IPM --> C4

    style FM fill:#7c3aed,color:#fff
```

---

## Fonctionnalites

| Fonctionnalite | Description |
|----------------|-------------|
| Demandes d'amis | Envoyer/accepter/refuser |
| Liste d'amis | Gestion des relations |
| Blocage | Bloquer/debloquer des utilisateurs |
| Messages prives | Communication 1-to-1 |
| Statut en ligne | Notifications temps reel |

---

## Protocol TCP

Le systeme utilise TCP (port 4125) pour garantir la livraison des messages.

| Plage | Fonctionnalite |
|-------|----------------|
| 0x0600-0x0650 | Amis (demandes, acceptation, blocage, statut) |
| 0x0660-0x0681 | Messages prives (envoi, reception, historique) |

---

## Composants Cles

### Server

| Composant | Role |
|-----------|------|
| `FriendManager` | Gestionnaire de notifications temps reel |
| `IFriendshipRepository` | Interface persistance amities |
| `IFriendRequestRepository` | Interface persistance demandes |
| `IBlockedUserRepository` | Interface persistance blocage |
| `IPrivateMessageRepository` | Interface persistance messages |

### Client

| Composant | Role |
|-----------|------|
| `FriendsScene` | Interface utilisateur amis |
| `PrivateChatScene` | Interface chat prive |

---

## Documentation

<div class="grid-cards">
  <div class="card">
    <h3><a href="../../friends-system/">Friends System</a></h3>
    <p>Protocol wire, structures, codes d'erreur</p>
  </div>
</div>
