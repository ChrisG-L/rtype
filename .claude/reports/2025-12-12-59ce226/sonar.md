# Rapport SonarQube - Résumé

Commit: 59ce226 | Branche: main | Date: 2025-12-12 16:59

Mode: **Filtré sur 15 fichiers du diff**

Fichiers analysés:
- src/client/src/network/TCPClient.cpp
- src/client/src/network/UDPClient.cpp
- src/common/protocol/Protocol.hpp
- src/server/application/use_cases/auth/Login.cpp
- src/server/application/use_cases/auth/Register.cpp
- src/server/application/use_cases/player/Move.cpp
- src/server/include/application/ports/out/persistence/IUserRepository.hpp
- src/server/include/application/use_cases/auth/Login.hpp
- src/server/include/application/use_cases/auth/Register.hpp
- src/server/include/application/use_cases/player/Move.hpp
- src/server/include/infrastructure/adapters/in/network/UDPServer.hpp
- src/server/include/infrastructure/adapters/out/persistence/MongoDBUserRepository.hpp
- src/server/infrastructure/adapters/in/network/UDPServer.cpp
- src/server/infrastructure/bootstrap/GameBootstrap.hpp
- src/server/main.cpp

*(4 issues sur les fichiers du diff, 96 sur tout le projet)*

---

## Vue d'ensemble

### Par sévérité

| Sévérité | Count |
|----------|-------|
| Blocker | 0 |
| Critical | 3 |
| Major | 1 |
| Minor | 0 |
| Info | 0 |
| **Total** | **4** |

### Par catégorie

| Catégorie | Count |
|-----------|-------|
| Security | 0 |
| Reliability | 0 |
| Maintainability | 4 |

---

## Issues Bloquantes (0)

Aucune issue bloquante.

---

## Issues Critiques (3)

### 1. Refactor this function to reduce its Cognitive Complexity from 29 to the 25 allowed.

- **Fichier**: src/client/src/network/TCPClient.cpp:170
- **Règle**: cpp:S3776
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 9min

### 2. Refactor this code to not nest more than 3 if|for|do|while|switch statements.

- **Fichier**: src/client/src/network/TCPClient.cpp:193
- **Règle**: cpp:S134
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 3. Refactor this code to not nest more than 3 if|for|do|while|switch statements.

- **Fichier**: src/client/src/network/TCPClient.cpp:199
- **Règle**: cpp:S134
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

---

## Issues Majeures (1)

### 1. Fill this compound statement, remove it, or add a nested comment explaining why it is empty.

- **Fichier**: src/client/src/network/UDPClient.cpp:169
- **Règle**: cpp:S108
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

---

## Fichiers les plus impactés (top 20)

| Fichier | Blocker | Critical | Major | Minor | Total |
|---------|---------|----------|-------|-------|-------|
| src/client/src/network/TCPClient.cpp | 0 | 3 | 0 | 0 | 3 |
| src/client/src/network/UDPClient.cpp | 0 | 0 | 1 | 0 | 1 |

---

## Règles les plus violées (top 20)

| Règle | Description | Count |
|-------|-------------|-------|
| cpp:S134 | cpp:S134 | 2 |
| cpp:S3776 | cpp:S3776 | 1 |
| cpp:S108 | cpp:S108 | 1 |

---

## Fichier complet

Détails complets disponibles dans : `.claude/sonar/issues.json` (96 issues sur tout le projet, 4 après filtrage)
