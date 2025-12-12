# ANALYZER Report

## Summary

| Metric | Value |
|--------|-------|
| **Impact Score** | 72/100 |
| **Impact Level** | GLOBAL |
| **Total Modules** | 7 (server, client, ECS, common, tests, .claude, ci_cd) |
| **Total Files** | 190 |
| **Total Lines of Code** | ~16,431 |
| **Critical Files** | 49 |
| **Security-Sensitive Files** | 28 |

## Critical Files Analysis

### 1. src/server/main.cpp (Entry Point - CRITICAL)
- **Impact Level**: GLOBAL
- **Lines**: 24/44
- **Complexity Max**: 3

### 2. src/server/infrastructure/adapters/in/network/UDPServer.cpp
- **Impact Level**: MODULE
- **Lines**: 77/100
- **Complexity Max**: 10

### 3. src/server/infrastructure/adapters/in/network/TCPServer.cpp
- **Impact Level**: MODULE
- **Lines**: 99/140
- **Complexity Max**: 7

### 4. src/ECS/Registry.hpp (ECS Core)
- **Impact Level**: GLOBAL
- **Lines**: 43/123
- **Complexity Max**: 23 (highest)

### 5. src/client/src/core/Engine.cpp (Client Core - CRITICAL)
- **Impact Level**: GLOBAL
- **Lines**: 30/54
- **Impact**: 13 files, 7 critical

## Findings

| ID | Severity | Category | isBug | Description |
|----|----------|----------|-------|-------------|
| ANA-001 | Critical | Reliability | false | Engine.cpp - 13 files impacted, 7 critical |
| ANA-002 | Major | Maintainability | false | Registry.hpp complexity 23 |
| ANA-003 | Major | Observability | false | Symbol callers not indexed |
| ANA-004 | Warning | Security | false | Protocol.hpp security-sensitive |
| ANA-005 | Info | Quality | false | Low test coverage |

## AgentDB Data Used
- file_context: 5 files
- file_impact: 4 files
- file_metrics: 3 files
- symbol_callers: 4 symbols (all empty)
- module_summary: 4 modules
