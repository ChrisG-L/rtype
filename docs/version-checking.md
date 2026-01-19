# Version Checking System

Client/server version comparison using git hashes at connection time.

## Architecture

```
[Client: LoginScene] ←TCP→ [TCPAuthServer]
        ↓                        ↓
  Version.hpp            VersionHistoryManager
  (getClientVersion)      (loadFromFile)
        ↓                        ↓
   VersionInfo    ←────→   VersionHistory
   (13 bytes)              (451 bytes)
```

## Wire Structures

```cpp
// VersionInfo (13 bytes) - Sent in AuthResponseWithToken
struct VersionInfo {
    uint8_t major, minor, patch;
    uint8_t flags;              // Bit 0: isDev
    char gitHash[9];            // 8 chars + null
};

// VersionHistory (451 bytes) - Last 50 commits
struct VersionHistory {
    uint8_t count;
    char hashes[50][9];         // Newest first
};
```

## Verification Flow

1. **Login** - Client sends credentials
2. **Auth Success** - Server responds with `serverVersion` + `versionHistory`
3. **Client compares** - `isVersionCompatible(client, server)`
4. **If mismatch** - Popup shows commits behind + JENKINS button

## Version Compatibility

The client performs an exact hash match with the server version:

```cpp
// Version.hpp - Actual implementation
inline bool isVersionCompatible(const VersionInfo& client, const VersionInfo& server) {
    if (client.isDev()) {
        return true;  // Dev mode bypasses check
    }
    return client.isExactMatch(server);  // Exact hash comparison
}
```

The history is used separately to calculate how many commits behind:

```cpp
// TCPClient.cpp - After receiving auth response
int commitsBehind = response.versionHistory.findPosition(clientVersion.gitHash);
if (!isVersionCompatible(clientVersion, response.serverVersion)) {
    emit(TCPVersionMismatchEvent{clientVersion, response.serverVersion, commitsBehind});
}
```

## Development Mode

Create a `version.dev` file at project root to bypass version checking:

```bash
touch version.dev    # Activates dev mode (gitignored)
```

In dev mode:
- Version mismatch warnings are suppressed
- Useful for local development against production server

## Test Environment Variable

Simulate a different git hash for testing:

```bash
RTYPE_TEST_HASH=abc1234 ./rtype_client
```

## Key Files

| File | Description |
|------|-------------|
| `src/client/include/core/Version.hpp` | `getClientVersion()`, `isDevMode()`, `formatVersion()` |
| `src/server/include/infrastructure/version/VersionHistoryManager.hpp` | Singleton, `loadFromFile()`, `getCommitsBehind()` |
| `src/common/protocol/Protocol.hpp` | VersionInfo, VersionHistory structs |
| `src/client/src/scenes/LoginScene.cpp` | Mismatch popup, JENKINS button |
| `scripts/generate_version_history.sh` | Helper script to generate history |

## CI/CD Integration

### Jenkinsfile

Generates `version_history.txt` at checkout:

```groovy
stage('Checkout') {
    steps {
        checkout scm
        sh 'git log --format="%h" -n 50 > version_history.txt'
    }
}
```

### deploy-service.py

Regenerates history at each VPS deployment:

```python
def generate_version_history():
    result = subprocess.run(
        ['git', 'log', '--format=%h', '-n', '50'],
        capture_output=True, text=True
    )
    with open('version_history.txt', 'w') as f:
        f.write(result.stdout)
```

## Version History File Format

Simple text file with one short hash per line (newest first):

```
f9dbaad
11d8c82
3a4d4c6
674b1c7
32120ac
...
```

The server loads this file at startup via `VersionHistoryManager::loadFromFile()`.

## Client Mismatch UI

When version mismatch is detected:

1. **Warning popup** displays:
   - "Version mismatch detected"
   - Number of commits behind (if calculable)
   - Current client hash vs server hash

2. **Actions available:**
   - **JENKINS** button - Opens CI/CD to download latest build
   - **CONTINUE** button - Proceed anyway (may cause issues)
   - **QUIT** button - Exit client

## Troubleshooting

### "Version mismatch" on every connection

1. Rebuild client from latest main branch
2. Check if `version.dev` exists (should bypass)
3. Verify server's `version_history.txt` is up to date

### Client shows wrong hash

1. Ensure clean git state (`git status`)
2. Check `RTYPE_TEST_HASH` env var isn't set
3. Rebuild client after `git pull`

### Server history not loading

1. Check `version_history.txt` exists in server directory
2. Verify file permissions
3. Check server logs for load errors
