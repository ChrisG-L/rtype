# Rapport SECURITY

## AgentDB Data Used

| Tool | Query | Status | Results |
|------|-------|--------|---------|
| error_history | src/server/infrastructure/adapters/in/network/UDPServer.cpp | ✅ | 0 bugs (nouveau fichier) |
| error_history | src/client/src/network/UDPClient.cpp | ✅ | 0 bugs (nouveau fichier) |
| error_history | src/common/protocol/Protocol.hpp | ✅ | 0 bugs |
| error_history | src/server/infrastructure/boostrap/GameBootstrap.hpp | ✅ | 0 bugs |
| patterns | security | ✅ | 3 patterns chargés |
| file_context | UDPServer.cpp | ✅ | security_sensitive=false |
| file_context | Protocol.hpp | ✅ | security_sensitive=true, is_critical=true |
| list_critical_files | - | ✅ | 47 fichiers critiques identifiés |

## Résumé

| Métrique | Valeur |
|----------|--------|
| **Score** | 35/100 |
| **Vulnérabilités critiques** | 2 |
| **Vulnérabilités HIGH** | 3 |
| **Vulnérabilités MEDIUM** | 2 |
| **Régressions détectées** | 0 |
| **Fichiers security_sensitive touchés** | 2 (Protocol.hpp, GameBootstrap.hpp) |

---

## Analyse des Vulnérabilités

### [CWE-798] SEC-001 : Hardcoded Credentials - MongoDB URI avec mot de passe

- **Fichier**: `src/server/infrastructure/boostrap/GameBootstrap.hpp`
- **Ligne**: 30
- **Sévérité**: **CRITICAL**
- **CWE**: CWE-798 (Use of Hard-coded Credentials)

**Code actuel** :
```cpp
std::string mongoURI = "mongodb+srv://dbUser:root@rtypehome.qxzb27g.mongodb.net/";
```

**Description** : Le mot de passe MongoDB ("root") est hardcodé directement dans le code source. Ce fichier est marqué `security_sensitive=true` par AgentDB. Cette URI de connexion contient des credentials qui seront exposés dans:
- Le repository Git
- Les logs de compilation
- Les binaires compilés (strings extractibles)

**Remédiation** :
```cpp
const char* mongoURI = std::getenv("MONGODB_URI");
if (!mongoURI) {
    throw std::runtime_error("MONGODB_URI environment variable not set");
}
```

- **Temps estimé** : 15 min
- **Bloquant** : OUI

---

### [CWE-319] SEC-002 : Transmission en clair de credentials sur UDP

- **Fichier**: `src/common/protocol/Protocol.hpp`
- **Lignes**: 83-98, 100-118
- **Sévérité**: **CRITICAL**
- **CWE**: CWE-319 (Cleartext Transmission of Sensitive Information)

**Code actuel** :
```cpp
struct LoginMessage {
    char username[32];
    char password[255];  // Mot de passe transmis en clair
};

struct RegisterMessage {
    char username[32];
    char email[255];
    char password[255];  // Mot de passe transmis en clair
};
```

**Description** : Le protocole UDP transmet les mots de passe en clair sur le réseau. UDP n'offre aucun chiffrement natif. Un attaquant sur le même réseau peut intercepter les paquets et extraire les credentials.

**Remédiation** :
1. Utiliser TLS/DTLS pour le transport UDP
2. Ne jamais transmettre de mots de passe en clair - utiliser un hash côté client
3. Implémenter un challenge-response pour l'authentification

- **Temps estimé** : 2-4 heures (implémentation DTLS)
- **Bloquant** : OUI pour production

---

### [CWE-312] SEC-003 : Stockage en mémoire de credentials sensibles

- **Fichier**: `src/client/include/network/UDPClient.hpp`
- **Lignes**: 85-87
- **Sévérité**: **HIGH**
- **CWE**: CWE-312 (Cleartext Storage of Sensitive Information)

**Code actuel** :
```cpp
std::string _pendingUsername;
std::string _pendingPassword;
std::string _pendingEmail;
```

**Description** : Les credentials sont stockés en mémoire sans protection.

**Remédiation** :
```cpp
void clearCredentials() {
    std::fill(_pendingPassword.begin(), _pendingPassword.end(), '\0');
    _pendingPassword.clear();
    _pendingPassword.shrink_to_fit();
}
```

- **Temps estimé** : 20 min
- **Bloquant** : Non

---

### [CWE-120] SEC-004 : Buffer overflow potentiel

- **Fichier**: `src/common/protocol/Protocol.hpp`
- **Lignes**: 45-54, 71-80, 92-97, 111-117
- **Sévérité**: **HIGH**
- **CWE**: CWE-120 (Buffer Copy without Checking Size of Input)

**Code actuel** :
```cpp
static Header from_bytes(const uint8_t* buf) {
    Header head;
    std::memcpy(&net_type, buf, 2);  // Pas de vérification
    std::memcpy(&net_size, buf + 2, 4);
}
```

**Remédiation** :
```cpp
static std::optional<LoginMessage> from_bytes(const uint8_t* buf, size_t buf_len) {
    constexpr size_t REQUIRED_SIZE = 32 + 255;
    if (buf == nullptr || buf_len < REQUIRED_SIZE) {
        return std::nullopt;
    }
    // ...
}
```

- **Temps estimé** : 30 min
- **Bloquant** : OUI

---

### [CWE-476] SEC-005 : Déréférencement potentiel de pointeur NULL

- **Fichier**: `src/common/protocol/Protocol.hpp`
- **Lignes**: 45, 71, 92, 111
- **Sévérité**: **HIGH**
- **CWE**: CWE-476 (NULL Pointer Dereference)

**Description** : Aucune vérification de pointeur NULL avant utilisation.

**Remédiation** : Ajouter vérification `if (buf == nullptr) return {};`

- **Temps estimé** : 15 min
- **Bloquant** : OUI

---

### [CWE-252] SEC-006 : Retour de fonction ignoré

- **Fichier**: `src/client/src/network/UDPClient.cpp`
- **Lignes**: 68, 170-176
- **Sévérité**: **MEDIUM**
- **CWE**: CWE-252 (Unchecked Return Value)

**Description** : Les opérations d'envoi réseau ne vérifient pas les erreurs.

- **Temps estimé** : 15 min
- **Bloquant** : Non

---

### [INFO] SEC-007 : Code mort / Fonctions incomplètes

- **Fichier**: `src/client/src/network/UDPClient.cpp`
- **Lignes**: 86-119, 179-233
- **Sévérité**: **MEDIUM**

**Description** : Plusieurs fonctions sont commentées ou vides :
- `disconnect()` : Implémentation commentée
- `handleRead()` : Implémentation commentée
- `handleWrite()` : Implémentation commentée

- **Temps estimé** : 1-2 heures
- **Bloquant** : Non

---

## Vérification des Régressions

| Bug passé | Fichier | Status | Réapparaît? |
|-----------|---------|--------|-------------|
| (aucun historique) | UDPServer.cpp | N/A | N/A |
| (aucun historique) | UDPClient.cpp | N/A | N/A |
| (aucun historique) | Protocol.hpp | N/A | N/A |

**Note** : Aucune régression détectée.

---

## Patterns de Sécurité

| Pattern | Appliqué? | Commentaire |
|---------|-----------|-------------|
| `security_input_validation` | ❌ | `from_bytes()` ne valide pas la taille des buffers |
| `security_sensitive_data` | ❌ | Passwords stockés en clair, pas de nettoyage |
| `memory_safety_bounds_check` | ❌ | memcpy sans vérification de taille |
| `memory_safety_null_deref` | ❌ | Pointeurs non vérifiés avant usage |
| `error_handling_return_codes` | ⚠️ | Certains retours ignorés |

---

## Conclusion

### Risques Majeurs

1. **Credentials hardcodés** (SEC-001) : Compromission possible de la base de données
2. **Transmission en clair** (SEC-002) : Vulnérable aux attaques man-in-the-middle
3. **Buffer overflow** (SEC-004, SEC-005) : Attaques par paquets malformés possibles

### Actions Requises

| Priorité | Action | Effort |
|----------|--------|--------|
| BLOQUANT | Supprimer credentials hardcodés | 15 min |
| BLOQUANT | Ajouter validation de taille dans `from_bytes()` | 30 min |
| BLOQUANT | Ajouter vérification NULL | 15 min |
| HIGH | Implémenter nettoyage des credentials en mémoire | 20 min |
| HIGH | Planifier implémentation DTLS pour UDP | 2-4h |
