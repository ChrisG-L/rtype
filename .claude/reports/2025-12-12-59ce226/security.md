## SECURITY Report

### AgentDB Data Used

| Query | Status | Results |
|-------|--------|---------|
| error_history (15 files) | OK | 0 bugs found (project new or no history) |
| file_context | OK | 3 security_sensitive files identified |
| patterns | OK | 3 security patterns loaded |
| symbol_callers | OK | sendLoginData: 2 callers, hashPassword: 3 callers |
| list_critical_files | OK | 50 critical files listed |

### Summary

- **Score** : 70/100 (Major issues found)
- **Vulnerabilities** : 4
- **Regressions** : 0
- **Severite max** : Major
- **CWEs references** : CWE-257, CWE-916, CWE-319, CWE-1333

### Bug History Analysis

| Bug ID | Date | Type | Severity | Status | Relevant? |
|--------|------|------|----------|--------|-----------|
| (none) | - | - | - | - | No bug history found |

**Note**: No error_history data found for any of the 15 modified files. This is expected for a new project or if the error tracking has not been populated yet.

### Vulnerabilities

#### [Major] SEC-001 : Password Hashing Uses SHA256 Without Salt (CWE-916)

- **Category** : Security
- **File** : /home/simia/Dev/third_year/rtype/src/server/domain/value_objects/user/utils/PasswordUtils.cpp:11
- **Function** : `hashPassword()`
- **isBug** : No (vulnerability, application continues to function)

**Code actuel** :
```cpp
std::string hashPassword(std::string password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
        << static_cast<int>(hash[i]);
    }
    return ss.str();
}
```

**Probleme** : 
- SHA256 alone without salt is vulnerable to rainbow table attacks
- No key derivation function (like bcrypt, scrypt, or Argon2)
- Passwords can be cracked if database is compromised

**Correction suggeree** :
```cpp
#include <openssl/evp.h>
#include <openssl/rand.h>

std::pair<std::string, std::string> hashPassword(const std::string& password) {
    // Generate random salt
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));
    
    // Use PBKDF2 with high iteration count
    unsigned char hash[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                      salt, sizeof(salt),
                      100000,  // iterations
                      EVP_sha256(),
                      sizeof(hash), hash);
    
    // Return salt and hash (both encoded)
    return {encode_base64(salt, 16), encode_base64(hash, 32)};
}
```

- **Temps estime** : ~45 min
- **Bloquant** : Non (mais recommande avant production)
- **Reference** : https://cwe.mitre.org/data/definitions/916.html

---

#### [Major] SEC-002 : Passwords Transmitted in Clear Text Over Network (CWE-319)

- **Category** : Security
- **File** : /home/simia/Dev/third_year/rtype/src/client/src/network/TCPClient.cpp:236-270
- **Function** : `sendLoginData()`, `sendRegisterData()`
- **isBug** : No (vulnerability, application continues to function)

**Code actuel** :
```cpp
void TCPClient::sendLoginData(const std::string& username, const std::string& password) {
    LoginMessage login;
    std::strncpy(login.username, username.c_str(), sizeof(login.username) - 1);
    login.username[sizeof(login.username) - 1] = '\0';
    std::strncpy(login.password, password.c_str(), sizeof(login.password) - 1);  // Clear text!
    login.password[sizeof(login.password) - 1] = '\0';
    // ... sent over TCP without encryption
}
```

**Probleme** :
- Passwords are sent in clear text over TCP
- No TLS/SSL encryption on the socket
- Network sniffing can capture credentials

**Correction suggeree** :
- Option 1: Use TLS (boost::asio::ssl::stream) for TCP connections
- Option 2: Hash password on client side before sending (less secure, but better)
- Option 3: Implement challenge-response authentication

- **Temps estime** : ~2-4 hours (TLS implementation)
- **Bloquant** : Non (mais critique pour production)
- **Reference** : https://cwe.mitre.org/data/definitions/319.html

---

#### [Medium] SEC-003 : Password Stored in Memory After Use (CWE-316)

- **Category** : Security
- **File** : /home/simia/Dev/third_year/rtype/src/client/src/network/TCPClient.cpp:223-234
- **Function** : `setLoginCredentials()`, `setRegisterCredentials()`
- **isBug** : No (vulnerability, application continues to function)

**Code actuel** :
```cpp
void TCPClient::setLoginCredentials(const std::string& username, const std::string& password) {
    std::scoped_lock lock(_mutex);
    _pendingUsername = username;
    _pendingPassword = password;  // Password remains in memory
}
```

**Probleme** :
- `_pendingPassword` stays in memory indefinitely
- Should be cleared after use with `memset` or secure_zero
- Memory dumps could expose credentials

**Correction suggeree** :
```cpp
void TCPClient::clearCredentials() {
    std::scoped_lock lock(_mutex);
    // Securely clear password from memory
    volatile char* p = &_pendingPassword[0];
    for (size_t i = 0; i < _pendingPassword.size(); ++i) {
        p[i] = 0;
    }
    _pendingPassword.clear();
    _pendingUsername.clear();
    _pendingEmail.clear();
}

// Call after successful authentication
```

- **Temps estime** : ~15 min
- **Bloquant** : Non
- **Reference** : https://cwe.mitre.org/data/definitions/316.html

---

#### [Minor] SEC-004 : Debug Information Removed - Good Practice

- **Category** : Security (Improvement)
- **Files** : Multiple files
- **isBug** : No (positive change)

**Observation** :
The diff shows removal of debug statements that were logging sensitive information:
- `std::cout << "password: " << password << std::endl;` removed from Login.cpp
- `std::cout << "User name not found" << std::endl;` removed (timing attack info)
- `std::cout << "Wrong password" << std::endl;` removed (timing attack info)

This is a **positive security improvement** that:
- Prevents password logging
- Reduces information leakage about authentication failures
- Makes timing attacks more difficult

**Status** : APPROVED - Good security practice

---

### Security Patterns Check

| Pattern | Status | Details |
|---------|--------|---------|
| security_input_validation | WARN | Protocol buffers validated for size, but no content validation |
| security_sensitive_data | WARN | Passwords not cleared from memory after use |
| security_sql_injection | PASS | No SQL queries found in changed files (MongoDB used) |
| memory_safety | PASS | strncpy used correctly with null termination |

### Positive Security Changes in This Diff

1. **Removed hardcoded credentials** : Previous code had `sendLoginData("Killian2", "1234")` - now uses proper credential handling
2. **Removed debug password logging** : `std::cout << "password: " << password` removed
3. **Fixed typo in enum** : `Snapshop` -> `Snapshot` (correctness)
4. **Proper error handling** : Added `_onError` callback instead of cout
5. **Thread-safe credential handling** : Added mutex locks around credential access

### Vulnerability Propagation

```
hashPassword (src/server/domain/value_objects/user/utils/PasswordUtils.cpp:11) [WEAK HASH: CWE-916]
+-- Password::verify (src/server/include/domain/value_objects/user/Password.hpp)
    +-- User::verifyPassword (src/server/domain/entities/User.cpp)
        +-- Login::execute (src/server/application/use_cases/auth/Login.cpp)
            +-- ExecuteAuth::execute (src/server/infrastructure/.../ExecuteAuth.cpp)
```

```
sendLoginData (src/client/src/network/TCPClient.cpp:236) [CLEAR TEXT: CWE-319]
+-- handleRead (src/client/src/network/TCPClient.cpp:194)
    +-- asyncRead (src/client/src/network/TCPClient.cpp)
```

### Recommendations

1. **[HAUTE]** Implement proper password hashing with salt (bcrypt/scrypt/Argon2) - SEC-001
2. **[HAUTE]** Add TLS encryption for TCP connections - SEC-002
3. **[MOYENNE]** Clear passwords from memory after use - SEC-003
4. **[BASSE]** Add rate limiting for authentication attempts
5. **[BASSE]** Implement account lockout after failed attempts

### Files Analyzed

| File | Security Sensitive | Critical | Issues Found |
|------|-------------------|----------|--------------|
| src/client/src/network/TCPClient.cpp | Yes | Yes | 2 (SEC-002, SEC-003) |
| src/common/protocol/Protocol.hpp | Yes | Yes | 0 |
| src/server/application/use_cases/auth/Login.cpp | Yes | Yes | 0 (improved) |
| src/server/application/use_cases/auth/Register.cpp | Yes | Yes | 0 (improved) |
| src/server/infrastructure/adapters/in/network/UDPServer.cpp | No | No | 0 |
| src/server/domain/value_objects/user/utils/PasswordUtils.cpp | Yes | Yes | 1 (SEC-001) |

### JSON Output (pour synthesis)

```json
{
  "agent": "security",
  "score": 70,
  "vulnerabilities": 4,
  "regressions": 0,
  "max_severity": "Major",
  "cwes": ["CWE-916", "CWE-319", "CWE-316"],
  "findings": [
    {
      "id": "SEC-001",
      "source": ["security"],
      "severity": "Major",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-916",
      "file": "src/server/domain/value_objects/user/utils/PasswordUtils.cpp",
      "line": 11,
      "function": "hashPassword",
      "message": "Password hashing uses SHA256 without salt - vulnerable to rainbow tables",
      "blocking": false,
      "time_estimate_min": 45
    },
    {
      "id": "SEC-002",
      "source": ["security"],
      "severity": "Major",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-319",
      "file": "src/client/src/network/TCPClient.cpp",
      "line": 236,
      "function": "sendLoginData",
      "message": "Passwords transmitted in clear text over TCP without TLS",
      "blocking": false,
      "time_estimate_min": 180,
      "propagation": 2
    },
    {
      "id": "SEC-003",
      "source": ["security"],
      "severity": "Medium",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-316",
      "file": "src/client/src/network/TCPClient.cpp",
      "line": 223,
      "function": "setLoginCredentials",
      "message": "Password stored in memory not cleared after use",
      "blocking": false,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-004",
      "source": ["security"],
      "severity": "Info",
      "category": "Security",
      "isBug": false,
      "type": "improvement",
      "cwe": null,
      "file": "multiple",
      "line": null,
      "function": null,
      "message": "Debug statements logging passwords removed - good security practice",
      "blocking": false,
      "time_estimate_min": 0
    }
  ],
  "positive_changes": [
    "Removed hardcoded credentials from code",
    "Removed password logging statements",
    "Added mutex locks for thread-safe credential handling",
    "Improved error handling with callbacks"
  ],
  "bug_history_analyzed": 0,
  "patterns_checked": 4,
  "agentdb_queries": {
    "error_history": {"status": "ok", "count": 0, "note": "No history - project appears new"},
    "file_context": {"status": "ok", "security_sensitive_count": 3},
    "patterns": {"status": "ok", "count": 3},
    "symbol_callers": {"status": "ok", "queries": 2},
    "list_critical_files": {"status": "ok", "count": 50}
  }
}
```
