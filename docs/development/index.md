# Développement - Guide pour Contributeurs

Bienvenue dans la section développement! Cette partie de la documentation est destinée aux contributeurs du projet R-Type, qu'ils soient débutants ou expérimentés.

## 📚 Guides Essentiels

### [Contribuer au Projet](contributing.md)
**Guide complet pour nouveaux contributeurs**

- Workflow Git (branches, commits, PR)
- Standards de code C++23
- Process de code review
- Communication avec l'équipe

[→ Guide de contribution](contributing.md)

---

### [Système de Logging](logging.md) 🆕
**Documentation complète du système spdlog**

- Architecture des 12 loggers (6 client + 6 serveur)
- Configuration multi-sink (console + fichiers)
- Utilisation dans le code
- Performance et thread safety
- Migration depuis std::cout

**Loggers disponibles:**

**Client:**
- NetworkLogger (DEBUG)
- EngineLogger (INFO)
- GraphicsLogger (INFO)
- SceneLogger (INFO)
- UILogger (INFO)
- BootLogger (INFO)

**Serveur:**
- NetworkLogger (DEBUG)
- AuthLogger (INFO)
- DomainLogger (INFO)
- PersistenceLogger (INFO)
- GameLogger (INFO)
- MainLogger (INFO)

[→ Documentation logging](logging.md)

---

### [Politiques de Commit](COMMIT_POLICES.md)
**Conventions AREA et hooks Git**

- Format: `TYPE(scope): description [RT-XXX]`
- Types autorisés: FEAT, FIX, DOCS, etc.
- Hooks de validation automatique
- Commits atomiques

**Exemple:**
```
FEAT(client/network): implémenter TCPClient asynchrone [RT-45]

- Connexion non-bloquante via Boost.ASIO
- Callbacks pour événements réseau
- Thread-safe avec mutex

🤖 Généré avec Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

[→ Politiques de commit](COMMIT_POLICES.md)

---

### [CI/CD Jenkins](ci-cd.md)
**Pipeline d'intégration et déploiement continu**

- Configuration Jenkins avec Docker
- Builder permanent avec workspaces isolés
- Pipeline automatisé (build, test, archive)
- Triggers sur Git push

**Features:**
- ✅ Build automatique sur push
- ✅ Tests unitaires (Google Test)
- ✅ Archivage artifacts
- ✅ Cache vcpkg réutilisé

[→ Guide CI/CD](ci-cd.md)

---

### [API Builder Jenkins](jenkins-builder-api.md)
**API REST Python pour builds asynchrones**

- Soumission de jobs via API
- Gestion de workspaces isolés
- Builds parallèles illimités
- Monitoring en temps réel

**Endpoints:**
```bash
POST /build       # Soumettre un build
GET /status/:id   # Vérifier statut
GET /logs/:id     # Récupérer logs
```

[→ API Builder](jenkins-builder-api.md)

---

### [Tests et Qualité](testing.md)
**Framework de tests et bonnes pratiques**

- Google Test (GTest)
- Tests unitaires
- Tests d'intégration
- Coverage de code
- Sanitizers (ASan, TSan, LSan, UBSan)

**Lancement:**
```bash
# Tests serveur
./artifacts/server/linux/server_tests

# Avec sanitizers
ENABLE_SANITIZERS=true ./scripts/compile.sh
LSAN_OPTIONS=suppressions=lsan.supp ./server_tests
```

[→ Guide des tests](testing.md)

---

### [Cross-Compilation Windows](cross-compilation.md)
**Compiler pour Windows depuis Linux**

- MinGW-w64 setup
- vcpkg Windows triplet
- Build artifacts Windows
- Debugging cross-platform

[→ Cross-compilation](cross-compilation.md)

---

### [Changelog](changelog.md)
**Historique des versions et modifications**

- Versions publiées
- Nouvelles fonctionnalités
- Corrections de bugs
- Breaking changes

[→ Voir le changelog](changelog.md)

---

## 🏗️ Architecture

### Guides d'Architecture

1. **[Architecture Hexagonale](../guides/hexagonal-architecture.md)**
   - Domain, Application, Infrastructure layers
   - Ports & Adapters pattern
   - Dependency injection

2. **[Architecture Réseau](../guides/network-architecture.md)**
   - Serveurs TCP/UDP asynchrones
   - Protocol design
   - Boost.ASIO patterns

3. **[Séparation Header/Implementation](../guides/cpp-header-implementation.md)**
   - Best practices .hpp/.cpp
   - Forward declarations
   - Compilation times

4. **[Système d'Authentification](../guides/authentication.md)**
   - User entity et value objects
   - MongoDB persistence
   - Password hashing

## 🛠️ Outils de Développement

### Scripts Utiles

```bash
# Build complet
./scripts/build.sh

# Compilation
./scripts/compile.sh

# Compilation avec sanitizers
ENABLE_SANITIZERS=true ./scripts/compile.sh

# Lancement infrastructure
./scripts/launch_ci_cd.sh

# Analyse SonarCloud
./scripts/sonar-analyze.sh
```

### Configuration IDE

**VSCode recommandé:**
```json
{
  "C_Cpp.default.cppStandard": "c++23",
  "C_Cpp.default.compilerPath": "/usr/bin/g++-13",
  "cmake.configureArgs": [
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
  ]
}
```

**CLion:**
- CMake automatiquement détecté
- vcpkg toolchain supporté
- Debugging intégré

### Sanitizers

**LeakSanitizer:**
```bash
# Fichier suppression faux positifs GPU
LSAN_OPTIONS=suppressions=lsan.supp ./rtype_client
```

**AddressSanitizer:**
```bash
# Détection erreurs mémoire
ASAN_OPTIONS=detect_leaks=1 ./rtype_client
```

**ThreadSanitizer:**
```bash
# Détection race conditions
TSAN_OPTIONS=history_size=7 ./rtype_client
```

## 📊 Métriques Qualité

### SonarCloud

**Métriques suivies:**
- Code smells
- Bugs potentiels
- Vulnérabilités sécurité
- Duplication de code
- Coverage tests
- Complexité cyclomatique

**Dashboard:** https://sonarcloud.io/project/overview?id=your-org_rtype

### Jenkins CI

**Build metrics:**
- Temps de build
- Taux de succès
- Tests passed/failed
- Artifacts size

**Dashboard:** http://localhost:8081

## 🐛 Debugging

### Logs

```bash
# Voir logs en temps réel
tail -f logs/client.log
tail -f logs/server.log

# Filtrer par niveau
grep "\[error\]" logs/client.log
grep "\[Network\]" logs/client.log

# Activer debug mode
// Dans le code:
client::logging::Logger::getNetworkLogger()->set_level(spdlog::level::debug);
```

### GDB

```bash
# Debug avec GDB
gdb ./artifacts/server/linux/rtype_server

# Avec core dump
gdb ./rtype_server core.12345

# Valgrind
valgrind --leak-check=full ./rtype_server
```

## 🔄 Workflow de Développement

### 1. Créer une Branche

```bash
git checkout -b feature/my-feature
```

### 2. Développer et Tester

```bash
# Code...
./scripts/compile.sh
./artifacts/server/linux/server_tests
```

### 3. Commit avec Logging

```cpp
// Ajouter logs appropriés
auto logger = client::logging::Logger::getEngineLogger();
logger->info("Feature implemented: {}", featureName);
```

### 4. Commit Atomique

```bash
git add -p  # Stage par morceaux
git commit --no-verify -m "FEAT(engine): implement new feature

- Feature description
- Tests added
- Logs integrated

Lié à: EPI-XX

🤖 Généré avec Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>"
```

### 5. Push et PR

```bash
git push origin feature/my-feature
# Créer PR sur GitHub
```

### 6. Code Review

- CI/CD passe ✅
- Tests passent ✅
- Code review approuvé ✅
- Merge!

## 📝 Standards de Code

### C++23

```cpp
// Utiliser concepts
template<typename T>
concept Drawable = requires(T t) {
    { t.draw() } -> std::same_as<void>;
};

// Ranges
auto result = myVector
    | std::views::filter([](int x) { return x > 0; })
    | std::views::transform([](int x) { return x * 2; });

// Modules (quand supporté)
import std.core;
```

### Logging

```cpp
// ✅ Bon
auto logger = client::logging::Logger::getNetworkLogger();
logger->info("Connecting to {}:{}", host, port);

// ❌ Mauvais
std::cout << "Connecting to " << host << ":" << port << std::endl;
```

### Error Handling

```cpp
// ✅ Bon
try {
    texture.load(file);
} catch (const std::exception& e) {
    logger->error("Failed to load texture: {}", e.what());
    throw;
}

// ❌ Mauvais
try {
    texture.load(file);
} catch (...) {
    // Silent error
}
```

## 🎯 Prochaines Étapes

Après avoir lu cette section:

1. **Contribuez!** - [Guide de contribution](contributing.md)
2. **Explorez l'architecture** - [Architecture hexagonale](../guides/hexagonal-architecture.md)
3. **Intégrez le logging** - [Documentation logging](logging.md)
4. **Testez votre code** - [Guide des tests](testing.md)

## Ressources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Modern C++ Best Practices](https://github.com/cpp-best-practices)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Boost.ASIO Patterns](https://think-async.com/Asio/)
- [spdlog Documentation](https://github.com/gabime/spdlog)
