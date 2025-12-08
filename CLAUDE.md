# R-Type - Claude Code Context

## Project Overview

R-Type is a multiplayer arcade game (shoot'em up) built with C++23, using an **Hexagonal Architecture** (Ports & Adapters). The project consists of a server and client communicating via UDP (gameplay) and TCP (authentication).

## Quick Reference

| Component | Technology | Port |
|-----------|------------|------|
| Server | C++23, Boost.ASIO | UDP 4124, TCP 4123 |
| Client | C++23, SFML | - |
| Database | MongoDB | - |
| Build | CMake 3.30+, Ninja, vcpkg | - |

## Project Structure

```
rtype/
├── src/
│   ├── server/                    # Server implementation
│   │   ├── include/               # Headers
│   │   │   ├── domain/            # Business logic (entities, value objects)
│   │   │   ├── application/       # Use cases, ports (interfaces)
│   │   │   └── infrastructure/    # Adapters (network, persistence)
│   │   ├── domain/                # Domain layer implementations
│   │   ├── application/           # Application layer implementations
│   │   └── infrastructure/        # Infrastructure layer implementations
│   ├── client/                    # Client implementation (SFML)
│   └── common/                    # Shared code (protocol)
├── tests/                         # Google Test tests
├── docs/                          # MkDocs documentation
├── third_party/vcpkg/             # Package manager
├── .mcp.json                      # MCP server configuration
└── .claude/                       # Claude Code tooling
    ├── agentdb/                   # Code analysis database
    │   ├── db.sqlite              # SQLite database
    │   ├── crud.py                # CRUD operations
    │   ├── db.py                  # Database connection
    │   ├── config.py              # Configuration
    │   ├── indexer.py             # Code indexer
    │   ├── models.py              # Data models
    │   └── queries.py             # Query functions
    ├── agents/                    # Specialized analysis agents
    │   ├── analyzer.md            # Impact analysis agent
    │   ├── security.md            # Security audit agent
    │   ├── reviewer.md            # Code review agent
    │   ├── risk.md                # Risk evaluation agent
    │   └── synthesis.md           # Report synthesis agent
    ├── mcp/                       # MCP server for agentDB
    │   └── agentdb/
    │       ├── server.py          # MCP server implementation
    │       └── tools.py           # MCP tool definitions
    ├── scripts/                   # Utility scripts
    │   ├── bootstrap.py           # Initial setup
    │   ├── update.py              # Database update
    │   └── maintenance.py         # Maintenance tasks
    ├── tests/                     # Python tests for agentDB
    ├── config/                    # Configuration files
    ├── AGENTS.md                  # Agent documentation
    └── settings.json              # Claude Code settings
```

## Build Commands

```bash
# Configure (first time or after CMakeLists changes)
./scripts/build.sh                    # Linux
./scripts/build.sh --platform=windows # Windows cross-compile

# Compile
./scripts/compile.sh

# Run tests
./artifacts/server/linux/server_tests

# Run server
./artifacts/server/linux/rtype_server

# Clean build
rm -rf build*/ artifacts/
```

## MCP Configuration

The `.mcp.json` file at the project root configures the MCP server for agentDB:

```json
{
  "mcpServers": {
    "agentdb": {
      "command": "bash",
      "args": ["-c", "PYTHONPATH=\"$PWD/.claude\" AGENTDB_PATH=\"$PWD/.claude/agentdb/db.sqlite\" python -m mcp.agentdb.server"]
    }
  }
}
```

## Analysis Agents

5 specialized agents for code analysis, defined in `.claude/agents/`:

| Agent | Description | Use Case |
|-------|-------------|----------|
| **analyzer** | Impact analysis of code changes | "What's the impact of my changes?" |
| **security** | Security audit, regression detection | "Check security of this code" |
| **reviewer** | Code review, conventions check | "Review this code" |
| **risk** | Global risk evaluation | "Is it safe to merge?" |
| **synthesis** | Final report synthesis | "Summarize the analyses" |

### Agent Workflow

```
┌─────────┐ ┌──────────┐ ┌──────────┐
│ANALYZER │ │ SECURITY │ │ REVIEWER │
└────┬────┘ └────┬─────┘ └────┬─────┘
     └───────────┼────────────┘
                 ▼
           ┌─────────┐
           │  RISK   │
           └────┬────┘
                ▼
          ┌───────────┐
          │ SYNTHESIS │
          └───────────┘
```

See `.claude/AGENTS.md` for detailed agent documentation.

## AgentDB MCP Tools

**AgentDB** is a SQLite-based code analysis database with MCP server integration. Use these tools to query project context:

### Available Tools (via MCP)

| Tool | Description | Example Use |
|------|-------------|-------------|
| `get_file_context` | Full 360° view of a file (symbols, dependencies, errors, patterns) | Before modifying a file |
| `get_symbol_callers` | Find all callers of a function (recursive) | Impact analysis |
| `get_symbol_callees` | Find all functions called by a symbol | Understanding dependencies |
| `get_file_impact` | Calculate impact of modifying a file | Before refactoring |
| `get_error_history` | Historical bugs for a file/symbol/module | Identify risky areas |
| `get_patterns` | Coding patterns applicable to a file | Ensure consistency |
| `get_architecture_decisions` | ADRs applicable to a module | Understand design choices |
| `search_symbols` | Search symbols by pattern (supports wildcards) | Find functions/types |
| `get_file_metrics` | Detailed metrics (complexity, lines, activity) | Code quality |
| `get_module_summary` | Summary of a module | Module overview |

### Database Schema (Key Tables)

- **files**: File metadata, metrics, criticality, git activity
- **symbols**: Functions, classes, structs, enums with location and signatures
- **relations**: Call graph, type usage, includes relationships
- **file_relations**: Include/import dependencies between files
- **error_history**: Historical bugs and their resolutions
- **patterns**: Coding conventions and best practices
- **architecture_decisions**: ADRs (Architecture Decision Records)

### Direct SQLite Queries

```bash
# File info
sqlite3 .claude/agentdb/db.sqlite "SELECT * FROM files WHERE path LIKE '%UDPServer%'"

# Symbols in a file
sqlite3 .claude/agentdb/db.sqlite "SELECT name, kind, line_start FROM symbols WHERE file_id = X"

# File relations (includes)
sqlite3 .claude/agentdb/db.sqlite "SELECT * FROM file_relations WHERE source_file_id = X"

# Module summary
sqlite3 .claude/agentdb/db.sqlite "SELECT module, COUNT(*) FROM files GROUP BY module"
```

### AgentDB Maintenance Scripts

```bash
# Initial setup / re-index entire codebase
python .claude/scripts/bootstrap.py

# Update database with recent changes
python .claude/scripts/update.py

# Database maintenance (cleanup, optimization)
python .claude/scripts/maintenance.py

# Run agentDB tests
pytest .claude/tests/
```

## Architecture (Hexagonal)

### Layers

1. **Domain Layer** (`domain/`) - Pure business logic, no external dependencies
   - Entities: `Player`, `User`
   - Value Objects: `Position`, `Health`, `PlayerId`, `UserId`
   - Exceptions: `DomainException`, `PositionException`, etc.

2. **Application Layer** (`application/`) - Use cases, orchestration
   - Use Cases: `MovePlayerUseCase`, `LoginUseCase`
   - Ports IN: `IGameCommands`
   - Ports OUT: `IPlayerRepository`, `IUserRepository`

3. **Infrastructure Layer** (`infrastructure/`) - Technical implementations
   - Adapters IN: `UDPServer`, `TCPServer`, `CLIGameController`
   - Adapters OUT: `MongoDBConfiguration`, `MongoDBPlayerRepository`

### Key Principle: Dependency Rule

```
Infrastructure → Application → Domain → (nothing)
```

- Domain NEVER depends on Application or Infrastructure
- Application NEVER depends on Infrastructure
- Dependencies flow inward only

## Coding Conventions

### C++ Style

- **Standard**: C++23
- **Naming**:
  - Classes: `PascalCase`
  - Methods/Functions: `camelCase`
  - Private members: `_prefixedWithUnderscore`
  - Constants: `SCREAMING_SNAKE_CASE`
- **Headers**: `.hpp` for headers, `.cpp` for implementations
- **Namespaces**: Match directory structure (`domain::entities::Player`)

### Value Objects

- Always validate in constructor
- Immutable: methods return new instances
- Example: `Position::move()` returns a new `Position`

### Repositories

- Define interface in Application layer (`IPlayerRepository`)
- Implement in Infrastructure layer (`MongoDBPlayerRepository`)
- Inject dependencies via constructor

## Key Files

| Purpose | Path |
|---------|------|
| Server main | `src/server/main.cpp` |
| UDP Server | `src/server/infrastructure/adapters/in/network/UDPServer.cpp` |
| TCP Server | `src/server/infrastructure/adapters/in/network/TCPServer.cpp` |
| Player entity | `src/server/include/domain/entities/Player.hpp` |
| Position VO | `src/server/include/domain/value_objects/Position.hpp` |
| Move use case | `src/server/include/application/use_cases/MovePlayerUseCase.hpp` |
| Binary protocol | `src/common/protocol/` |

## Testing

```bash
# Run all tests
./artifacts/server/linux/server_tests

# Run specific test
./artifacts/server/linux/server_tests --gtest_filter=PlayerTest.*

# With verbose output
./artifacts/server/linux/server_tests --gtest_verbose
```

## Dependencies (vcpkg)

- **boost-asio**: Async networking
- **gtest**: Testing framework
- **mongo-cxx-driver**: MongoDB driver
- **nlohmann-json**: JSON parsing
- **sfml**: Client graphics (SFML)

## Git Conventions

Commit format: `TYPE: Description`

| Type | Usage |
|------|-------|
| FEAT | New feature |
| FIX | Bug fix |
| DOCS | Documentation |
| STYLE | Formatting, no code change |
| REFACTOR | Code refactoring |
| TEST | Adding tests |
| CHORE | Maintenance, build, deps |
| BUILD | Build system changes |

## Documentation

```bash
# Serve docs locally
mkdocs serve

# Or with Docker
cd ci_cd/docker && docker-compose -f docker-compose.docs.yml up
```

Access at http://localhost:8000

## Environment

- **OS**: Linux (Ubuntu 22.04 recommended), Windows via WSL2
- **Compiler**: GCC 11+ or Clang 15+ with C++23 support
- **CMake**: 3.30+
- **Build**: Ninja (faster) or Make

## Debugging

```bash
# GDB
gdb ./artifacts/server/linux/rtype_server

# Valgrind (memory leaks)
valgrind --leak-check=full ./artifacts/server/linux/rtype_server
```

Debug builds include AddressSanitizer by default.

## Common Issues

| Problem | Solution |
|---------|----------|
| `CMake version too old` | Install CMake 3.30+ from official site |
| `undefined reference to boost::...` | Reconfigure: `rm -rf build && ./scripts/build.sh` |
| `vcpkg dependencies missing` | Run `./third_party/vcpkg/vcpkg install` |
| `std::ranges not declared` | Upgrade GCC to 11+ |

## Notes for Claude

1. **Always use agentDB MCP tools** to understand file context before modifications
2. **Follow Hexagonal Architecture** - never add infrastructure code to domain
3. **Value Objects are immutable** - return new instances, don't modify
4. **Separate .hpp/.cpp** - declarations in headers, implementations in source files
5. **Check file relations** before refactoring to understand impact
6. **Review error history** for files with past issues
7. **Use analysis agents** for code reviews:
   - `analyzer` for impact analysis before changes
   - `security` for security-sensitive code
   - `reviewer` for code quality checks
   - `risk` before merging to evaluate risk
   - `synthesis` for final reports
