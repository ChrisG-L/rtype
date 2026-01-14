# :material-shield-lock: Rapport du Commandant Sécurité

<div class="report-header" markdown>

!!! danger "ALERTE CRITIQUE"

    | Attribut | Valeur |
    |:---------|:-------|
    | **Date** | 2025-11-26 |
    | **Score** | :material-close-circle:{ .status-critical } **3/10** |
    | **Vulnérabilités Critiques** | **5** |
    | **Statut** | :material-alert-octagon: BLOCAGE DÉPLOIEMENT |

</div>

---

## :material-alert-octagon: Résumé Exécutif

!!! danger "VERDICT: NE PAS DÉPLOYER"

    Le projet présente **5 vulnérabilités de sécurité critiques** qui empêchent
    tout déploiement en environnement de production ou accessible publiquement.

```mermaid
%%{init: {'theme': 'dark', 'themeVariables': { 'pie1': '#f44336', 'pie2': '#ff9800', 'pie3': '#4caf50'}}}%%
pie showData
    title Répartition des Vulnérabilités
    "Critiques (P0)" : 5
    "Élevées (P1)" : 4
    "Moyennes (P2)" : 3
```

---

## :material-skull-crossbones: Vulnérabilités Critiques (P0)

### 1. :material-database-alert: MongoDB Credentials Exposées

!!! danger "CRITIQUE - CWE-798: Use of Hard-coded Credentials"

    **Fichier**: `src/server/infrastructure/boostrap/GameBootstrap.hpp:30`

    **Code Vulnérable**:
    ```cpp
    std::string mongoURI = "mongodb+srv://dbUser:root@rtypehome.qxzb27g.mongodb.net/";
    ```

    **Impact**:

    - :material-skull: Accès complet non autorisé à la base de données
    - :material-skull: Credentials visibles dans l'historique Git
    - :material-skull: Compromission de toutes les données utilisateurs

    **CVSS Score**: 9.8 (Critique)

??? success "Solution Recommandée"

    ```cpp
    // Utiliser des variables d'environnement
    #include <cstdlib>

    std::string getMongoURI() {
        const char* uri = std::getenv("MONGODB_URI");
        if (!uri) {
            throw std::runtime_error("MONGODB_URI environment variable not set");
        }
        return std::string(uri);
    }
    ```

    **Actions**:

    1. Supprimer les credentials du code
    2. Utiliser un fichier `.env` (non versionné)
    3. Configurer les variables d'environnement sur le serveur
    4. Faire tourner les credentials MongoDB

---

### 2. :material-key-alert: Hachage de Mot de Passe Sans Salt

!!! danger "CRITIQUE - CWE-916: Use of Password Hash With Insufficient Computational Effort"

    **Fichier**: `src/server/domain/value_objects/user/utils/PasswordUtils.cpp`

    **Code Vulnérable**:
    ```cpp
    std::string hashPassword(std::string password) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
               password.length(), hash);
        // Conversion hex...
        return ss.str();
    }
    ```

    **Impact**:

    - :material-skull: Vulnérable aux attaques par rainbow tables
    - :material-skull: Même mot de passe = même hash (déterministe)
    - :material-skull: Compromission massive en cas de vol de BDD

    **CVSS Score**: 9.1 (Critique)

??? success "Solution Recommandée"

    ```cpp
    // Utiliser Argon2id (recommandé OWASP)
    #include <argon2.h>

    std::string hashPassword(const std::string& password) {
        // Générer un salt aléatoire
        std::array<uint8_t, 16> salt;
        RAND_bytes(salt.data(), salt.size());

        // Paramètres Argon2id recommandés
        uint32_t t_cost = 3;        // Itérations
        uint32_t m_cost = 65536;    // 64 MB mémoire
        uint32_t parallelism = 4;

        std::array<uint8_t, 32> hash;
        argon2id_hash_raw(t_cost, m_cost, parallelism,
                          password.c_str(), password.length(),
                          salt.data(), salt.size(),
                          hash.data(), hash.size());

        // Retourner salt + hash encodé
        return encode(salt, hash);
    }
    ```

---

### 3. :material-console: Mots de Passe dans les Logs

!!! danger "CRITIQUE - CWE-532: Insertion of Sensitive Information into Log File"

    **Fichier**: `src/server/application/use_cases/auth/Login.cpp:21`

    **Code Vulnérable**:
    ```cpp
    std::cout << "password: " << password << std::endl;
    ```

    **Impact**:

    - :material-skull: Tous les mots de passe exposés dans les logs
    - :material-skull: Logs persistés dans `/logs/server.log`
    - :material-skull: Accès aux credentials pour quiconque lit les logs

    **CVSS Score**: 8.6 (Élevé)

??? success "Solution Recommandée"

    ```cpp
    // SUPPRIMER CETTE LIGNE IMMÉDIATEMENT
    // std::cout << "password: " << password << std::endl;

    // Pour le debug, logger seulement les métadonnées
    _logger->debug("Login attempt for user: {}", email);
    ```

---

### 4. :material-bug: Logique d'Enregistrement Inversée

!!! danger "CRITIQUE - CWE-287: Improper Authentication"

    **Fichier**: `src/server/application/use_cases/auth/Register.cpp:18-28`

    **Code Vulnérable**:
    ```cpp
    auto playerOptByName = _userRepository->findByName(email);
    if (!playerOptByName.has_value()) {  // INVERSÉ!
        std::cout << "Name already exist" << std::endl;
        return;
    }

    auto playerOptByEmail = _userRepository->findByEmail(email);
    if (!playerOptByEmail.has_value()) {  // INVERSÉ!
        std::cout << "Email already exist" << std::endl;
        return;
    }
    ```

    **Impact**:

    - :material-skull: Bypass complet du contrôle d'unicité
    - :material-skull: Création de comptes dupliqués
    - :material-skull: Messages d'erreur trompeurs

    **CVSS Score**: 8.2 (Élevé)

??? success "Solution Recommandée"

    ```cpp
    // La logique CORRECTE:
    auto playerOptByName = _userRepository->findByName(username);
    if (playerOptByName.has_value()) {  // SANS le !
        _logger->warn("Registration failed: username already exists");
        throw UsernameAlreadyExistsException(username);
    }

    auto playerOptByEmail = _userRepository->findByEmail(email);
    if (playerOptByEmail.has_value()) {  // SANS le !
        _logger->warn("Registration failed: email already exists");
        throw EmailAlreadyExistsException(email);
    }
    ```

---

### 5. :material-lock-alert: Vérification de Mot de Passe Cassée

!!! danger "CRITIQUE - CWE-287: Improper Authentication"

    **Fichier**: `src/server/domain/value_objects/user/Password.cpp:26`

    **Code Vulnérable**:
    ```cpp
    bool Password::verify(std::string password) {
        if (utils::hashPassword(password) == password)  // COMPARE AVEC LE MOT DE PASSE EN CLAIR!
            return true;
        return false;
    }
    ```

    **Impact**:

    - :material-skull: Authentification **toujours échoue**
    - :material-skull: Compare hash avec le mot de passe en clair (jamais égal)
    - :material-skull: Aucun utilisateur ne peut se connecter

    **CVSS Score**: 9.8 (Critique)

??? success "Solution Recommandée"

    ```cpp
    bool Password::verify(const std::string& password) const {
        // Comparer le hash du password fourni avec le hash stocké
        return utils::hashPassword(password) == _passwordHash;
    }
    ```

---

## :material-alert: Vulnérabilités Élevées (P1)

### 6. :material-ethernet: Communication Réseau ~~Non Chiffrée~~ ✅ CORRIGÉ

!!! success "CORRIGÉ - TLS 1.2+ implémenté sur TCP 4125"

    **Fichiers modifiés**: `TCPAuthServer.cpp`, `TCPClient.cpp`

    Le canal TCP d'authentification (port 4125) est maintenant chiffré avec TLS 1.2+.
    Cipher suites modernes : ECDHE + AES-GCM / ChaCha20-Poly1305.

    **Note**: UDP gameplay (4124) et voice (4126) restent en clair (acceptable pour données temps réel non sensibles).

### 7. :material-text-box-search: Absence de Validation des Inputs

!!! warning "ÉLEVÉ - CWE-20: Improper Input Validation"

    **Fichier**: `CommandParser.cpp`

    Pas de validation du format, longueur ou contenu des commandes reçues.

    **Impact**: Injection de commandes, buffer overflow potentiel.

### 8. :material-server: Rsync Sans Authentification

!!! warning "ÉLEVÉ - Configuration CI/CD"

    **Fichier**: `ci_cd/docker/rsyncd.conf`

    ```conf
    auth users =
    secrets file =
    ```

    **Impact**: Upload de code malveillant dans le pipeline.

### 9. :material-speedometer: Absence de Rate Limiting

!!! warning "ÉLEVÉ - CWE-307: Improper Restriction of Excessive Authentication Attempts"

    Aucune limite sur les tentatives d'authentification.

    **Impact**: Attaques par force brute sans restriction.

---

## :material-check-all: Bonnes Pratiques Appliquées

| Aspect | État | Détails |
|:-------|:----:|:--------|
| Logging centralisé (spdlog) | :material-check-circle:{ .status-ok } | 12 loggers configurés |
| Architecture hexagonale | :material-check-circle:{ .status-ok } | Séparation des responsabilités |
| Gestion dépendances (vcpkg) | :material-check-circle:{ .status-ok } | Versions pinées |
| Async I/O (Boost.Asio) | :material-check-circle:{ .status-ok } | Non-bloquant |
| Smart pointers | :material-check-circle:{ .status-ok } | Pas de fuites mémoire |

---

## :material-clipboard-check: Plan de Remédiation

### Priorité Immédiate (P0) - 24-48h

```
[ ] 1. Migrer MongoDB credentials vers variables d'environnement
[ ] 2. Implémenter Argon2id pour le hachage de mots de passe
[ ] 3. Supprimer tous les logs de mots de passe
[ ] 4. Corriger la logique de Register.cpp
[ ] 5. Corriger Password::verify()
```

### Priorité Haute (P1) - 1 semaine

```
[x] 6. ~~Implémenter TLS pour TCP~~ ✅ FAIT (TLS 1.2+ sur port 4125)
[ ] 7. Ajouter validation des inputs dans CommandParser
[ ] 8. Sécuriser rsync avec authentification
[ ] 9. Implémenter rate limiting
```

### Priorité Moyenne (P2) - 2 semaines

```
[ ] 10. Ajouter tests de sécurité automatisés
[ ] 11. Configurer SAST dans CI/CD (Cppcheck, SonarQube)
[ ] 12. Audit des dépendances (Dependabot)
[ ] 13. Scanner les images Docker (Trivy)
```

---

## :material-chart-line: Métriques de Sécurité

| Métrique | Valeur | Cible |
|:---------|:------:|:-----:|
| Vulnérabilités Critiques | **5** | 0 |
| Vulnérabilités Élevées | **4** | 0 |
| Score OWASP | **F** | A |
| Couverture Tests Sécurité | **0%** | 80% |
| Secrets dans Code | **1** | 0 |

---

<div class="report-footer" markdown>

!!! quote "Devise du Commandant Sécurité"

    *"La sécurité n'est pas une feature, c'est une fondation."*

:material-shield-lock: **Rapport généré par le Commandant Sécurité - Army2077**

</div>

---

[:material-arrow-left: Retour aux Commandants](../index.md){ .md-button }
[:material-shield-star: Voir Rapport Général](../../general/latest.md){ .md-button .md-button--primary }
