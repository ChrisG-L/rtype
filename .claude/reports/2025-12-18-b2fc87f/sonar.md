# Rapport SonarQube - Résumé

Commit: b2fc87f | Branche: defense2 | Date: 2025-12-18 14:06

Mode: **Filtré sur 20 fichiers du diff**

Fichiers analysés:
- src/client/include/accessibility/AccessibilityConfig.hpp
- src/client/include/audio/AudioManager.hpp
- src/client/include/graphics/IWindow.hpp
- src/client/include/network/UDPClient.hpp
- src/client/include/scenes/GameScene.hpp
- src/client/lib/sdl2/include/SDL2Window.hpp
- src/client/lib/sdl2/src/SDL2Window.cpp
- src/client/lib/sfml/include/SFMLWindow.hpp
- src/client/lib/sfml/include/utils/TextField.hpp
- src/client/lib/sfml/src/SFMLWindow.cpp
- src/client/main.cpp
- src/client/src/accessibility/AccessibilityConfig.cpp
- src/client/src/audio/AudioManager.cpp
- src/client/src/network/UDPClient.cpp
- src/client/src/scenes/GameScene.cpp
- src/common/protocol/Protocol.hpp
- src/server/include/infrastructure/adapters/in/network/UDPServer.hpp
- src/server/include/infrastructure/game/GameWorld.hpp
- src/server/infrastructure/adapters/in/network/UDPServer.cpp
- src/server/infrastructure/game/GameWorld.cpp

*(24 issues sur les fichiers du diff, 96 sur tout le projet)*

---

## Vue d'ensemble

### Par sévérité

| Sévérité | Count |
|----------|-------|
| Blocker | 0 |
| Critical | 8 |
| Major | 11 |
| Minor | 5 |
| Info | 0 |
| **Total** | **24** |

### Par catégorie

| Catégorie | Count |
|-----------|-------|
| Security | 0 |
| Reliability | 1 |
| Maintainability | 23 |

---

## Issues Bloquantes (0)

Aucune issue bloquante.

---

## Issues Critiques (8)

### 1. Do not throw uncaught exceptions in a destructor.

- **Fichier**: src/client/src/network/UDPClient.cpp:28
- **Règle**: cpp:S1048
- **Sévérité**: Critical
- **Catégorie**: Reliability
- **Effort**: 30min

### 2. Replace this use of "void *" with a more meaningful type.

- **Fichier**: src/common/protocol/Protocol.hpp:88
- **Règle**: cpp:S5008
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 3. Replace this use of "void *" with a more meaningful type.

- **Fichier**: src/common/protocol/Protocol.hpp:182
- **Règle**: cpp:S5008
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 4. Refactor this code to not nest more than 3 if|for|do|while|switch statements.

- **Fichier**: src/server/infrastructure/adapters/in/network/UDPServer.cpp:79
- **Règle**: cpp:S134
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 5. Customize this class' copy constructor to participate in resource management. Customize or delete its copy assignment operator. Also consider whether move operations should be customized.

- **Fichier**: src/client/lib/sdl2/include/SDL2Window.hpp:18
- **Règle**: cpp:S3624
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 30min

### 6. Replace this use of "void *" with a more meaningful type.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:150
- **Règle**: cpp:S5008
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 7. Add a nested comment explaining why this method is empty, or complete the implementation.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:156
- **Règle**: cpp:S1186
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 8. Add a nested comment explaining why this method is empty, or complete the implementation.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:167
- **Règle**: cpp:S1186
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

---

## Issues Majeures (11)

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

### 3. Reduce the number of switch cases from 52 to at most 30.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:15
- **Règle**: cpp:S1479
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 30min

### 4. Do not use the constructor's initializer list for data member "_renderer". Use the in-class initializer instead.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:72
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 5. Do not use the constructor's initializer list for data member "_isOpen". Use the in-class initializer instead.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:72
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 6. Do not use the constructor's initializer list for data member "_window". Use the in-class initializer instead.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:72
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 7. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:75
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 8. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:89
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 9. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:96
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 10. Use braces to initialize the aggregate.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:115
- **Règle**: cpp:S6872
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 1min

### 11. Pass polymorphic object "drawable" by reference to const.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:166
- **Règle**: cpp:S1238
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

---

## Fichiers les plus impactés (top 20)

| Fichier | Blocker | Critical | Major | Minor | Total |
|---------|---------|----------|-------|-------|-------|
| src/client/lib/sdl2/src/SDL2Window.cpp | 0 | 3 | 9 | 3 | 15 |
| src/client/src/network/UDPClient.cpp | 0 | 1 | 1 | 1 | 3 |
| src/common/protocol/Protocol.hpp | 0 | 2 | 0 | 0 | 2 |
| src/server/infrastructure/adapters/in/network/UDPServer.cpp | 0 | 1 | 1 | 0 | 2 |
| src/client/lib/sdl2/include/SDL2Window.hpp | 0 | 1 | 0 | 1 | 2 |

---

## Règles les plus violées (top 20)

| Règle | Description | Count |
|-------|-------------|-------|
| cpp:S5008 | cpp:S5008 | 3 |
| cpp:S3230 | cpp:S3230 | 3 |
| cpp:S112 | cpp:S112 | 3 |
| cpp:S1186 | cpp:S1186 | 2 |
| cpp:S108 | cpp:S108 | 1 |
| cpp:S1048 | cpp:S1048 | 1 |
| cpp:S1481 | cpp:S1481 | 1 |
| cpp:S134 | cpp:S134 | 1 |
| cpp:S1066 | cpp:S1066 | 1 |
| cpp:S6004 | cpp:S6004 | 1 |
| cpp:S3624 | cpp:S3624 | 1 |
| cpp:S3471 | cpp:S3471 | 1 |
| cpp:S6177 | cpp:S6177 | 1 |
| cpp:S1479 | cpp:S1479 | 1 |
| cpp:S1659 | cpp:S1659 | 1 |
| cpp:S6872 | cpp:S6872 | 1 |
| cpp:S1238 | cpp:S1238 | 1 |

---

## Fichier complet

Détails complets disponibles dans : `.claude/sonar/issues.json` (96 issues sur tout le projet, 24 après filtrage)
