# ANALYSE COMPARATIVE COMPLETE - R-Type Project

## Vue d'Ensemble des 4 Options

| Option | Description | Difficulte | Temps | Impact RNCP |
|--------|-------------|------------|-------|-------------|
| **1. TLS/SSL** | Chiffrement TCP pour credentials | Moyenne | 2-3 jours | CRITIQUE |
| **2. Gameplay** | Armes multiples, boss, score | Elevee | 5-7 jours | ELEVE |
| **3. VPS Deploy** | Hebergement serveur distant | Facile | 1 jour | MOYEN |
| **4. Documents RNCP** | Comparatifs, audits, matrices | Facile | 1-2 jours | CRITIQUE |

---

## OPTION 1 : TLS/SSL sur TCP

### Probleme Actuel

```
FAILLE DE SECURITE CRITIQUE
===========================

Client -------- TCP PORT 4125 -------- Serveur
         |                         |
         |  LoginMessage {         |
         |    username: "alex"     |  <-- INTERCEPTABLE
         |    password: "secret"   |  <-- EN CLAIR !
         |    email: "a@b.com"     |  <-- EN CLAIR !
         |  }                      |
         |                         |

Risque: Man-in-the-Middle (MITM)
Impact: Credentials compromis
CWE: CWE-319 (Cleartext Transmission)
```

### Solution Proposee

```
APRES TLS 1.3
=============

Client ======== TLS 1.3 ENCRYPTED ======== Serveur
         ||                           ||
         ||  Handshake SSL            ||
         ||  Certificat verifie       ||
         ||  Donnees chiffrees AES    ||
         ||                           ||

Dependances deja presentes:
- OpenSSL dans vcpkg.json
- Boost.ASIO supporte ssl::stream nativement
```

### Implementation Technique

| Fichier | Modifications |
|---------|---------------|
| `TCPAuthServer.cpp/hpp` | Ajouter SSL context serveur, handshake |
| `TCPClient.cpp/hpp` | Ajouter SSL context client, verification |
| `certs/server.crt` | Nouveau certificat auto-signe |
| `certs/server.key` | Nouvelle cle privee |

### Code Exemple (Serveur)

```cpp
// SSL Context setup
boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tls_server);
ssl_ctx.set_options(
    boost::asio::ssl::context::default_workarounds |
    boost::asio::ssl::context::no_sslv2 |
    boost::asio::ssl::context::no_sslv3 |
    boost::asio::ssl::context::single_dh_use
);
ssl_ctx.use_certificate_chain_file("certs/server.crt");
ssl_ctx.use_private_key_file("certs/server.key", boost::asio::ssl::context::pem);

// Utiliser ssl::stream au lieu de tcp::socket
boost::asio::ssl::stream<tcp::socket> ssl_socket(io_ctx, ssl_ctx);
```

### Estimation Effort

| Metrique | Valeur |
|----------|--------|
| Lignes de code | ~300-400 |
| Fichiers modifies | 4 |
| Fichiers nouveaux | 2 (certificats) |
| Complexite | Moyenne |
| Tests requis | Handshake, transmission, erreurs |

### Criteres RNCP Valides

| Critere | Description | Bloc |
|---------|-------------|------|
| **C7** | Reviser protocoles (failles securite) | Bloc 2 |
| **C16** | Integrite des donnees traitees | Bloc 3 |
| **C17** | Solutions techniques reconnues (OpenSSL) | Bloc 3 |
| **C24** | Mesures securite identifiees audit | Bloc 5 |
| **C32** | Normes securite en vigueur | Bloc 6 |

### Avantages / Inconvenients

| Avantages | Inconvenients |
|-----------|---------------|
| Corrige faille CRITIQUE | Invisible en demo |
| OpenSSL deja disponible | Gestion certificats |
| Standards industrie | Latence +1-2ms handshake |
| RNCP securite obligatoire | Debug plus complexe |

---

## OPTION 2 : Gameplay Complet

### Etat Actuel vs Objectif

| Composant | Existant | Manquant |
|-----------|----------|----------|
| **Ennemis** | 5 types (Basic, Tracker, Zigzag, Fast, Bomber) | Formations, boss |
| **Armes** | 1 seule (missile basique) | Spread, Laser, Bouncy |
| **Boss** | Aucun | Phases, patterns d'attaque |
| **UI** | Barre de vie simple | Cooldowns, score, boss HP |
| **Score** | Aucun | Points par kill, combo |

### Systeme d'Armes Multiples

```
ARMES PROPOSEES
===============

+----------+------------------+----------+---------+
|   Arme   |   Comportement   | Cooldown | Degats  |
+----------+------------------+----------+---------+
| Basic    | Tir unique       |   0.3s   |   15    |
| Spread   | 3 tirs en event  |   0.5s   |  3x10   |
| Laser    | Tres rapide      |   0.8s   |   25    |
| Bouncy   | Rebondit murs    |   0.4s   |   12    |
+----------+------------------+----------+---------+

Touches: 1, 2, 3, 4 pour changer d'arme
Affichage: Icone arme active + cooldown visuel
```

### Systeme de Boss

```
BOSS PROPOSEES
==============

+--------------+------------+------+--------+-------------------+
|     Boss     | Apparition |  HP  | Phases | Pattern Principal |
+--------------+------------+------+--------+-------------------+
| Interceptor  |  Vague 5   | 300  |   2    | Tirs rapides      |
| Fortress     |  Vague 10  | 500  |   3    | Barrage + Shields |
| Swarm Master |  Vague 15  | 400  |   3    | Invoque ennemis   |
+--------------+------------+------+--------+-------------------+

Phase 1: Pattern normal
Phase 2: Enrage (vitesse +50%)
Phase 3: Desperate (patterns speciaux)
```

### Systeme de Score

```
POINTS PAR ACTION
=================

- Kill Basic Enemy:    100 pts
- Kill Tracker Enemy:  150 pts
- Kill Zigzag Enemy:   120 pts
- Kill Fast Enemy:     180 pts
- Kill Bomber Enemy:   200 pts
- Kill Boss:          1000 pts x phase
- Combo (kills rapides): x1.5 multiplicateur
- No damage wave:      500 pts bonus
```

### Implementation Technique

| Fichier | Modifications |
|---------|---------------|
| `Protocol.hpp` | +WeaponType enum, +BossState struct, +ScoreUpdate msg |
| `GameWorld.cpp/hpp` | Logique armes, spawn boss, phases, score |
| `GameScene.cpp/hpp` | UI cooldowns, rendu boss, selection arme, score |
| `AccessibilityConfig` | Touches 1-4 pour armes |
| Nouveaux assets | Boss sprites, armes sprites |

### Estimation Effort

| Metrique | Valeur |
|----------|--------|
| Lignes de code | ~1500-2000 |
| Fichiers modifies | 6-8 |
| Fichiers nouveaux | 2-3 (boss sprites, config) |
| Complexite | Elevee |
| Tests requis | Gameplay, balance, collisions |

### Criteres RNCP Valides

| Critere | Description | Bloc |
|---------|-------------|------|
| **C8** | Solution technique creative | Bloc 2 |
| **C10** | Traduire specs en composants | Bloc 3 |
| **C11** | Segmenter problemes complexes | Bloc 3 |
| **C12** | Solutions originales (IA boss) | Bloc 3 |
| **C14** | Structures de donnees adaptees | Bloc 3 |
| **C15** | Interfaces UI/UX optimisees | Bloc 3 |
| **C18** | Code operationnel | Bloc 3 |

### Avantages / Inconvenients

| Avantages | Inconvenients |
|-----------|---------------|
| Impact DEMO maximum | Temps eleve (5-7 jours) |
| Differenciation forte | Risque de bugs gameplay |
| 7 criteres RNCP | Balance difficile |
| Impressionne le jury | Assets graphiques requis |

---

## OPTION 3 : Deploiement VPS

### Architecture Cible

```
ARCHITECTURE VPS
================

                    +-------------+
                    |   INTERNET  |
                    +------+------+
                           |
                    +------v------+
                    |  FIREWALL   |
                    |    (ufw)    |
                    +------+------+
                           |
      +--------------------+--------------------+
      |                    |                    |
+-----v-----+        +-----v-----+        +-----v-----+
| TCP 4125  |        | UDP 4124  |        | UDP 4126  |
|   Auth    |        |   Game    |        |   Voice   |
+-----------+        +-----------+        +-----------+
                           |
                    +------v------+
                    | rtype_server|
                    |  (systemd)  |
                    +-------------+
```

### Configuration Systemd

```ini
# /etc/systemd/system/rtype.service
[Unit]
Description=R-Type Game Server
After=network.target mongodb.service

[Service]
Type=simple
ExecStart=/opt/rtype/rtype_server
Restart=always
RestartSec=5
User=rtype
Group=rtype
WorkingDirectory=/opt/rtype
Environment=MONGODB_URI=mongodb://localhost:27017

[Install]
WantedBy=multi-user.target
```

### Configuration Firewall (UFW)

```bash
# Ouvrir les ports necessaires
sudo ufw allow 4125/tcp  # Auth TCP
sudo ufw allow 4124/udp  # Game UDP
sudo ufw allow 4126/udp  # Voice UDP
sudo ufw allow 22/tcp    # SSH (admin)
sudo ufw enable
```

### Script de Deploiement

```bash
#!/bin/bash
# deploy.sh

# Variables
VPS_USER="rtype"
VPS_HOST="vps.example.com"
BUILD_DIR="./artifacts/server/linux"

# Build
./scripts/build.sh
./scripts/compile.sh

# Deploy
scp ${BUILD_DIR}/rtype_server ${VPS_USER}@${VPS_HOST}:/opt/rtype/
ssh ${VPS_USER}@${VPS_HOST} "sudo systemctl restart rtype"
```

### Estimation Effort

| Metrique | Valeur |
|----------|--------|
| Lignes de code | ~50 (config) |
| Fichiers crees | 3 (service, deploy, firewall) |
| Complexite | Facile |
| Prerequis | VPS avec Ubuntu/Debian |

### Criteres RNCP Valides

| Critere | Description | Bloc |
|---------|-------------|------|
| **C30** | Technologies hebergement adaptees | Bloc 6 |
| **C31** | Automatisation deploiement | Bloc 6 |
| **C32** | Normes securite reseau | Bloc 6 |

### Avantages / Inconvenients

| Avantages | Inconvenients |
|-----------|---------------|
| Demo accessible partout | Cout VPS mensuel |
| Facile a faire | Latence reseau |
| Impressionne le jury | Dependance externe |
| Automatisable | Maintenance requise |

---

## OPTION 4 : Documents RNCP Manquants

### Documents a Creer

| Document | Critere | Priorite |
|----------|---------|----------|
| `TECHNOLOGICAL_COMPARISON.md` | C6 | CRITIQUE |
| `SECURITY_AUDIT.md` | C7 | CRITIQUE |
| `PROTOTYPE_EVALUATION.md` | C8 | HAUTE |
| `ARCHITECTURE_DECISIONS.md` | C9 | MOYENNE |

### 4.1 Comparatif Technologique (C6)

```markdown
# Comparatif SFML vs SDL2

| Critere          | SFML              | SDL2              | Choix  |
|------------------|-------------------|-------------------|--------|
| Performance      | Bonne             | Excellente        | SDL2   |
| Portabilite      | Windows/Linux/Mac | 10+ plateformes   | SDL2   |
| API              | C++ moderne       | C avec wrappers   | SFML   |
| Shaders          | Natif             | Externe (OpenGL)  | SFML   |
| Audio            | Integre           | SDL_mixer requis  | SFML   |
| Documentation    | Excellente        | Bonne             | SFML   |
| Communaute       | Active            | Tres active       | SDL2   |
| Licence          | zlib              | zlib              | Egal   |

Decision: SDL2 par defaut pour performance et portabilite,
          SFML disponible via plugin pour shaders avances.
```

### 4.2 Audit de Securite (C7)

```markdown
# Audit de Securite R-Type

## Vulnerabilites Identifiees

| CWE     | Vulnerabilite              | Localisation         | Risque    |
|---------|----------------------------|----------------------|-----------|
| CWE-338 | PRNG non-cryptographique   | SessionManager.hpp   | CRITIQUE  |
| CWE-319 | Transmission en clair      | TCPAuthServer.cpp    | CRITIQUE  |
| CWE-120 | Buffer overflow potentiel  | Protocol.hpp memcpy  | HAUTE     |
| CWE-306 | HeartBeat sans auth        | UDPServer.cpp        | MOYENNE   |

## Mitigations Implementees

- Validation des buffers (len < WIRE_SIZE)
- Null termination des strings
- Session timeouts (30s inactivite)
- Thread-safe session management (std::mutex)

## Mitigations a Implementer

1. Remplacer mt19937_64 par RAND_bytes() (OpenSSL)
2. Ajouter TLS 1.3 sur TCP:4125
3. Rate limiting sur HeartBeat
```

### 4.3 Evaluation des Prototypes (C8)

```markdown
# Matrice d'Evaluation des Prototypes

| Prototype        | Etat     | Avantages              | Inconvenients          |
|------------------|----------|------------------------|------------------------|
| SFML Backend     | Complet  | Shaders, API simple    | Moins portable         |
| SDL2 Backend     | Complet  | Performance, 10+ OS    | Post-processing stub   |
| Blob-ECS         | Pret     | 51.3M ops/s            | Non integre gameplay   |
| Hexagonal Arch   | Complet  | Testabilite, modularite| 110+ fichiers          |
| Accessibility    | 40%      | WCAG 2.1 refs          | Pas de screen reader   |

## Criteres de Selection

- Performance: SDL2 > SFML
- Maintenabilite: Hexagonal
- Evolutivite: ECS prepare mais non requis
```

### Estimation Effort

| Metrique | Valeur |
|----------|--------|
| Temps total | 1-2 jours |
| Documents | 4 fichiers .md |
| Complexite | Facile (redaction) |

### Criteres RNCP Valides

| Critere | Description | Bloc |
|---------|-------------|------|
| **C6** | Etude comparative technologies | Bloc 2 |
| **C7** | Failles de securite benchmarkees | Bloc 2 |
| **C8** | Travaux de prototypages | Bloc 2 |
| **C9** | Architecture bonnes pratiques | Bloc 2 |

### Avantages / Inconvenients

| Avantages | Inconvenients |
|-----------|---------------|
| Impact RNCP direct | Pas visible en demo |
| Facile et rapide | Redaction fastidieuse |
| Couvre 4 criteres | Necessite reflexion |
| Obligatoire pour soutenance | - |

---

## MATRICE DE DECISION COMPLETE

```
+---------------------+----------+----------+----------+----------+
|   Critere (poids)   |  TLS/SSL | Gameplay |   VPS    |   Docs   |
+---------------------+----------+----------+----------+----------+
| Criteres RNCP (35%) |    5     |    7     |    3     |    4     |
|                     | ======   | ======== | ====     | =====    |
+---------------------+----------+----------+----------+----------+
| Securite (25%)      |   10/10  |   3/10   |   6/10   |   8/10   |
|                     | ======== | ===      | ======   | ========  |
+---------------------+----------+----------+----------+----------+
| Impact Demo (20%)   |   2/10   |  10/10   |   7/10   |   1/10   |
|                     | ==       | ======== | =======  | =        |
+---------------------+----------+----------+----------+----------+
| Temps/Effort (10%)  |   7/10   |   4/10   |  10/10   |   9/10   |
|                     | =======  | ====     | ======== | =========|
+---------------------+----------+----------+----------+----------+
| Differenciation(10%)|   6/10   |  10/10   |   4/10   |   3/10   |
|                     | ======   | ======== | ====     | ===      |
+---------------------+----------+----------+----------+----------+
|   SCORE TOTAL       |   6.55   |   6.85   |   5.85   |   5.30   |
+---------------------+----------+----------+----------+----------+
```

### Calcul Detaille

| Option | RNCP (35%) | Secu (25%) | Demo (20%) | Effort (10%) | Diff (10%) | **TOTAL** |
|--------|------------|------------|------------|--------------|------------|-----------|
| TLS    | 5*0.35=1.75| 10*0.25=2.5| 2*0.20=0.4 | 7*0.10=0.7   | 6*0.10=0.6 | **5.95**  |
| Gameplay| 7*0.35=2.45| 3*0.25=0.75| 10*0.20=2.0| 4*0.10=0.4   | 10*0.10=1.0| **6.60**  |
| VPS    | 3*0.35=1.05| 6*0.25=1.5 | 7*0.20=1.4 | 10*0.10=1.0  | 4*0.10=0.4 | **5.35**  |
| Docs   | 4*0.35=1.40| 8*0.25=2.0 | 1*0.20=0.2 | 9*0.10=0.9   | 3*0.10=0.3 | **4.80**  |

---

## RECOMMANDATION FINALE

```
+------------------------------------------------------------------------+
|                      ORDRE DE PRIORITE RECOMMANDE                       |
+------------------------------------------------------------------------+
|                                                                         |
|  1. PRIORITE 1 : Documents RNCP + TLS (en parallele)                   |
|     |-- Raison: Obligatoire pour soutenance + faille CRITIQUE          |
|     |-- RNCP: C6, C7, C8, C16, C24, C32                                |
|     |-- Temps: 2-3 jours                                                |
|                                                                         |
|  2. PRIORITE 2 : Gameplay                                               |
|     |-- Raison: Maximum impact demo + differenciation                   |
|     |-- RNCP: C8, C10, C11, C12, C15, C18                              |
|     |-- Temps: 5-7 jours                                                |
|                                                                         |
|  3. PRIORITE 3 : VPS                                                    |
|     |-- Raison: A faire J-1 pour la demo finale                         |
|     |-- RNCP: C30, C31                                                  |
|     |-- Temps: 1 jour                                                   |
|                                                                         |
+------------------------------------------------------------------------+
```

---

## PLANNING SUGGERE

```
+----------+----------------------------------------------------------------+
|   Jour   |                          Taches                                |
+----------+----------------------------------------------------------------+
|  Jour 1  | Docs: TECHNOLOGICAL_COMPARISON.md + SECURITY_AUDIT.md         |
|  Jour 2  | Docs: PROTOTYPE_EVALUATION.md + TLS: Certificats              |
|  Jour 3  | TLS: SSL context serveur + client + tests                     |
+----------+----------------------------------------------------------------+
|  Jour 4  | Gameplay: WeaponType enum, Protocol.hpp                       |
|  Jour 5  | Gameplay: GameWorld armes multiples                           |
|  Jour 6  | Gameplay: UI cooldowns + selection armes                       |
|  Jour 7  | Gameplay: Boss basique (spawn + HP bar)                        |
|  Jour 8  | Gameplay: Boss phases + patterns                               |
|  Jour 9  | Gameplay: Score system + polish                                |
|  Jour 10 | Gameplay: Tests + bugfix                                       |
+----------+----------------------------------------------------------------+
|  Jour 11 | VPS: Deploiement + firewall + systemd                          |
+----------+----------------------------------------------------------------+
```

---

## IMPACT RNCP PAR OPTION

### Recap Criteres Couverts

| Bloc | Critere | TLS | Gameplay | VPS | Docs |
|------|---------|-----|----------|-----|------|
| Bloc 2 | C6 - Etude comparative | | | | X |
| Bloc 2 | C7 - Securite failles | X | | | X |
| Bloc 2 | C8 - Prototypages | | X | | X |
| Bloc 3 | C10 - Traduire specs | | X | | |
| Bloc 3 | C11 - Segmenter problemes | | X | | |
| Bloc 3 | C12 - Solutions originales | | X | | |
| Bloc 3 | C14 - Structures donnees | | X | | |
| Bloc 3 | C15 - UI/UX | | X | | |
| Bloc 3 | C16 - Integrite donnees | X | | | |
| Bloc 3 | C17 - Solutions reconnues | X | | | |
| Bloc 3 | C18 - Code operationnel | | X | | |
| Bloc 5 | C24 - Mesures securite | X | | | |
| Bloc 6 | C30 - Hebergement | | | X | |
| Bloc 6 | C31 - Automatisation | | | X | |
| Bloc 6 | C32 - Normes securite | X | | X | |

### Total Criteres par Option

| Option | Criteres Uniques | Criteres Partages |
|--------|------------------|-------------------|
| TLS/SSL | 5 | 1 (C32) |
| Gameplay | 7 | 0 |
| VPS | 2 | 1 (C32) |
| Documents | 3 | 1 (C7) |

---

## RISQUES ET MITIGATIONS

| Option | Risque Principal | Mitigation |
|--------|------------------|------------|
| TLS | Complexite certificats | Utiliser Let's Encrypt ou auto-signe |
| Gameplay | Bugs de balance | Tests incrementaux, valeurs ajustables |
| VPS | Latence reseau | Choisir VPS proche geographiquement |
| Docs | Temps de redaction | Templates pre-definis |

---

## CONCLUSION

Pour maximiser le score RNCP tout en ayant une demo impressionnante:

1. **Commencer par Documents + TLS** (obligatoire, corrige faille critique)
2. **Enchainer avec Gameplay** (differenciation, impact demo)
3. **Finir par VPS** (J-1, facile, impressionne jury)

**Score RNCP potentiel apres toutes les options: 95/100+**

---

*Document genere le 14/01/2026*
*Projet: R-Type Multiplayer - EPITECH*
