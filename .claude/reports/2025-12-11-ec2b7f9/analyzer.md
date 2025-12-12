# ANALYZER Report

**Date** : 2025-12-11
**Commit** : ec2b7f9
**Branch** : main
**Type** : Analyse complete du depot (mode --all)

---

## AgentDB Data Used

| Query | File/Symbol | Status | Results |
|-------|-------------|--------|---------|
| file_context | src/main.c | OK | 6 symbols |
| file_context | src/config/config.c | OK | 16 symbols |
| file_context | src/config/config.h | OK | 15 symbols |
| file_context | src/server/udp_server.c | OK | 10 symbols |
| file_context | src/server/udp_server.h | OK | 9 symbols |
| file_context | src/server/connection.c | OK | 19 symbols |
| file_context | src/server/connection.h | OK | 1 symbol |
| file_context | src/shell/command.c | OK | 7 symbols |
| file_context | src/shell/command.h | OK | 1 symbol |
| file_context | src/file_manager/file_ops.c | OK | 13 symbols |
| file_context | src/file_manager/file_ops.h | OK | 1 symbol |
| file_context | src/utils/memory.c | OK | 14 symbols |
| file_context | src/utils/memory.h | OK | 6 symbols |
| file_context | src/utils/string_utils.c | OK | 12 symbols |
| file_context | src/utils/string_utils.h | OK | 1 symbol |
| file_context | src/utils/logger.c | OK | 26 symbols |
| file_context | src/utils/logger.h | OK | 9 symbols |
| file_context | src/utils/cache.c | OK | 24 symbols |
| file_context | src/utils/cache.h | OK | 1 symbol |
| file_context | src/utils/crypto.c | OK | 15 symbols |
| file_context | src/utils/crypto.h | OK | 1 symbol |
| file_context | src/utils/validator.c | OK | 12 symbols |
| file_context | src/utils/validator.h | OK | 1 symbol |
| list_modules | - | OK | 7 modules |
| list_critical_files | - | OK | 15 critical files |
| symbol_callers | main | OK | 0 callers |
| symbol_callers | config_init | OK | 2 callers |
| symbol_callers | config_load | OK | 2 callers |
| symbol_callers | udp_server_init | OK | 1 caller |
| symbol_callers | handle_connection | OK | 1 caller |
| symbol_callers | execute_command | OK | 3 callers |
| symbol_callers | file_read | OK | 3 callers |
| symbol_callers | cache_get | OK | 1 caller |
| symbol_callers | crypto_encrypt | OK | 1 caller |
| symbol_callers | validate_email | OK | 0 callers |
| symbol_callers | str_dup | OK | 11 callers |
| symbol_callers | logger_init | OK | 0 callers |
| symbol_callers | pool_alloc | OK | 0 callers |
| file_impact | src/main.c | OK | 1 file impacted |
| file_impact | src/config/config.c | OK | 1 file impacted |
| file_impact | src/server/connection.c | OK | 1 file impacted |
| file_impact | src/utils/validator.c | OK | 0 files impacted |
| file_impact | src/utils/crypto.c | OK | 0 files impacted |

**Total Queries**: 43 | **Successful**: 43 | **Failed**: 0

---

## Summary

| Metric | Value |
|--------|-------|
| **Score** | 35/100 |
| **Impact Level** | GLOBAL |
| **C Source Files** | 22 |
| **Total Functions** | 120+ |
| **Critical Files** | 10 (C only) |
| **Security-Sensitive Files** | 10 (C only) |
| **Total Callers Found** | 25 |
| **Modules** | 7 |

---

## Module Summary

| Module | Files | Lines | Critical Files | Security Sensitive |
|--------|-------|-------|----------------|-------------------|
| utils | 12 | 911 | 6 | Yes |
| server | 4 | 311 | 1 | Yes |
| config | 2 | 119 | 2 | Yes |
| file_manager | 2 | 162 | 0 | No |
| shell | 2 | 70 | 0 | No |
| main.c | 1 | 148 | 1 | No |

---

## Critical Files Analysis

### CRITICAL src/main.c

| Attribute | Value |
|-----------|-------|
| **Module** | main.c |
| **Lines** | 181 (148 code, 13 comments) |
| **Complexity Max** | 47 |
| **Is Critical** | Yes |
| **Security Sensitive** | No |

**Symbols (6)**:
- `g_server` (variable, line 16)
- `g_config` (variable, line 17)
- `g_initialized` (variable, line 18)
- `main` (function, line 20)
- `unused_function` (function, line 166)
- `processUserInput` (function, line 172)

**Dependencies (includes)**:
- src/server/udp_server.h
- src/file_manager/file_ops.h
- src/config/config.h
- src/shell/command.h
- src/utils/memory.h
- src/utils/string_utils.h

**Impact Graph**:
```
src/main.c [CRITICAL]
|-- includes --> src/server/udp_server.h
|-- includes --> src/file_manager/file_ops.h  
|-- includes --> src/config/config.h [CRITICAL]
|-- includes --> src/shell/command.h
|-- includes --> src/utils/memory.h
|-- includes --> src/utils/string_utils.h [CRITICAL]
```

---

### CRITICAL src/config/config.c

| Attribute | Value |
|-----------|-------|
| **Module** | config |
| **Lines** | 145 (106 code, 17 comments) |
| **Complexity Max** | 20 |
| **Is Critical** | Yes |
| **Security Sensitive** | Yes |

**Symbols (16)** - Including hardcoded credentials:
- `ADMIN_USERNAME` (variable, line 12) - HARDCODED
- `ADMIN_PASSWORD` (variable, line 13) - HARDCODED
- `BACKUP_PASSWORD` (variable, line 14) - HARDCODED
- `ROOT_TOKEN` (variable, line 15) - HARDCODED
- `DATABASE_CONNECTION_STRING` (variable, line 17) - HARDCODED
- `AWS_ACCESS_KEY` (variable, line 19) - HARDCODED
- `AWS_SECRET_KEY` (variable, line 20) - HARDCODED
- `PRIVATE_KEY` (variable, line 21) - HARDCODED
- `config_init` (function, line 24)
- `config_authenticate` (function, line 36)
- `config_get_api_key` (function, line 55)
- `config_load_from_db` (function, line 60) - SQL queries
- `config_load` (function, line 68)
- `config_free` (function, line 97)
- `config_debug_dump` (function, line 105) - Logs sensitive data
- `config_expand_env` (function, line 117)

**Impact Graph**:
```
config_init (src/config/config.c:24) [CRITICAL]
+-- [L1] main (.claude/mcp/agentdb/server.py:23)
+-- [L1] config_authenticate (src/config/config.c:71) [CRITICAL]

config_load (src/config/config.c:68) [CRITICAL]
+-- [L1] main (.claude/mcp/agentdb/server.py:114)
+-- [L1] config_authenticate (src/config/config.c:68) [CRITICAL]
```

---

### CRITICAL src/server/connection.c

| Attribute | Value |
|-----------|-------|
| **Module** | server |
| **Lines** | 247 (185 code, 24 comments) |
| **Complexity Max** | 41 |
| **Is Critical** | Yes |
| **Security Sensitive** | Yes |

**Symbols (19)**:
- `MAX_CONNECTIONS` (macro, line 16)
- `RECV_BUFFER_SIZE` (macro, line 17)
- `Connection` (struct/typedef, line 19-26)
- `connections` (variable, line 28)
- `conn_mutex` (variable, line 29)
- `conn_count` (variable, line 30)
- `find_free_slot` (function, line 33)
- `parse_request` (function, line 43)
- `handle_connection` (function, line 62) - Thread handler
- `accept_connection` (function, line 183)
- `close_all_connections` (function, line 219)
- `dump_connections` (function, line 234)

**Impact Graph**:
```
handle_connection (src/server/connection.c:62) [CRITICAL]
+-- [L1] find_free_slot (src/server/connection.c:62) [CRITICAL]

src/server/connection.c
+-- impacts --> src/server/udp_server.c (via socket symbol)
```

---

### CRITICAL src/utils/string_utils.c

| Attribute | Value |
|-----------|-------|
| **Module** | utils |
| **Lines** | 190 (132 code, 21 comments) |
| **Complexity Max** | 25 |
| **Is Critical** | Yes |
| **Security Sensitive** | Yes |

**Symbols (12)**:
- `str_dup` (function, line 13) - 11 callers!
- `str_concat` (function, line 25)
- `str_safe_copy` (function, line 42)
- `str_equals_ignore_case` (function, line 47)
- `str_trim` (function, line 64)
- `str_to_upper` (function, line 87)
- `str_to_int` (function, line 95)
- `str_free_all` (function, line 100)
- `str_format` (function, line 110)
- `str_process_complex` (function, line 117)
- `str_build` (function, line 128)
- `str_tokenize_and_process` (function, line 153)

**Impact Graph** - str_dup (HIGH IMPACT - 11 callers):
```
str_dup (src/utils/string_utils.c:13) [CRITICAL - 11 CALLERS]
+-- [L1] config_init (src/config/config.c:25,27,28,29) [CRITICAL] x4
+-- [L1] config_authenticate (src/config/config.c:81,85,87,143) [CRITICAL] x4
+-- [L1] str_concat (src/utils/string_utils.c:118) [CRITICAL]
+-- [L1] str_build (src/utils/string_utils.c:158,168) [CRITICAL] x2
```

---

### CRITICAL src/utils/validator.c

| Attribute | Value |
|-----------|-------|
| **Module** | utils |
| **Lines** | 251 (180 code, 21 comments) |
| **Complexity Max** | 47 |
| **Is Critical** | Yes |
| **Security Sensitive** | Yes |

**Symbols (12)**:
- `MAX_INPUT_SIZE` (macro, line 12)
- `validate_email` (function, line 15)
- `validate_username` (function, line 34)
- `sanitize_html` (function, line 49)
- `validate_int_range` (function, line 90)
- `is_safe_sql` (function, line 105) - SQL injection protection
- `validate_path` (function, line 123) - Path traversal protection
- `validate_json_field` (function, line 140)
- `url_decode` (function, line 174)
- `validate_password` (function, line 204)
- `check_bounds` (function, line 226)
- `validate_command` (function, line 235) - Command injection protection

---

### CRITICAL src/utils/crypto.c

| Attribute | Value |
|-----------|-------|
| **Module** | utils |
| **Lines** | 193 (120 code, 24 comments) |
| **Complexity Max** | 18 |
| **Is Critical** | Yes |
| **Security Sensitive** | Yes |

**Symbols (15)**:
- `BLOCK_SIZE` (macro, line 11)
- `KEY_SIZE` (macro, line 12)
- `crypto_encrypt` (function, line 15)
- `crypto_decrypt` (function, line 34)
- `crypto_hash` (function, line 40)
- `crypto_hash_password` (function, line 54)
- `crypto_verify_password` (function, line 68)
- `crypto_random_bytes` (function, line 78)
- `crypto_generate_token` (function, line 85)
- `crypto_secure_compare` (function, line 99)
- `crypto_derive_key` (function, line 120)
- `crypto_to_hex` (function, line 141)
- `crypto_from_hex` (function, line 153)
- `crypto_sign` (function, line 166)
- `crypto_verify_signature` (function, line 185)

**Impact Graph**:
```
crypto_encrypt (src/utils/crypto.c:15) [CRITICAL]
+-- [L1] crypto_decrypt (src/utils/crypto.c:36) [CRITICAL]
```

---

### CRITICAL src/utils/cache.c

| Attribute | Value |
|-----------|-------|
| **Module** | utils |
| **Lines** | 289 (204 code, 27 comments) |
| **Complexity Max** | 33 |
| **Is Critical** | Yes |
| **Security Sensitive** | Yes |

**Symbols (24)**:
- `CACHE_SIZE` (macro, line 12)
- `MAX_KEY_LEN` (macro, line 13)
- `MAX_VALUE_LEN` (macro, line 14)
- `CacheEntry` (struct/typedef)
- `cache_table` (variable, line 23)
- `cache_lock` (variable, line 24)
- `cache_initialized` (variable, line 25)
- `cache_hash` (function, line 28)
- `cache_init` (function, line 37)
- `cache_get` (function, line 50)
- `cache_set` (function, line 74)
- `cache_delete` (function, line 111)
- `cache_clear` (function, line 149)
- `cache_stats` (function, line 167)
- `cache_evict_expired` (function, line 184)
- `cache_save` (function, line 221)
- `cache_load` (function, line 244)
- `cache_copy_value` (function, line 271)
- `cache_destroy` (function, line 284)

---

## Non-Critical Files Analysis

### src/server/udp_server.c

| Attribute | Value |
|-----------|-------|
| **Module** | server |
| **Lines** | 154 (110 code, 22 comments) |
| **Complexity Max** | 35 |
| **Is Critical** | No |
| **Security Sensitive** | No |

**Symbols (10)**:
- `TEMP_BUFFER_SIZE` (macro)
- `udp_server_init` (function) - 1 caller
- `udp_server_start` (function)
- `udp_server_process_request` (function)
- `udp_server_send_response` (function)
- `udp_server_read_input` (function)
- `udp_server_process_batch` (function)
- `udp_server_cleanup` (function)
- `udp_server_log` (function)
- `udp_server_get_command` (function)

---

### src/shell/command.c

| Attribute | Value |
|-----------|-------|
| **Module** | shell |
| **Lines** | 98 (65 code, 16 comments) |
| **Complexity Max** | 12 |
| **Is Critical** | No |
| **Security Sensitive** | No |

**Symbols (7)**:
- `execute_command` (function, line 13) - 3 callers - Potential command injection
- `execute_with_args` (function, line 18)
- `run_script` (function, line 25)
- `admin_execute` (function, line 44) - Elevated privileges
- `debug_exec` (function, line 56)
- `evaluate_expression` (function, line 63)
- `batch_execute` (function, line 70)

---

### src/file_manager/file_ops.c

| Attribute | Value |
|-----------|-------|
| **Module** | file_manager |
| **Lines** | 225 (156 code, 22 comments) |
| **Complexity Max** | 22 |
| **Is Critical** | No |
| **Security Sensitive** | No |

**Symbols (13)**:
- `g_last_error` (variable)
- `file_read` (function) - 3 callers
- `file_write` (function)
- `file_delete` (function)
- `file_copy` (function)
- `file_include` (function) - Potential path traversal
- `load_user_config` (function)
- `create_temp_file` (function)
- `safe_read` (function)
- `secure_file_read` (function)
- `process_file_lines` (function)
- `file_append` (function)
- `file_read_into` (function)

---

### src/utils/memory.c

| Attribute | Value |
|-----------|-------|
| **Module** | utils |
| **Lines** | 126 (80 code, 20 comments) |
| **Complexity Max** | 13 |
| **Is Critical** | No |
| **Security Sensitive** | No |

**Symbols (14)**:
- `g_allocations` (variable)
- `g_alloc_count` (variable)
- `DEFAULT_POOL_SIZE` (macro)
- `pool_init` (function)
- `pool_alloc` (function) - 0 callers
- `pool_free` (function)
- `global_alloc` (function)
- `global_free` (function)
- `safe_array_alloc` (function)
- `resize_buffer` (function)
- `process_data_buffer` (function)
- `cleanup_resources` (function)
- `alloc_uninitialized` (function)
- `alloc_zeroed` (function)

---

### src/utils/logger.c

| Attribute | Value |
|-----------|-------|
| **Module** | utils |
| **Lines** | 205 (133 code, 26 comments) |
| **Complexity Max** | 17 |
| **Is Critical** | No |
| **Security Sensitive** | No |

**Symbols (26)**:
- `MAX_LOG_SIZE` (macro)
- `LOG_BUFFER_SIZE` (macro)
- `LogEntry` (struct/typedef)
- `log_file` (variable)
- `log_level` (variable)
- `log_mutex` (variable)
- `logger_initialized` (variable)
- `log_buffer` (variable)
- `buffer_index` (variable)
- `logger_init` (function) - 0 callers
- `logger_set_level` (function)
- `do_log` (function)
- `log_debug` (function)
- `log_info` (function)
- `log_warn` (function)
- `log_error` (function)
- `logger_get_recent` (function)
- `logger_flush` (function)
- `logger_rotate` (function)
- `log_with_context` (function)
- `logger_cleanup` (function)
- `log_hex` (function)

---

## Findings

### [Critical] ANA-001 : Hardcoded Credentials in config.c

- **Category** : Security
- **File** : src/config/config.c:12-21
- **Symbols** : ADMIN_USERNAME, ADMIN_PASSWORD, AWS_SECRET_KEY, PRIVATE_KEY, etc.
- **Problem** : 8 hardcoded credentials/secrets in source code
- **Time Estimate** : ~60 min
- **Blocking** : No (compile OK but security issue)
- **isBug** : No (design flaw, not runtime bug)

### [Critical] ANA-002 : High Complexity in validator.c

- **Category** : Maintainability
- **File** : src/utils/validator.c
- **Problem** : Cyclomatic complexity of 47 (threshold: 20)
- **Time Estimate** : ~45 min to refactor
- **Blocking** : No
- **isBug** : No

### [Critical] ANA-003 : High Complexity in main.c

- **Category** : Maintainability
- **File** : src/main.c
- **Problem** : Cyclomatic complexity of 47 (threshold: 20)
- **Time Estimate** : ~45 min to refactor
- **Blocking** : No
- **isBug** : No

### [Critical] ANA-004 : High Complexity in connection.c

- **Category** : Maintainability
- **File** : src/server/connection.c
- **Problem** : Cyclomatic complexity of 41 (threshold: 20)
- **Time Estimate** : ~30 min to refactor
- **Blocking** : No
- **isBug** : No

### [Major] ANA-005 : str_dup has 11 callers across critical files

- **Category** : Reliability
- **File** : src/utils/string_utils.c:13
- **Symbol** : str_dup
- **Problem** : Function widely used in critical files (config.c, string_utils.c)
- **Time Estimate** : ~15 min to review
- **Blocking** : No
- **isBug** : No

### [Major] ANA-006 : Debug function exposes sensitive data

- **Category** : Security
- **File** : src/config/config.c:105
- **Symbol** : config_debug_dump
- **Problem** : May log sensitive configuration data
- **Time Estimate** : ~15 min
- **Blocking** : No
- **isBug** : No

### [Minor] ANA-007 : Unused functions detected

- **Category** : Maintainability
- **File** : src/main.c:166
- **Symbol** : unused_function
- **Problem** : Dead code in main.c
- **Time Estimate** : ~5 min
- **Blocking** : No
- **isBug** : No

### [Minor] ANA-008 : Missing callers for utility functions

- **Category** : Maintainability
- **Files** : src/utils/logger.c, src/utils/memory.c
- **Symbols** : logger_init, pool_alloc, validate_email
- **Problem** : Several utility functions have 0 callers (unused or new)
- **Time Estimate** : ~10 min to review
- **Blocking** : No
- **isBug** : No

---

## Dependency Graph

```
src/main.c [CRITICAL, complexity=47]
|
+-- src/server/udp_server.h
|   +-- src/server/udp_server.c [complexity=35]
|
+-- src/file_manager/file_ops.h
|   +-- src/file_manager/file_ops.c [complexity=22]
|
+-- src/config/config.h [CRITICAL, SECURITY]
|   +-- src/config/config.c [CRITICAL, SECURITY, complexity=20]
|
+-- src/shell/command.h
|   +-- src/shell/command.c [complexity=12]
|
+-- src/utils/memory.h
|   +-- src/utils/memory.c [complexity=13]
|
+-- src/utils/string_utils.h
    +-- src/utils/string_utils.c [CRITICAL, SECURITY, complexity=25]

src/server/connection.c [CRITICAL, SECURITY, complexity=41]
+-- src/server/connection.h

src/utils/cache.c [CRITICAL, SECURITY, complexity=33]
+-- src/utils/cache.h

src/utils/crypto.c [CRITICAL, SECURITY, complexity=18]
+-- src/utils/crypto.h [CRITICAL, SECURITY]

src/utils/validator.c [CRITICAL, SECURITY, complexity=47]
+-- src/utils/validator.h [CRITICAL, SECURITY]

src/utils/logger.c [complexity=17]
+-- src/utils/logger.h
```

---

## Recommendations

1. **[CRITICAL]** Remove hardcoded credentials from src/config/config.c - Use environment variables or secure vault
2. **[CRITICAL]** Refactor src/utils/validator.c to reduce complexity (47 -> <20)
3. **[CRITICAL]** Refactor src/main.c to reduce complexity (47 -> <20)
4. **[HIGH]** Review src/utils/string_utils.c:str_dup - 11 callers in critical code
5. **[HIGH]** Review config_debug_dump for sensitive data exposure
6. **[MEDIUM]** Refactor src/server/connection.c complexity (41 -> <20)
7. **[LOW]** Remove unused_function from src/main.c
8. **[LOW]** Review unused utility functions (logger_init, pool_alloc, validate_email)

---

## Score Calculation

```
Base Score: 100

Penalties Applied:
- Critical file modified (config.c, config.h): -15 x 2 = -30
- Critical file modified (connection.c): -15
- Critical file modified (validator.c, validator.h): -15 x 2 = -30 (capped)
- Critical file modified (crypto.c, crypto.h): -15 x 2 (already counted)
- Critical file modified (string_utils.c): -15 (already counted)
- Critical file modified (cache.c): -15 (already counted)
- Critical file modified (main.c): -15 (already counted)
- Total critical files: 10 -> capped at -45
- Impact GLOBAL (cross-module dependencies): -20
- More than 10 total callers for str_dup (11): -10

Final Score: 100 - 45 - 20 = 35/100
```

---

## JSON Output (for synthesis)

```json
{
  "agent": "analyzer",
  "score": 35,
  "impact_level": "GLOBAL",
  "files_analyzed": 22,
  "functions_analyzed": 120,
  "total_callers": 25,
  "critical_files_count": 10,
  "security_sensitive_count": 10,
  "modules": 7,
  "findings": [
    {
      "id": "ANA-001",
      "severity": "Critical",
      "category": "Security",
      "isBug": false,
      "file": "src/config/config.c",
      "line": 12,
      "symbol": "ADMIN_USERNAME,ADMIN_PASSWORD,etc",
      "message": "Hardcoded credentials in source code",
      "blocking": false,
      "time_estimate_min": 60
    },
    {
      "id": "ANA-002",
      "severity": "Critical",
      "category": "Maintainability",
      "isBug": false,
      "file": "src/utils/validator.c",
      "line": 1,
      "symbol": null,
      "message": "Cyclomatic complexity 47 exceeds threshold 20",
      "blocking": false,
      "time_estimate_min": 45
    },
    {
      "id": "ANA-003",
      "severity": "Critical",
      "category": "Maintainability",
      "isBug": false,
      "file": "src/main.c",
      "line": 1,
      "symbol": null,
      "message": "Cyclomatic complexity 47 exceeds threshold 20",
      "blocking": false,
      "time_estimate_min": 45
    },
    {
      "id": "ANA-004",
      "severity": "Critical",
      "category": "Maintainability",
      "isBug": false,
      "file": "src/server/connection.c",
      "line": 1,
      "symbol": null,
      "message": "Cyclomatic complexity 41 exceeds threshold 20",
      "blocking": false,
      "time_estimate_min": 30
    },
    {
      "id": "ANA-005",
      "severity": "Major",
      "category": "Reliability",
      "isBug": false,
      "file": "src/utils/string_utils.c",
      "line": 13,
      "symbol": "str_dup",
      "message": "Function has 11 callers in critical files",
      "blocking": false,
      "time_estimate_min": 15
    },
    {
      "id": "ANA-006",
      "severity": "Major",
      "category": "Security",
      "isBug": false,
      "file": "src/config/config.c",
      "line": 105,
      "symbol": "config_debug_dump",
      "message": "Debug function may expose sensitive data",
      "blocking": false,
      "time_estimate_min": 15
    },
    {
      "id": "ANA-007",
      "severity": "Minor",
      "category": "Maintainability",
      "isBug": false,
      "file": "src/main.c",
      "line": 166,
      "symbol": "unused_function",
      "message": "Dead code detected",
      "blocking": false,
      "time_estimate_min": 5
    },
    {
      "id": "ANA-008",
      "severity": "Minor",
      "category": "Maintainability",
      "isBug": false,
      "file": "src/utils/logger.c",
      "line": 31,
      "symbol": "logger_init",
      "message": "Utility function has 0 callers",
      "blocking": false,
      "time_estimate_min": 10
    }
  ],
  "agentdb_queries": {
    "file_context": {"status": "ok", "count": 22},
    "list_modules": {"status": "ok", "count": 7},
    "list_critical_files": {"status": "ok", "count": 15},
    "symbol_callers": {"status": "ok", "count": 13},
    "file_impact": {"status": "ok", "count": 5}
  },
  "critical_files": [
    "src/main.c",
    "src/config/config.c",
    "src/config/config.h",
    "src/server/connection.c",
    "src/utils/string_utils.c",
    "src/utils/validator.c",
    "src/utils/validator.h",
    "src/utils/crypto.c",
    "src/utils/crypto.h",
    "src/utils/cache.c"
  ],
  "high_complexity_files": [
    {"file": "src/utils/validator.c", "complexity": 47},
    {"file": "src/main.c", "complexity": 47},
    {"file": "src/server/connection.c", "complexity": 41},
    {"file": "src/server/udp_server.c", "complexity": 35},
    {"file": "src/utils/cache.c", "complexity": 33}
  ]
}
```
