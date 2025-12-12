# SECURITY Report

## Summary

| Metric | Value |
|--------|-------|
| **Security Score** | 55/100 |
| **Blocker Issues** | 1 |
| **Critical Issues** | 3 |
| **Major Issues** | 3 |
| **Medium Issues** | 2 |
| **Minor Issues** | 1 |
| **Regressions** | 0 |

## Vulnerabilities Found

### [Blocker] SEC-001: Password Logged to Stdout (CWE-200)
- **File**: src/server/application/use_cases/auth/Login.cpp:21
- **Category**: Security
- **isBug**: false

### [Critical] SEC-002: Hardcoded Credentials (CWE-798)
- **File**: src/client/src/network/TCPClient.cpp:195
- **Category**: Security
- **isBug**: false

### [Critical] SEC-003: Weak Password Hashing (CWE-916)
- **File**: src/server/domain/value_objects/user/utils/PasswordUtils.cpp:11
- **Category**: Security
- **isBug**: false

### [Critical] SEC-004: Password in Exception Message (CWE-209)
- **File**: src/server/include/domain/exceptions/player/PlayerPasswordException.hpp:23
- **Category**: Security
- **isBug**: false

### [Major] SEC-005: Missing payload_size Validation (CWE-20)
- **File**: src/server/infrastructure/adapters/in/network/TCPServer.cpp:52
- **Category**: Security
- **isBug**: false

### [Major] SEC-006: No Rate Limiting on Auth (CWE-307)
- **File**: src/server/application/use_cases/auth/Login.cpp
- **Category**: Security
- **isBug**: false

### [Major] SEC-007: No TLS Encryption (CWE-319)
- **Files**: Network layer
- **Category**: Security
- **isBug**: false

### [Medium] SEC-008: Unbounded Accumulator Growth (CWE-400)
- **File**: src/server/infrastructure/adapters/in/network/TCPServer.cpp:44
- **Category**: Security
- **isBug**: false

### [Medium] SEC-009: Command Buffer Extra Data (CWE-126)
- **File**: src/server/infrastructure/adapters/in/network/TCPServer.cpp:105
- **Category**: Security
- **isBug**: false

### [Minor] SEC-010: Debug Output in Production
- **Files**: Multiple
- **Category**: CodeQuality
- **isBug**: false

## AgentDB Data Used
- error_history: 6 files (0 bugs found)
- file_context: 2 files
- patterns (security): 3 patterns loaded
- list_critical_files: 28 security-sensitive files
