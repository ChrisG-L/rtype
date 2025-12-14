# Rapport SonarQube - Résumé

Commit: abc123 | Branche: feature/test | Date: 2025-12-12 11:14

---

## Vue d'ensemble

### Par sévérité

| Sévérité | Count |
|----------|-------|
| Blocker | 1 |
| Critical | 1 |
| Major | 3 |
| Minor | 1 |
| Info | 1 |
| **Total** | **7** |

### Par catégorie

| Catégorie | Count |
|-----------|-------|
| Security | 1 |
| Reliability | 1 |
| Maintainability | 5 |

---

## Issues Bloquantes (1)

### 1. Possible null pointer dereference in socket handler

- **Fichier**: src/network/UDPServer.cpp:142
- **Règle**: cpp:S2259
- **Sévérité**: Blocker
- **Catégorie**: Reliability
- **Effort**: 15min

---

## Issues Critiques (1)

### 1. Hardcoded password detected: 'admin123'

- **Fichier**: src/auth/Login.cpp:34
- **Règle**: cpp:S2068
- **Sévérité**: Critical
- **Catégorie**: Security
- **Effort**: 30min

---

## Issues Majeures (3)

### 1. Function has too many parameters (8 > 7 allowed)

- **Fichier**: src/ecs/Registry.hpp:12
- **Règle**: cpp:S107
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 1h

### 2. This class has 45 methods, which is greater than 20 authorized

- **Fichier**: src/ecs/Registry.hpp:1
- **Règle**: cpp:S1200
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2h

### 3. Control flow statements nested too deeply (5 > 4 allowed)

- **Fichier**: src/game/GameLoop.cpp:89
- **Règle**: cpp:S134
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 45min

---

## Fichiers les plus impactés (top 20)

| Fichier | Blocker | Critical | Major | Minor | Total |
|---------|---------|----------|-------|-------|-------|
| src/ecs/Registry.hpp | 0 | 0 | 2 | 0 | 2 |
| src/network/UDPServer.cpp | 1 | 0 | 0 | 0 | 1 |
| src/auth/Login.cpp | 0 | 1 | 0 | 0 | 1 |
| src/game/GameLoop.cpp | 0 | 0 | 1 | 0 | 1 |
| src/utils/Logger.cpp | 0 | 0 | 0 | 1 | 1 |
| src/main.cpp | 0 | 0 | 0 | 0 | 1 |

---

## Règles les plus violées (top 20)

| Règle | Description | Count |
|-------|-------------|-------|
| cpp:S2259 | Null pointer dereference | 1 |
| cpp:S2068 | Hardcoded credentials | 1 |
| cpp:S107 | Too many parameters | 1 |
| cpp:S1200 | Classes should not have too many methods | 1 |
| cpp:S134 | Control flow statements nested too deeply | 1 |
| cpp:S1066 | Merge if statements | 1 |
| cpp:S1135 | TODO comment | 1 |

---

## Fichier complet

Détails complets disponibles dans : `.claude/tests/test_sonar_input.json`
