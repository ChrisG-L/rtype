# Rapport SonarQube - Résumé

Commit: b0d4558 | Branche: main | Date: 2025-12-19 15:49

Mode: **Filtré sur 15 fichiers du diff**

Fichiers analysés:
- .claude/scripts/bootstrap.py
- .claude/scripts/transform-sonar.py
- .claude/scripts/validate-web-report.py
- claude.py
- main.py
- src/client/include/network/UDPClient.hpp
- src/client/include/scenes/GameScene.hpp
- src/client/lib/sfml/include/utils/TextField.hpp
- src/client/main.cpp
- src/client/src/network/UDPClient.cpp
- src/client/src/scenes/GameScene.cpp
- src/server/include/infrastructure/adapters/in/network/UDPServer.hpp
- src/server/include/infrastructure/game/GameWorld.hpp
- src/server/infrastructure/adapters/in/network/UDPServer.cpp
- src/server/infrastructure/game/GameWorld.cpp

*(5 issues sur les fichiers du diff, 96 sur tout le projet)*

---

## Vue d'ensemble

### Par sévérité

| Sévérité | Count |
|----------|-------|
| Blocker | 0 |
| Critical | 2 |
| Major | 2 |
| Minor | 1 |
| Info | 0 |
| **Total** | **5** |

### Par catégorie

| Catégorie | Count |
|-----------|-------|
| Security | 0 |
| Reliability | 1 |
| Maintainability | 4 |

---

## Issues Bloquantes (0)

Aucune issue bloquante.

---

## Issues Critiques (2)

### 1. Do not throw uncaught exceptions in a destructor.

- **Fichier**: src/client/src/network/UDPClient.cpp:28
- **Règle**: cpp:S1048
- **Sévérité**: Critical
- **Catégorie**: Reliability
- **Effort**: 30min

### 2. Refactor this code to not nest more than 3 if|for|do|while|switch statements.

- **Fichier**: src/server/infrastructure/adapters/in/network/UDPServer.cpp:79
- **Règle**: cpp:S134
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

---

## Issues Majeures (2)

### 1. Fill this compound statement, remove it, or add a nested comment explaining why it is empty.

- **Fichier**: src/client/src/network/UDPClient.cpp:169
- **Règle**: cpp:S108
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 2. Merge this "if" statement with the enclosing one.

- **Fichier**: src/server/infrastructure/adapters/in/network/UDPServer.cpp:79
- **Règle**: cpp:S1066
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

---

## Fichiers les plus impactés (top 20)

| Fichier | Blocker | Critical | Major | Minor | Total |
|---------|---------|----------|-------|-------|-------|
| src/client/src/network/UDPClient.cpp | 0 | 1 | 1 | 1 | 3 |
| src/server/infrastructure/adapters/in/network/UDPServer.cpp | 0 | 1 | 1 | 0 | 2 |

---

## Règles les plus violées (top 20)

| Règle | Description | Count |
|-------|-------------|-------|
| cpp:S108 | cpp:S108 | 1 |
| cpp:S1048 | cpp:S1048 | 1 |
| cpp:S1481 | cpp:S1481 | 1 |
| cpp:S134 | cpp:S134 | 1 |
| cpp:S1066 | cpp:S1066 | 1 |

---

## Fichier complet

Détails complets disponibles dans : `.claude/sonar/issues.json` (96 issues sur tout le projet, 5 après filtrage)
