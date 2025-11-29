# R-Type Project Context

## Quick Reference

| Aspect | Value |
|--------|-------|
| **Language** | C++23 (strict, no extensions) |
| **Build** | CMake 3.30+ / Ninja / vcpkg |
| **Server** | Hexagonal Architecture (DDD) |
| **Client** | Plugin-based (SFML 3.0) |
| **Tests** | Google Test (210+ tests) |
| **Docs** | MkDocs Material (French) |

## Build Commands

```bash
# First time setup (10-30 min)
./scripts/build.sh --platform=linux

# Compile (15s incremental)
./scripts/compile.sh --platform=linux --both

# Run tests
./scripts/test.sh

# Run binaries
./artifacts/r-type_server
./artifacts/r-type_client
```

## Project Structure

```
src/
├── server/                    # Hexagonal architecture
│   ├── domain/                # Entities, Value Objects (NO external deps)
│   ├── application/           # Use Cases, Ports (interfaces)
│   └── infrastructure/        # Adapters (network, DB, CLI)
└── client/                    # Plugin architecture
    ├── core/                  # Engine, GameLoop, DynamicLib
    ├── graphics/              # IGraphicPlugin interface
    ├── scenes/                # SceneManager, LoginScene, GameScene
    └── lib/sfml/              # SFML plugin (rtype_sfml.so)

tests/                         # Google Test (server: 170+, client: 40+)
docs/                          # MkDocs documentation
scripts/                       # build.sh, compile.sh, test.sh
.claude/agents/                # Army2077 agent system
```

## Architecture Rules

### Server: Hexagonal (CRITICAL)

```
Domain ← Application ← Infrastructure
  │           │              │
  │           │              └── Adapters (TCP, UDP, MongoDB, CLI)
  │           └── Use Cases + Ports (interfaces)
  └── Entities + Value Objects (ZERO external dependencies)
```

**Domain Layer** (`src/server/domain/`):
- NO #include of Boost, MongoDB, spdlog, etc.
- Pure C++ standard library only
- Entities: `Player`, `User`
- Value Objects: `Health`, `Position`, `PlayerId`, `Username`, `Email`, `Password`

**Application Layer** (`src/server/application/`):
- Use Cases orchestrate domain objects
- Ports IN: `IGameCommands` (driving interfaces)
- Ports OUT: `IPlayerRepository`, `IUserRepository` (driven interfaces)

**Infrastructure Layer** (`src/server/infrastructure/`):
- Adapters IN: `TCPServer`, `UDPServer`, `CLIGameController`
- Adapters OUT: `MongoDBUserRepository`
- All external libraries here

### Client: Plugin Architecture

```cpp
// Interface (src/client/include/graphics/)
class IGraphicPlugin {
    virtual IWindow& getWindow() = 0;
    virtual IRenderer& getRenderer() = 0;
};

// Implementation (src/client/lib/sfml/)
class SFMLPlugin : public IGraphicPlugin { };
```

- Plugins compiled as shared libraries (`.so`)
- Runtime loading via `DynamicLib`
- SFML abstracted behind interfaces

## Code Conventions

### Naming
```cpp
class PascalCase { };              // Classes
void camelCase();                  // Functions
const float UPPER_SNAKE = 1.0f;    // Constants
namespace lowercase::nested { }    // Namespaces
float _privateVar;                 // Private members (underscore prefix)
class IInterface { };              // Interfaces (I prefix)
```

### File Header
```cpp
/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Brief description
*/

#ifndef FILENAME_HPP_
#define FILENAME_HPP_

namespace domain::entities {
    // ...
}

#endif /* !FILENAME_HPP_ */
```

### Modern C++ Requirements
- RAII (no manual new/delete)
- Smart pointers (`std::unique_ptr` preferred)
- Move semantics
- const-correctness
- Range-based for loops
- `auto` where appropriate

## Key Dependencies

| Library | Purpose | Include Path |
|---------|---------|--------------|
| Boost.Asio | Async networking | `<boost/asio.hpp>` |
| spdlog | Logging | `<spdlog/spdlog.h>` |
| MongoDB | Persistence | `<mongocxx/...>` |
| SFML 3 | Graphics | `<SFML/Graphics.hpp>` |
| GTest | Testing | `<gtest/gtest.h>` |
| Protobuf | Serialization | `<google/protobuf/...>` |

## Network Configuration

| Protocol | Port | Purpose |
|----------|------|---------|
| TCP | 4123 | Authentication |
| UDP | 4123 | Gameplay (real-time) |

## Git Conventions

### Commit Format (AREA)
```
TYPE: Description courte

Explication détaillée (optionnel)

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

**Types**: `FEAT`, `FIX`, `DOCS`, `STYLE`, `REFACTOR`, `TEST`, `BUILD`, `PERF`, `CI`, `CHORE`

### Branches
- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation
- `refactor/` - Code restructuring

## Testing

```bash
# All tests
./scripts/test.sh

# Server only
./scripts/test.sh --server

# Client only
./scripts/test.sh --client

# Filter pattern
./scripts/test.sh --filter "Health*"

# List tests
./scripts/test.sh --list
```

**Test locations**:
- `tests/server/domain/` - Domain entities/value objects
- `tests/server/network/` - TCP/UDP integration
- `tests/client/utils/` - Client utilities

## Army2077 Agent System

Located in `.claude/agents/`:

| Agent | Purpose | Command |
|-------|---------|---------|
| **Général** | Orchestrator | "Général, audit complet" |
| **CMD Documentation** | Docs | "CMD Doc, documente [module]" |
| **CMD Qualité** | Quality/Tests | "CMD Qualité, analyse" |
| **CMD Sécurité** | Security/CI | "CMD Sécurité, audit" |
| **CMD Git** | Git commits | "CMD Git, analyse changements" |
| **CMD Jira** | Ticket sync | "CMD Jira, sync Jira" |
| **Soldat Advisor** | Learning guide | "Soldat Advisor, analyse [file]" |
| **Soldat Review** | Code review | "Soldat Review, revue PR" |
| **Soldat Architecture** | Architecture | "Soldat Archi, cartographie" |

## Common Tasks

### Add New Entity (Domain)
1. Create in `src/server/include/domain/entities/`
2. NO external includes (Boost, MongoDB, etc.)
3. Add tests in `tests/server/domain/entities/`

### Add New Value Object (Domain)
1. Create in `src/server/include/domain/value_objects/`
2. Immutable, validated in constructor
3. Add tests in `tests/server/domain/value_objects/`

### Add New Use Case (Application)
1. Create in `src/server/include/application/use_cases/`
2. Inject dependencies via constructor (ports)
3. Add tests with mock repositories

### Add Network Adapter (Infrastructure)
1. Create in `src/server/include/infrastructure/adapters/`
2. Implement port interfaces
3. Use Boost.Asio for async

### Add Client Scene
1. Create in `src/client/include/scenes/`
2. Inherit from `IScene`
3. Register in `SceneManager`

### Add SFML Feature
1. Implement in `src/client/lib/sfml/`
2. Expose via `IGraphicPlugin` interface
3. Keep abstraction clean

## Compiler Flags

**Debug** (sanitizers enabled):
```
-O0 -g3 -Wall -Wextra -Wpedantic
-fsanitize=address,undefined,leak
```

**Release**:
```
-O3 -Wall -Wextra -Wpedantic
```

## Important Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Root build config |
| `scripts/build.sh` | Setup vcpkg + CMake |
| `scripts/compile.sh` | Compile with filtering |
| `scripts/test.sh` | Run tests |
| `mkdocs.yml` | Documentation config |
| `Jenkinsfile` | CI/CD pipeline |
| `flake.nix` | Nix dev environment |

## Quality Standards

- Test coverage: >80%
- Zero compilation warnings
- Zero critical vulnerabilities
- 100% atomic commits
- Documentation sync with code

## Do's and Don'ts

### Do
- Read code before modifying
- Follow hexagonal layer boundaries
- Write tests for new code
- Use existing patterns
- Keep commits atomic
- Update docs when changing code

### Don't
- Import external libs in Domain layer
- Skip tests
- Make large commits
- Over-engineer solutions
- Add features not requested
- Ignore compiler warnings

## Jira Integration

**Project**: KAN (epitech-team-w5qkn5hj.atlassian.net)
**Cloud ID**: `07d07e1e-f1e8-42f8-930c-ca7af792f859`

Epics: KAN-5 to KAN-14 (10 domains)
Tickets: 79 synchronized

## Quick Debug

```bash
# Rebuild from scratch
rm -rf buildLinux && ./scripts/build.sh --platform=linux

# Check sanitizer output
ASAN_OPTIONS=detect_leaks=1 ./artifacts/r-type_server

# Run single test
./artifacts/tests/server_tests --gtest_filter="HealthTest.*"
```
