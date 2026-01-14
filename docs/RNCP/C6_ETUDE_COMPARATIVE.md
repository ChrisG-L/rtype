# C6 - Étude Comparative des Solutions de Sécurité

## Contexte du Projet

Le projet R-Type est un jeu multijoueur en temps réel nécessitant :
- **Authentification sécurisée** des joueurs (TCP port 4125)
- **Communication temps réel** pour le gameplay (UDP port 4124)
- **Chat vocal** entre joueurs (UDP port 4126)

Ce document présente l'étude comparative des solutions de sécurité évaluées pour le canal d'authentification TCP.

---

## 1. Comparatif des Protocoles de Chiffrement

### 1.1 Options Évaluées

| Critère | TLS 1.2 | TLS 1.3 | DTLS 1.2 | Chiffrement Custom |
|---------|---------|---------|----------|-------------------|
| **Maturité** | Stable (2008) | Récent (2018) | Stable (2012) | Risqué |
| **Latence Handshake** | 2 RTT | 1 RTT | 2 RTT | Variable |
| **Support OpenSSL** | Excellent | Bon | Moyen | N/A |
| **Support Boost.ASIO** | Natif | Natif | Limité | Manuel |
| **Cipher Suites** | Large choix | Restreint (sécurisé) | Large choix | Manuel |
| **Forward Secrecy** | Optionnel | Obligatoire | Optionnel | Manuel |
| **Complexité Impl.** | Faible | Faible | Moyenne | Élevée |

### 1.2 Analyse Détaillée

#### TLS 1.2 (Solution Retenue)

**Avantages :**
- Support universel (OpenSSL, Boost.ASIO ssl::stream)
- Documentation abondante
- Compatibilité avec tous les systèmes d'exploitation
- Cipher suites modernes disponibles (ECDHE + AES-GCM)
- Intégration transparente avec `boost::asio::ssl`

**Inconvénients :**
- Handshake en 2 RTT (vs 1 RTT pour TLS 1.3)
- Cipher suites obsolètes disponibles (nécessite configuration stricte)

**Configuration Implémentée :**
```cpp
// Désactiver protocoles obsolètes
_sslContext.set_options(
    ssl::context::no_sslv2 |
    ssl::context::no_sslv3 |
    ssl::context::no_tlsv1 |
    ssl::context::no_tlsv1_1
);

// Forcer TLS 1.2 minimum
SSL_CTX_set_min_proto_version(_sslContext.native_handle(), TLS1_2_VERSION);

// Cipher suites AEAD avec forward secrecy
SSL_CTX_set_cipher_list(_sslContext.native_handle(),
    "ECDHE-ECDSA-AES256-GCM-SHA384:"
    "ECDHE-RSA-AES256-GCM-SHA384:"
    "ECDHE-ECDSA-CHACHA20-POLY1305:"
    "ECDHE-RSA-CHACHA20-POLY1305"
);
```

#### TLS 1.3 (Alternative Évaluée)

**Avantages :**
- Handshake en 1 RTT (0-RTT possible mais risqué)
- Cipher suites obligatoirement sécurisées
- Forward secrecy obligatoire

**Inconvénients :**
- Support OpenSSL 1.1.1+ requis (non garanti sur toutes les plateformes)
- Moins de documentation pour cas d'usage gaming
- Pas de gain significatif pour authentification (connexion unique)

**Raison du Rejet :**
Le gain de latence (1 RTT vs 2 RTT) est négligeable pour une authentification unique au lancement. TLS 1.2 offre une meilleure compatibilité cross-platform.

#### DTLS 1.2 (Non Retenu)

**Avantages :**
- Chiffrement pour UDP
- Gestion de la perte de paquets

**Inconvénients :**
- Support Boost.ASIO limité (implémentation manuelle requise)
- Overhead significatif pour gameplay temps réel
- Non adapté au canal TCP d'authentification

**Raison du Rejet :**
DTLS est conçu pour UDP, pas TCP. Le gameplay UDP (port 4124) ne nécessite pas de chiffrement car :
- Données non sensibles (positions, états)
- Latence critique (< 50ms)
- Token de session validé côté serveur

#### Chiffrement Custom (Non Retenu)

**Raison du Rejet Immédiat :**
- Violation du principe "Don't Roll Your Own Crypto"
- Risque élevé de vulnérabilités
- Aucun audit possible
- Maintenance complexe

---

## 2. Comparatif des Générateurs de Nombres Aléatoires

### 2.1 Options Évaluées

| Critère | mt19937_64 | std::random_device | OpenSSL RAND_bytes | /dev/urandom |
|---------|------------|-------------------|-------------------|--------------|
| **Type** | PRNG | CSPRNG (variable) | CSPRNG | CSPRNG |
| **Prédictibilité** | Oui (624 outputs) | Non | Non | Non |
| **Performance** | Très rapide | Variable | Rapide | Rapide |
| **Portabilité** | Excellente | Variable | Excellente | Linux only |
| **Entropie** | Faible seed | Système | Système | Système |
| **Standards** | Aucun | C++11 | FIPS 140-2 | POSIX |

### 2.2 Analyse Détaillée

#### mt19937_64 (Mersenne Twister - Ancienne Implémentation)

**Problème Identifié (CWE-338) :**
```cpp
// AVANT - Vulnérable
std::random_device _rd;
std::mt19937_64 _rng{_rd()};  // Seed unique de 64 bits

SessionToken SessionManager::generateToken() {
    for (size_t i = 0; i < TOKEN_SIZE; i += 8) {
        uint64_t val = _rng();  // Prédictible après 624 outputs
        std::memcpy(token.bytes + i, &val, remaining);
    }
}
```

**Vulnérabilité :**
- État interne de 19968 bits (624 × 32 bits)
- Après observation de 624 outputs, l'état peut être reconstitué
- Un attaquant peut prédire les futurs tokens

**Impact :**
- Session hijacking possible
- Usurpation d'identité

#### OpenSSL RAND_bytes (Solution Retenue)

**Implémentation :**
```cpp
#include <openssl/rand.h>

SessionToken SessionManager::generateToken() {
    SessionToken token;

    // CSPRNG certifié FIPS 140-2
    if (RAND_bytes(token.bytes, TOKEN_SIZE) != 1) {
        throw std::runtime_error("CSPRNG failure");
    }

    return token;
}
```

**Avantages :**
- Certifié FIPS 140-2 (standard gouvernemental US)
- Utilise l'entropie système (/dev/urandom, CryptGenRandom)
- Reseeding automatique
- Déjà lié au projet (OpenSSL pour TLS)

**Garanties :**
- 256 bits d'entropie par token
- Probabilité de collision : 2^-128 (négligeable)
- Résistance aux attaques par prédiction

#### std::random_device (Non Retenu)

**Problème :**
- Comportement non garanti par le standard C++
- Sur certaines implémentations (MinGW), utilise mt19937 en fallback
- Pas de garantie CSPRNG

#### /dev/urandom (Non Retenu)

**Problème :**
- Linux/macOS uniquement
- Nécessite gestion fichier manuelle
- OpenSSL abstrait déjà cette complexité

---

## 3. Comparatif des Cipher Suites

### 3.1 Cipher Suites Sélectionnées

| Cipher Suite | Échange | Chiffrement | MAC | Score |
|--------------|---------|-------------|-----|-------|
| ECDHE-ECDSA-AES256-GCM-SHA384 | ECDHE | AES-256-GCM | SHA384 | A+ |
| ECDHE-RSA-AES256-GCM-SHA384 | ECDHE | AES-256-GCM | SHA384 | A+ |
| ECDHE-ECDSA-CHACHA20-POLY1305 | ECDHE | ChaCha20 | Poly1305 | A+ |
| ECDHE-RSA-CHACHA20-POLY1305 | ECDHE | ChaCha20 | Poly1305 | A+ |

### 3.2 Justification des Choix

**ECDHE (Elliptic Curve Diffie-Hellman Ephemeral) :**
- Forward secrecy : compromission de la clé privée ne compromet pas les sessions passées
- Performance : plus rapide que DHE classique
- Sécurité : courbes P-256, P-384 recommandées par NIST

**AES-GCM vs ChaCha20-Poly1305 :**
- AES-GCM : accéléré matériellement (AES-NI) sur x86
- ChaCha20 : meilleure performance sur ARM (mobile, Raspberry Pi)
- Les deux sont AEAD (Authenticated Encryption with Associated Data)

**Cipher Suites Exclues :**
```
# Rejetées - Pas de forward secrecy
RSA-AES256-GCM-SHA384

# Rejetées - CBC mode vulnérable (BEAST, Lucky13)
ECDHE-RSA-AES256-CBC-SHA384

# Rejetées - MD5/SHA1 obsolètes
ECDHE-RSA-AES256-SHA

# Rejetées - 3DES obsolète
ECDHE-RSA-DES-CBC3-SHA
```

---

## 4. Matrice de Décision

### 4.1 Critères Pondérés

| Critère | Poids | TLS 1.2 | TLS 1.3 | DTLS | Custom |
|---------|-------|---------|---------|------|--------|
| Sécurité | 30% | 9/10 | 10/10 | 8/10 | 3/10 |
| Compatibilité | 25% | 10/10 | 7/10 | 5/10 | 10/10 |
| Performance | 20% | 8/10 | 9/10 | 7/10 | 6/10 |
| Complexité Impl. | 15% | 9/10 | 8/10 | 5/10 | 2/10 |
| Maintenance | 10% | 9/10 | 8/10 | 6/10 | 3/10 |
| **Score Final** | 100% | **9.05** | 8.45 | 6.35 | 4.60 |

### 4.2 Décision Finale

**Solution Retenue : TLS 1.2 avec OpenSSL RAND_bytes**

**Justification :**
1. Meilleur équilibre sécurité/compatibilité
2. Support natif Boost.ASIO (`ssl::stream<tcp::socket>`)
3. OpenSSL déjà présent dans le projet (dépendance vcpkg)
4. CSPRNG certifié FIPS 140-2
5. Documentation et support communautaire excellents

---

## 5. Conformité OWASP Top 10:2025

### 5.1 Mapping des Vulnérabilités Traitées

| OWASP 2025 | CWE Associés | Statut Projet |
|------------|--------------|---------------|
| A04:2025 - Cryptographic Failures | CWE-319, CWE-338 | ✅ Corrigé (TLS 1.2 + CSPRNG) |
| A07:2025 - Authentication Failures | CWE-287, CWE-384 | ✅ Tokens sécurisés |
| A02:2025 - Security Misconfiguration | CWE-16 | ✅ Cipher suites restreintes |

### 5.2 Justification OWASP

**A04:2025 - Cryptographic Failures** (32 CWEs couverts) :
- Transmission en clair → TLS 1.2 avec AEAD
- PRNG faible → OpenSSL RAND_bytes (FIPS 140-2)

**A07:2025 - Authentication Failures** (36 CWEs couverts) :
- Tokens de session 256 bits
- Pas de prédictibilité
- Expiration configurable

---

## 6. Références

### Standards et Spécifications
- [RFC 5246 - TLS 1.2](https://tools.ietf.org/html/rfc5246)
- [RFC 8446 - TLS 1.3](https://tools.ietf.org/html/rfc8446)
- [FIPS 140-2 - Security Requirements for Cryptographic Modules](https://csrc.nist.gov/publications/detail/fips/140/2/final)
- [CWE-319 - Cleartext Transmission of Sensitive Information](https://cwe.mitre.org/data/definitions/319.html)
- [CWE-338 - Use of Cryptographically Weak PRNG](https://cwe.mitre.org/data/definitions/338.html)

### OWASP
- [OWASP Top 10:2025](https://owasp.org/Top10/2025/)
- [OWASP ASVS v5.0](https://owasp.org/www-project-application-security-verification-standard/)
- [OWASP Cheat Sheet Series](https://cheatsheetseries.owasp.org/)

### Documentation Technique
- [OpenSSL RAND_bytes](https://www.openssl.org/docs/man3.0/man3/RAND_bytes.html)
- [Boost.ASIO SSL](https://www.boost.org/doc/libs/1_87_0/doc/html/boost_asio/overview/ssl.html)
- [Mozilla SSL Configuration Generator](https://ssl-config.mozilla.org/)
- [NIST SP 800-52 Rev. 2 - TLS Guidelines](https://csrc.nist.gov/publications/detail/sp/800-52/rev-2/final)

---

**Document rédigé le :** 2025-01-14
**Version :** 1.1
**Critère RNCP :** C6 - Étude comparative documentée
**Conformité :** OWASP Top 10:2025, NIST SP 800-52, FIPS 140-2
