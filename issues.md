# Rapport SonarQube - Résumé

Commit: (non spécifié) | Branche: (non spécifiée) | Date: 2025-12-12 11:16

---

## Vue d'ensemble

### Par sévérité

| Sévérité | Count |
|----------|-------|
| Blocker | 1 |
| Critical | 30 |
| Major | 43 |
| Minor | 22 |
| Info | 0 |
| **Total** | **96** |

### Par catégorie

| Catégorie | Count |
|-----------|-------|
| Security | 1 |
| Reliability | 1 |
| Maintainability | 94 |

---

## Issues Bloquantes (1)

### 1. Make sure this MongoDB database password gets changed and removed from the code.

- **Fichier**: reports/web-report-2025-12-12-e8a89a9.json:135
- **Règle**: secrets:S6694
- **Sévérité**: Blocker
- **Catégorie**: Security
- **Effort**: 30min

---

## Issues Critiques (30)

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

### 4. Refactor this code to not nest more than 3 if|for|do|while|switch statements.

- **Fichier**: src/client/src/core/GameLoop.cpp:52
- **Règle**: cpp:S134
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 5. Do not throw uncaught exceptions in a destructor.

- **Fichier**: src/client/src/network/UDPClient.cpp:28
- **Règle**: cpp:S1048
- **Sévérité**: Critical
- **Catégorie**: Reliability
- **Effort**: 30min

### 6. Replace this use of "void *" with a more meaningful type.

- **Fichier**: src/common/protocol/Protocol.hpp:88
- **Règle**: cpp:S5008
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 7. Replace this use of "void *" with a more meaningful type.

- **Fichier**: src/common/protocol/Protocol.hpp:182
- **Règle**: cpp:S5008
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 8. Refactor this code to not nest more than 3 if|for|do|while|switch statements.

- **Fichier**: src/server/infrastructure/adapters/in/network/UDPServer.cpp:79
- **Règle**: cpp:S134
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 9. Explicitly capture the required scope variables.

- **Fichier**: src/client/lib/sdl2/src/SDL2Renderer.cpp:32
- **Règle**: cpp:S3608
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 10. Explicitly capture all local variables required in this lambda.

- **Fichier**: src/client/lib/sdl2/src/SDL2Renderer.cpp:32
- **Règle**: cpp:S5019
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 1min

### 11. Add the "explicit" keyword to this constructor.

- **Fichier**: src/client/lib/sdl2/include/SDL2Renderer.hpp:24
- **Règle**: cpp:S1709
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 2min

### 12. Customize this class' copy constructor to participate in resource management. Customize or delete its copy assignment operator. Also consider whether move operations should be customized.

- **Fichier**: src/client/lib/sdl2/include/SDL2Window.hpp:18
- **Règle**: cpp:S3624
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 30min

### 13. Customize this class' copy constructor to participate in resource management. Customize or delete its copy assignment operator. Also consider whether move operations should be customized.

- **Fichier**: src/client/lib/sdl2/include/utils/SDL2AssetManager.hpp:29
- **Règle**: cpp:S3624
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 30min

### 14. Add the "explicit" keyword to this constructor.

- **Fichier**: src/client/lib/sdl2/include/utils/SDL2AssetManager.hpp:31
- **Règle**: cpp:S1709
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 2min

### 15. Use "=default" instead of the default implementation of this special member functions.

- **Fichier**: src/client/lib/sdl2/src/SDL2Plugin.cpp:11
- **Règle**: cpp:S3490
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 2min

### 16. Replace the use of "new" with an operation that automatically manages the memory.

- **Fichier**: src/client/lib/sdl2/src/SDL2Plugin.cpp:33
- **Règle**: cpp:S5025
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 17. Rewrite the code so that you no longer need this "delete".

- **Fichier**: src/client/lib/sdl2/src/SDL2Plugin.cpp:37
- **Règle**: cpp:S5025
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 18. Explicitly capture the required scope variables.

- **Fichier**: src/client/lib/sdl2/src/SDL2Renderer.cpp:26
- **Règle**: cpp:S3608
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 19. Explicitly capture all local variables required in this lambda.

- **Fichier**: src/client/lib/sdl2/src/SDL2Renderer.cpp:26
- **Règle**: cpp:S5019
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 1min

### 20. Replace this use of "void *" with a more meaningful type.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:150
- **Règle**: cpp:S5008
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 10min

### 21. Add a nested comment explaining why this method is empty, or complete the implementation.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:156
- **Règle**: cpp:S1186
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 22. Add a nested comment explaining why this method is empty, or complete the implementation.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:167
- **Règle**: cpp:S1186
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 23. Explicitly capture the required scope variables.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:21
- **Règle**: cpp:S3608
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 24. Explicitly capture all local variables required in this lambda.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:21
- **Règle**: cpp:S5019
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 1min

### 25. Explicitly capture the required scope variables.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:26
- **Règle**: cpp:S3608
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 26. Explicitly capture all local variables required in this lambda.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:26
- **Règle**: cpp:S5019
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 1min

### 27. Explicitly capture the required scope variables.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:35
- **Règle**: cpp:S3608
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 28. Explicitly capture all local variables required in this lambda.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:35
- **Règle**: cpp:S5019
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 1min

### 29. Explicitly capture the required scope variables.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:40
- **Règle**: cpp:S3608
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 5min

### 30. Explicitly capture all local variables required in this lambda.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:40
- **Règle**: cpp:S5019
- **Sévérité**: Critical
- **Catégorie**: Maintainability
- **Effort**: 1min

---

## Issues Majeures (43)

### 1. Fill this compound statement, remove it, or add a nested comment explaining why it is empty.

- **Fichier**: src/client/src/network/UDPClient.cpp:169
- **Règle**: cpp:S108
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 2. Reduce the number of nested "break" statements from 2 to 1 authorized.

- **Fichier**: src/client/src/network/TCPClient.cpp:177
- **Règle**: cpp:S924
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 3. Reduce the number of nested "break" statements from 2 to 1 authorized.

- **Fichier**: src/server/infrastructure/adapters/in/network/TCPServer.cpp:46
- **Règle**: cpp:S924
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 4. Merge this "if" statement with the enclosing one.

- **Fichier**: src/server/infrastructure/adapters/in/network/UDPServer.cpp:79
- **Règle**: cpp:S1066
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 5. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/server/infrastructure/bootstrap/GameBootstrap.hpp:34
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 6. Use the transparent equality "std::equal_to<>" and a custom transparent heterogeneous hasher with this associative string container.

- **Fichier**: src/client/lib/sdl2/include/utils/SDL2AssetManager.hpp:42
- **Règle**: cpp:S6045
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 7. Use the transparent equality "std::equal_to<>" and a custom transparent heterogeneous hasher with this associative string container.

- **Fichier**: src/client/lib/sdl2/include/utils/SDL2AssetManager.hpp:43
- **Règle**: cpp:S6045
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 8. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2AssetManager.cpp:16
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 9. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Renderer.cpp:15
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 10. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Renderer.cpp:51
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 11. Reduce the number of switch cases from 52 to at most 30.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:15
- **Règle**: cpp:S1479
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 30min

### 12. Do not use the constructor's initializer list for data member "_renderer". Use the in-class initializer instead.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:72
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 13. Do not use the constructor's initializer list for data member "_isOpen". Use the in-class initializer instead.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:72
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 14. Do not use the constructor's initializer list for data member "_window". Use the in-class initializer instead.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:72
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 15. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:75
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 16. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:89
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 17. Define and throw a dedicated exception instead of using a generic one.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:96
- **Règle**: cpp:S112
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 20min

### 18. Use braces to initialize the aggregate.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:115
- **Règle**: cpp:S6872
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 1min

### 19. Pass polymorphic object "drawable" by reference to const.

- **Fichier**: src/client/lib/sdl2/src/SDL2Window.cpp:166
- **Règle**: cpp:S1238
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 20. Merge this "if" statement with the enclosing one.

- **Fichier**: src/server/include/infrastructure/configuration/EnvLoader.hpp:58
- **Règle**: cpp:S1066
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 21. Use the transparent equality "std::equal_to<>" and a custom transparent heterogeneous hasher with this associative string container.

- **Fichier**: src/client/lib/sfml/include/utils/AssetManager.hpp:44
- **Règle**: cpp:S6045
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 22. Remove this use of the constructor's initializer list for data member "textures". It is redundant with default initialization behavior.

- **Fichier**: src/client/lib/sfml/src/AssetManager.cpp:15
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 23. Update this method so that its implementation is not identical to addSprite.

- **Fichier**: src/client/lib/sfml/src/AssetManager.cpp:82
- **Règle**: cpp:S4144
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 15min

### 24. Replace the redundant type with "auto".

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:62
- **Règle**: cpp:S5827
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 25. Use "std::to_underlying" to cast enums to their underlying type.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:62
- **Règle**: cpp:S7035
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 15min

### 26. This function should be declared "const".

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:69
- **Règle**: cpp:S5817
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 27. Remove the unused parameter "textureAsset", make it unnamed, or declare it "[[maybe_unused]]".

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:69
- **Règle**: cpp:S1172
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 28. Remove the commented out code.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:70
- **Règle**: cpp:S125
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 29. Replace the redundant type with "auto".

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:85
- **Règle**: cpp:S5827
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 30. Use "std::to_underlying" to cast enums to their underlying type.

- **Fichier**: src/client/lib/sfml/src/SFMLRenderer.cpp:85
- **Règle**: cpp:S7035
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 15min

### 31. Do not use the constructor's initializer list for data member "_size". Use the in-class initializer instead.

- **Fichier**: src/client/src/graphics/Graphics.cpp:15
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 32. Pass expensive to copy object "name" by reference to const.

- **Fichier**: src/client/src/graphics/Graphics.cpp:43
- **Règle**: cpp:S1238
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 5min

### 33. Do not use the constructor's initializer list for data member "_isAlive". Use the in-class initializer instead.

- **Fichier**: src/client/src/graphics/Graphics.cpp:45
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 34. Remove this use of the constructor's initializer list for data member "_textures". It is redundant with default initialization behavior.

- **Fichier**: src/client/src/scenes/LoginScene.cpp:21
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 35. Remove this use of the constructor's initializer list for data member "_elements". It is redundant with default initialization behavior.

- **Fichier**: src/client/src/scenes/LoginScene.cpp:22
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 36. Do not use the constructor's initializer list for data member "_entityManager". Use the in-class initializer instead.

- **Fichier**: src/client/src/scenes/LoginScene.cpp:23
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 37. Remove this use of the constructor's initializer list for data member "_fireCooldown". It is redundant with the in-class initializer.

- **Fichier**: src/client/src/scenes/LoginScene.cpp:24
- **Règle**: cpp:S3230
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 10min

### 38. Replace this use of "push_back" with "emplace_back".

- **Fichier**: src/client/src/scenes/LoginScene.cpp:36
- **Règle**: cpp:S6003
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 39. Replace this use of "push_back" with "emplace_back".

- **Fichier**: src/client/src/scenes/LoginScene.cpp:37
- **Règle**: cpp:S6003
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 40. Replace this use of "push_back" with "emplace_back".

- **Fichier**: src/client/src/scenes/LoginScene.cpp:38
- **Règle**: cpp:S6003
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 41. Replace this use of "push_back" with "emplace_back".

- **Fichier**: src/client/src/scenes/LoginScene.cpp:40
- **Règle**: cpp:S6003
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 42. Replace this use of "push_back" with "emplace_back".

- **Fichier**: src/client/src/scenes/LoginScene.cpp:41
- **Règle**: cpp:S6003
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 2min

### 43. Use std::format instead of concatenating pieces manually.

- **Fichier**: src/client/src/scenes/LoginScene.cpp:104
- **Règle**: cpp:S6185
- **Sévérité**: Major
- **Catégorie**: Maintainability
- **Effort**: 3min

---

## Fichiers les plus impactés (top 20)

| Fichier | Blocker | Critical | Major | Minor | Total |
|---------|---------|----------|-------|-------|-------|
| src/client/lib/sfml/src/SFMLRenderer.cpp | 0 | 8 | 7 | 2 | 17 |
| src/client/lib/sdl2/src/SDL2Window.cpp | 0 | 3 | 9 | 3 | 15 |
| src/client/src/scenes/LoginScene.cpp | 0 | 0 | 10 | 5 | 15 |
| src/client/lib/sdl2/src/SDL2Renderer.cpp | 0 | 4 | 2 | 0 | 6 |
| src/client/src/network/TCPClient.cpp | 0 | 3 | 1 | 0 | 4 |
| src/client/lib/sdl2/include/utils/SDL2AssetManager.hpp | 0 | 2 | 2 | 0 | 4 |
| src/client/lib/sdl2/src/SDL2AssetManager.cpp | 0 | 0 | 1 | 3 | 4 |
| src/client/lib/sfml/src/AssetManager.cpp | 0 | 0 | 2 | 2 | 4 |
| src/client/src/network/UDPClient.cpp | 0 | 1 | 1 | 1 | 3 |
| src/client/lib/sdl2/src/SDL2Plugin.cpp | 0 | 3 | 0 | 0 | 3 |
| src/client/src/graphics/Graphics.cpp | 0 | 0 | 3 | 0 | 3 |
| src/common/protocol/Protocol.hpp | 0 | 2 | 0 | 0 | 2 |
| src/server/infrastructure/adapters/in/network/UDPServer.cpp | 0 | 1 | 1 | 0 | 2 |
| src/client/lib/sdl2/include/SDL2Window.hpp | 0 | 1 | 0 | 1 | 2 |
| src/client/lib/sdl2/include/plugins/SDL2Plugin.hpp | 0 | 0 | 0 | 2 | 2 |
| src/server/include/infrastructure/configuration/EnvLoader.hpp | 0 | 0 | 1 | 1 | 2 |
| src/client/include/graphics/Asset.hpp | 0 | 0 | 0 | 2 | 2 |
| reports/web-report-2025-12-12-e8a89a9.json | 1 | 0 | 0 | 0 | 1 |
| src/client/src/core/GameLoop.cpp | 0 | 1 | 0 | 0 | 1 |
| src/server/infrastructure/adapters/in/network/TCPServer.cpp | 0 | 0 | 1 | 0 | 1 |

---

## Règles les plus violées (top 20)

| Règle | Description | Count |
|-------|-------------|-------|
| cpp:S3230 | cpp:S3230 | 10 |
| cpp:S112 | cpp:S112 | 7 |
| cpp:S3608 | cpp:S3608 | 6 |
| cpp:S5019 | cpp:S5019 | 6 |
| cpp:S6003 | cpp:S6003 | 5 |
| cpp:S134 | cpp:S134 | 4 |
| cpp:S5008 | cpp:S5008 | 3 |
| cpp:S6045 | cpp:S6045 | 3 |
| cpp:S5350 | cpp:S5350 | 3 |
| cpp:S6009 | cpp:S6009 | 3 |
| cpp:S924 | cpp:S924 | 2 |
| cpp:S1066 | cpp:S1066 | 2 |
| cpp:S6004 | cpp:S6004 | 2 |
| cpp:S1709 | cpp:S1709 | 2 |
| cpp:S3624 | cpp:S3624 | 2 |
| cpp:S3539 | cpp:S3539 | 2 |
| cpp:S1659 | cpp:S1659 | 2 |
| cpp:S5025 | cpp:S5025 | 2 |
| cpp:S6177 | cpp:S6177 | 2 |
| cpp:S1186 | cpp:S1186 | 2 |

---

## Fichier complet

Détails complets disponibles dans : `issues.json`
