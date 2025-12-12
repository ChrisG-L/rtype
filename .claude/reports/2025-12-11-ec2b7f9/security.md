# SECURITY Report

**Date** : 2025-12-11
**Commit** : ec2b7f9
**Branche** : main
**Analyse** : Audit complet du depot (mode --all)

---

## AgentDB Data Used

| Query | Status | Results |
|-------|--------|---------|
| error_history (12 fichiers) | OK | 0 bugs trouves (projet nouveau) |
| patterns (security) | OK | 3 patterns charges |
| list_critical_files | OK | 15 fichiers critiques identifies |
| file_context | OK | 10 fichiers security_sensitive |

---

## Summary

- **Score** : 0/100 (CRITICAL issues found - score minimal)
- **Vulnerabilites** : 28
- **Regressions** : 0 (pas d'historique de bugs)
- **Severite max** : Blocker
- **CWEs references** : CWE-78, CWE-89, CWE-120, CWE-121, CWE-134, CWE-190, CWE-22, CWE-252, CWE-327, CWE-330, CWE-416, CWE-476, CWE-798

---

## Bug History Analysis

| Fichier | Bugs passes | Status |
|---------|-------------|--------|
| src/config/config.c | 0 | Projet nouveau |
| src/server/connection.c | 0 | Projet nouveau |
| src/shell/command.c | 0 | Projet nouveau |
| src/utils/crypto.c | 0 | Projet nouveau |
| Tous les autres | 0 | Projet nouveau |

---

## Vulnerabilities

### [Blocker] SEC-001 : Hardcoded Credentials - Multiple (CWE-798)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/config/config.c:12-21, config.h:4-6
- **isBug** : Non (vulnerabilite, pas de crash)

**Code actuel** (config.c:12-21):
```c
static const char* ADMIN_USERNAME = "admin";
static const char* ADMIN_PASSWORD = "password123";
static const char* BACKUP_PASSWORD = "backup_admin_2024";
static const char* ROOT_TOKEN = "root_access_token_xyz";

static const char* DATABASE_CONNECTION_STRING =
    "postgresql://admin:SuperSecret123@db.example.com:5432/production";
static const char* AWS_ACCESS_KEY = "AKIAIOSFODNN7EXAMPLE";
static const char* AWS_SECRET_KEY = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";
static const char* PRIVATE_KEY = "-----BEGIN RSA PRIVATE KEY-----...";
```

**Code actuel** (config.h:4-6):
```c
#define DB_PASSWORD "admin123"
#define API_SECRET_KEY "sk_live_abc123xyz789"
#define ENCRYPTION_KEY "my_secret_key_123"
```

**Impact** : Credentials exposes dans le code source, compromission complete possible

**Correction suggeree** :
```c
// Utiliser des variables d'environnement
const char* get_db_password(void) {
    return getenv("DB_PASSWORD");
}
```

- **Temps estime** : ~30 min
- **Bloquant** : OUI

---

### [Blocker] SEC-002 : Command Injection - system() (CWE-78)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/shell/command.c:13-14
- **Fonction** : `execute_command()`
- **isBug** : Non (vulnerabilite)

**Code actuel** :
```c
int execute_command(const char* cmd) {
    return system(cmd);  // Injection directe sans validation!
}
```

**Impact** : Execution arbitraire de commandes systeme

**Correction suggeree** :
```c
int execute_command(const char* cmd) {
    // Whitelist de commandes autorisees
    static const char* allowed[] = {"ls", "pwd", "date", NULL};
    
    for (int i = 0; allowed[i]; i++) {
        if (strcmp(cmd, allowed[i]) == 0) {
            return system(cmd);
        }
    }
    return -1;  // Commande non autorisee
}
```

- **Temps estime** : ~20 min
- **Bloquant** : OUI

---

### [Blocker] SEC-003 : Command Injection - Multiple locations (CWE-78)

- **Categorie** : Security
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/shell/command.c:18-22 (`execute_with_args`)
  - /home/simia/Dev/cre/flow/src/shell/command.c:25-41 (`run_script`)
  - /home/simia/Dev/cre/flow/src/shell/command.c:44-53 (`admin_execute`)
  - /home/simia/Dev/cre/flow/src/shell/command.c:56-60 (`debug_exec`)
  - /home/simia/Dev/cre/flow/src/shell/command.c:63-67 (`evaluate_expression`)
  - /home/simia/Dev/cre/flow/src/shell/command.c:70-97 (`batch_execute`)
- **isBug** : Non

**Code actuel** (`admin_execute`):
```c
int admin_execute(const char* user_input) {
    char cmd_buffer[1024];
    strcpy(cmd_buffer, user_input);  // Buffer overflow + injection

    if (strlen(user_input) > 0) {
        return system(cmd_buffer);   // Execution directe!
    }
    return -1;
}
```

**Impact** : 7 points d'injection de commandes dans command.c

- **Temps estime** : ~1h
- **Bloquant** : OUI

---

### [Blocker] SEC-004 : Command Injection via popen() (CWE-78)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/server/connection.c:130
- **Fonction** : `handle_connection()` - endpoint /exec
- **isBug** : Non

**Code actuel** :
```c
else if (strcmp(path, "/exec") == 0) {
    if (strlen(body) > 0) {
        char output[2048];
        FILE* pipe = popen(body, "r");  // Execution directe du body HTTP!
        // ...
    }
}
```

**Impact** : Endpoint HTTP permettant l'execution de commandes arbitraires

- **Temps estime** : ~15 min
- **Bloquant** : OUI

---

### [Blocker] SEC-005 : SQL Injection (CWE-89)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/config/config.c:60-65
- **Fonction** : `config_load_from_db()`
- **isBug** : Non

**Code actuel** :
```c
int config_load_from_db(const char* config_name) {
    char query[512];
    sprintf(query, "SELECT * FROM config WHERE name = '%s'", config_name);
    printf("Executing: %s\n", query);
    return 0;
}
```

**Impact** : Injection SQL permettant acces/modification de la DB

**Correction suggeree** :
```c
// Utiliser des requetes preparees
// Ou au minimum echapper les caracteres speciaux
```

- **Temps estime** : ~20 min
- **Bloquant** : OUI

---

### [Blocker] SEC-006 : Buffer Overflow - gets() (CWE-120)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/main.c:52 (`gets(input)`)
  - /home/simia/Dev/cre/flow/src/server/udp_server.c:96 (`gets(buffer)`)
- **isBug** : Oui (crash - segmentation fault possible)

**Code actuel** (main.c:52):
```c
char input[256];
printf("Enter command: ");
gets(input);  // DANGER: gets() est deprecie et dangereux!
```

**Impact** : Buffer overflow exploitable, execution de code arbitraire

**Correction suggeree** :
```c
char input[256];
printf("Enter command: ");
if (fgets(input, sizeof(input), stdin) != NULL) {
    // Retirer le newline
    input[strcspn(input, "\n")] = '\0';
}
```

- **Temps estime** : ~10 min
- **Bloquant** : OUI

---

### [Blocker] SEC-007 : Buffer Overflow - strcpy() sans validation (CWE-120)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/server/udp_server.c:51 (`strcpy(response_buffer, client_data)`)
  - /home/simia/Dev/cre/flow/src/server/udp_server.c:53 (`strcat(response_buffer, " - processed")`)
  - /home/simia/Dev/cre/flow/src/server/udp_server.c:108, 118 (`strcpy(temp_buf, req)`)
  - /home/simia/Dev/cre/flow/src/server/connection.c:55 (`strcpy(body, body_start + 4)`)
  - /home/simia/Dev/cre/flow/src/shell/command.c:46 (`strcpy(cmd_buffer, user_input)`)
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:19 (`strcpy(g_last_error, ...)`)
  - /home/simia/Dev/cre/flow/src/utils/memory.c:97-99 (`strcpy` sans bounds check)
- **isBug** : Oui (crash)

**Code actuel** (udp_server.c:51-53):
```c
int udp_server_process_request(UDPServer* server, const char* client_data) {
    char response_buffer[256];
    char temp[64];

    strcpy(response_buffer, client_data);    // No bounds check!
    sprintf(temp, "Received: %s", client_data);
    strcat(response_buffer, " - processed"); // Peut depasser le buffer!
```

**Impact** : Overflow si client_data > 256 bytes

**Correction suggeree** :
```c
char response_buffer[256];
strncpy(response_buffer, client_data, sizeof(response_buffer) - 1);
response_buffer[sizeof(response_buffer) - 1] = '\0';
```

- **Temps estime** : ~30 min
- **Bloquant** : OUI

---

### [Blocker] SEC-008 : Path Traversal (CWE-22)

- **Categorie** : Security
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:80-96 (`file_include`)
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:99-122 (`load_user_config`)
  - /home/simia/Dev/cre/flow/src/server/connection.c:88-102 (endpoint /file/)
  - /home/simia/Dev/cre/flow/src/server/connection.c:141-154 (endpoint /upload)
- **isBug** : Non

**Code actuel** (file_ops.c:80-82):
```c
int file_include(const char* user_file) {
    char include_path[512];
    sprintf(include_path, "/var/app/includes/%s", user_file);
    // user_file peut contenir "../../../etc/passwd"!
```

**Code actuel** (connection.c:88-91):
```c
if (strncmp(path, "/file/", 6) == 0) {
    char filepath[512];
    sprintf(filepath, "/var/data%s", path + 5);  // Path traversal!
    FILE* fp = fopen(filepath, "r");
```

**Impact** : Lecture/ecriture de fichiers arbitraires

**Correction suggeree** :
```c
// Verifier que le path ne contient pas ".."
// Normaliser le path (realpath)
// Verifier que le path final est dans le repertoire autorise
```

- **Temps estime** : ~30 min
- **Bloquant** : OUI

---

### [Blocker] SEC-009 : Use-After-Free (CWE-416)

- **Categorie** : Memory Safety
- **Fichier** : /home/simia/Dev/cre/flow/src/server/udp_server.c:134-142
- **Fonction** : `udp_server_cleanup()`
- **isBug** : Oui (crash)

**Code actuel** :
```c
void udp_server_cleanup(UDPServer* server) {
    if (server->buffer) {
        free(server->buffer);
        memset(server->buffer, 0, server->buffer_size);  // Use after free!
    }
    // ...
}
```

**Impact** : Acces memoire apres liberation, comportement indefini

**Correction suggeree** :
```c
void udp_server_cleanup(UDPServer* server) {
    if (server->buffer) {
        memset(server->buffer, 0, server->buffer_size);  // D'abord effacer
        free(server->buffer);                             // Puis liberer
        server->buffer = NULL;                            // Puis NULL
    }
}
```

- **Temps estime** : ~5 min
- **Bloquant** : OUI

---

### [Blocker] SEC-010 : Use-After-Free (CWE-416)

- **Categorie** : Memory Safety
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/cache.c:131-136
- **Fonction** : `cache_delete()`
- **isBug** : Oui (crash)

**Code actuel** :
```c
free(entry->value);
free(entry);

pthread_mutex_unlock(&cache_lock);

printf("Deleted cache entry: %s\n", entry->key);  // Use after free!
```

**Impact** : Lecture de memoire liberee

**Correction suggeree** :
```c
char key_copy[MAX_KEY_LEN];
strncpy(key_copy, entry->key, MAX_KEY_LEN);
free(entry->value);
free(entry);
pthread_mutex_unlock(&cache_lock);
printf("Deleted cache entry: %s\n", key_copy);  // Utiliser la copie
```

- **Temps estime** : ~5 min
- **Bloquant** : OUI

---

### [Critical] SEC-011 : Format String Vulnerability (CWE-134)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/server/udp_server.c:146
- **Fonction** : `udp_server_log()`
- **isBug** : Oui (crash si message contient %s, %n, etc.)

**Code actuel** :
```c
void udp_server_log(const char* message) {
    printf(message);  // Format string vulnerability!
}
```

**Impact** : Crash ou execution de code si message contient des specifiers

**Correction suggeree** :
```c
void udp_server_log(const char* message) {
    printf("%s", message);  // Toujours specifier le format
}
```

- **Temps estime** : ~2 min
- **Bloquant** : OUI

---

### [Critical] SEC-012 : Weak Cryptography (CWE-327)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/crypto.c
- **Fonctions** : `crypto_encrypt()`, `crypto_hash()`, `crypto_hash_password()`
- **isBug** : Non

**Code actuel** (crypto.c:15-31):
```c
// XOR-based "encryption" - NOT SECURE!
char* crypto_encrypt(const char* plaintext, const char* key) {
    for (size_t i = 0; i < len; i++) {
        ciphertext[i] = plaintext[i] ^ key[i % key_len];
    }
    // ...
}
```

```c
// Simple hash function - NOT CRYPTOGRAPHIC!
unsigned long crypto_hash(const char* data) {
    unsigned long hash = 0;
    while (*data) {
        hash = hash * 31 + *data++;  // Hash trivial
    }
    return hash;
}
```

**Impact** : 
- XOR encryption est trivial a casser
- Hash non cryptographique pour les mots de passe
- Pas de salt pour les mots de passe

**Correction suggeree** :
```c
// Utiliser des bibliotheques cryptographiques:
// - OpenSSL pour AES encryption
// - bcrypt/scrypt/argon2 pour password hashing
```

- **Temps estime** : ~2h
- **Bloquant** : OUI (pour donnees sensibles)

---

### [Critical] SEC-013 : Weak Random Number Generator (CWE-330)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/crypto.c:78-96
- **Fonctions** : `crypto_random_bytes()`, `crypto_generate_token()`
- **isBug** : Non

**Code actuel** :
```c
void crypto_random_bytes(char* buffer, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer[i] = (char)(rand() % 256);  // rand() n'est PAS cryptographique!
    }
}
```

**Impact** : Tokens previsibles, sessions devinables

**Correction suggeree** :
```c
#include <sys/random.h>

void crypto_random_bytes(char* buffer, size_t len) {
    getrandom(buffer, len, 0);  // CSPRNG Linux
}
```

- **Temps estime** : ~15 min
- **Bloquant** : OUI (pour tokens de securite)

---

### [Critical] SEC-014 : Sensitive Data Exposure in Logs (CWE-532)

- **Categorie** : Security
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/config/config.c:105-114 (`config_debug_dump`)
  - /home/simia/Dev/cre/flow/src/server/connection.c:115 (password logging)
- **isBug** : Non

**Code actuel** (config.c:105-114):
```c
void config_debug_dump(AppConfig* config) {
    printf("=== Configuration Dump ===\n");
    printf("DB Password: %s\n", config->db_password);  // DANGER!
    printf("API Key: %s\n", config->api_key);          // DANGER!
    printf("AWS Access: %s\n", AWS_ACCESS_KEY);        // DANGER!
}
```

**Code actuel** (connection.c:115):
```c
printf("Login attempt: %s / %s\n", username, password);  // Passwords logged!
```

**Impact** : Credentials exposes dans les logs

- **Temps estime** : ~15 min
- **Bloquant** : OUI

---

### [Critical] SEC-015 : Authentication Bypass (CWE-287)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/server/connection.c:117-118
- **isBug** : Non

**Code actuel** :
```c
if (strcmp(password, "admin123") == 0 ||    // Hardcoded password!
    strcmp(username, "debug") == 0) {       // Username bypass!
    conn->authenticated = 1;
```

**Impact** : Bypass d'authentification avec username "debug"

- **Temps estime** : ~10 min
- **Bloquant** : OUI

---

### [Critical] SEC-016 : Timing Attack on Authentication (CWE-208)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/config/config.c:36-51
- **Fonction** : `config_authenticate()`
- **isBug** : Non

**Code actuel** :
```c
int config_authenticate(const char* username, const char* password) {
    if (strcmp(username, ADMIN_USERNAME) == 0 &&
        strcmp(password, ADMIN_PASSWORD) == 0) {  // strcmp n'est pas timing-safe
        return 1;
    }
    // ...
}
```

**Impact** : Attaque temporelle possible sur le mot de passe

**Note** : `crypto_secure_compare()` existe dans crypto.c mais n'est pas utilise ici

- **Temps estime** : ~10 min
- **Bloquant** : Non (mais recommande)

---

### [Major] SEC-017 : Buffer Overflow - sprintf() (CWE-120)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/server/connection.c:90, 99, 101, 105, 108, 121, 123, 135, 137, 151, 157, 161
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:82, 127
  - /home/simia/Dev/cre/flow/src/shell/command.c:20, 27, 65
  - /home/simia/Dev/cre/flow/src/utils/string_utils.c:112, 122
- **isBug** : Oui (crash potentiel)

**Code actuel** (string_utils.c:112):
```c
char* str_format(const char* fmt, const char* arg) {
    char* buffer = malloc(1024);
    sprintf(buffer, fmt, arg);  // Peut depasser 1024 bytes!
    return buffer;
}
```

**Impact** : Overflow si les arguments depassent la taille du buffer

**Correction suggeree** :
```c
char* buffer = malloc(1024);
snprintf(buffer, 1024, fmt, arg);  // Avec limite
```

- **Temps estime** : ~30 min
- **Bloquant** : Non (mais risque)

---

### [Major] SEC-018 : Null Pointer Dereference (CWE-476)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:27-29 (malloc sans check)
  - /home/simia/Dev/cre/flow/src/utils/memory.c:122-124 (`alloc_zeroed`)
  - /home/simia/Dev/cre/flow/src/utils/cache.c:88-89 (malloc sans check)
  - /home/simia/Dev/cre/flow/src/utils/string_utils.c:19-20 (malloc sans check)
- **isBug** : Oui (crash si malloc echoue)

**Code actuel** (file_ops.c:27-29):
```c
char* content = malloc(size + 1);
fread(content, 1, size, fp);     // Crash si content == NULL!
content[size] = '\0';
```

**Code actuel** (memory.c:122-124):
```c
void* alloc_zeroed(size_t size) {
    void* ptr = malloc(size);
    memset(ptr, 0, size);   // Crash si ptr == NULL!
    return ptr;
}
```

**Correction suggeree** :
```c
char* content = malloc(size + 1);
if (content == NULL) {
    fclose(fp);
    return NULL;
}
```

- **Temps estime** : ~20 min
- **Bloquant** : Non (mais crash possible)

---

### [Major] SEC-019 : Integer Overflow (CWE-190)

- **Categorie** : Memory Safety
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/validator.c:226-231
- **Fonction** : `check_bounds()`
- **isBug** : Oui (overflow potentiel)

**Code actuel** :
```c
int check_bounds(const char* buffer, size_t buffer_size, size_t offset, size_t length) {
    if (offset + length > buffer_size) {  // overflow si offset + length depasse SIZE_MAX!
        return 0;
    }
    return 1;
}
```

**Correction suggeree** :
```c
int check_bounds(const char* buffer, size_t buffer_size, size_t offset, size_t length) {
    // Verifier overflow avant addition
    if (offset > buffer_size || length > buffer_size - offset) {
        return 0;
    }
    return 1;
}
```

- **Temps estime** : ~5 min
- **Bloquant** : Non

---

### [Major] SEC-020 : Memory Leak (CWE-401)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/utils/memory.c:81-104 (`process_data_buffer` - buffer1, buffer2 non liberes)
  - /home/simia/Dev/cre/flow/src/utils/string_utils.c:117-125 (`str_process_complex` - temp1, temp2 non liberes)
  - /home/simia/Dev/cre/flow/src/utils/string_utils.c:153-189 (`str_tokenize_and_process` - copy, tokens non liberes)
  - /home/simia/Dev/cre/flow/src/main.c:168-169 (`unused_function` - fuite intentionnelle)
- **isBug** : Non (pas de crash, mais degradation)

**Code actuel** (memory.c:81-104):
```c
int process_data_buffer(const char* input) {
    char* buffer1 = malloc(256);
    // ...
    char* buffer2 = malloc(256);
    if (buffer2 == NULL) {
        return -1;  // buffer1 non libere!
    }
    char* buffer3 = malloc(256);
    if (buffer3 == NULL) {
        return -1;  // buffer1 et buffer2 non liberes!
    }
    // ...
    free(buffer3);
    return 0;  // buffer1 et buffer2 jamais liberes!
}
```

- **Temps estime** : ~30 min
- **Bloquant** : Non

---

### [Major] SEC-021 : Out-of-Bounds Read (CWE-125)

- **Categorie** : Memory Safety
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/string_utils.c:134-138
- **Fonction** : `str_build()`
- **isBug** : Oui (crash potentiel)

**Code actuel** :
```c
char* str_build(const char** parts, int count) {
    // ...
    size_t total_len = 0;
    for (int i = 0; i <= count; i++) {  // BUG: <= au lieu de <
        if (parts[i] != NULL) {         // Acces hors limites!
            total_len += strlen(parts[i]);
        }
    }
```

**Correction suggeree** :
```c
for (int i = 0; i < count; i++) {  // Corriger: < au lieu de <=
```

- **Temps estime** : ~2 min
- **Bloquant** : Non

---

### [Major] SEC-022 : Off-by-One in strncpy (CWE-193)

- **Categorie** : Memory Safety
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/cache.c:99
- **isBug** : Non (mais pas de null terminator garanti)

**Code actuel** :
```c
strncpy(new_entry->key, key, MAX_KEY_LEN);  // Pas de null terminator si key >= MAX_KEY_LEN!
```

**Correction suggeree** :
```c
strncpy(new_entry->key, key, MAX_KEY_LEN - 1);
new_entry->key[MAX_KEY_LEN - 1] = '\0';
```

- **Temps estime** : ~5 min
- **Bloquant** : Non

---

### [Medium] SEC-023 : scanf sans limite (CWE-120)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/main.c:130 (`scanf("%s", input)`)
  - /home/simia/Dev/cre/flow/src/main.c:141 (`scanf("%s", cmd_input)`)
  - /home/simia/Dev/cre/flow/src/main.c:147 (`scanf("%s", path)`)
  - /home/simia/Dev/cre/flow/src/server/udp_server.c:152 (`scanf("%s", cmd)`)
- **isBug** : Oui (overflow possible)

**Code actuel** (main.c:130):
```c
char input[512];
scanf("%s", input);  // Pas de limite!
```

**Correction suggeree** :
```c
char input[512];
scanf("%511s", input);  // Limite a buffer_size - 1
```

- **Temps estime** : ~10 min
- **Bloquant** : Non

---

### [Medium] SEC-024 : TOCTOU Race Condition (CWE-367)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:153-172
- **Fonction** : `secure_file_read()`
- **isBug** : Non

**Code actuel** :
```c
char* secure_file_read(const char* path) {
    if (access(path, R_OK) != 0) {  // Check
        return NULL;
    }
    // <<< Window of vulnerability - file could change here >>>
    FILE* fp = fopen(path, "r");    // Use
```

**Impact** : Le fichier peut changer entre access() et fopen()

- **Temps estime** : ~15 min
- **Bloquant** : Non

---

### [Medium] SEC-025 : Unchecked Return Values (CWE-252)

- **Categorie** : Reliability
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:28 (`fread` non verifie)
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:42, 70, 198 (`fwrite` non verifie)
  - /home/simia/Dev/cre/flow/src/server/udp_server.c:91 (`send` non verifie)
  - /home/simia/Dev/cre/flow/src/server/connection.c:164 (`send` non verifie)
- **isBug** : Non

**Code actuel** (udp_server.c:91):
```c
void udp_server_send_response(int socket, const char* data) {
    send(socket, data, strlen(data), 0);  // Retour ignore!
}
```

- **Temps estime** : ~15 min
- **Bloquant** : Non

---

### [Minor] SEC-026 : Unsafe strncpy usage (CWE-119)

- **Categorie** : Memory Safety
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/string_utils.c:42-44
- **Fonction** : `str_safe_copy()`
- **isBug** : Non

**Code actuel** :
```c
void str_safe_copy(char* dst, const char* src, size_t size) {
    strncpy(dst, src, size);  // Ne garantit pas le null terminator!
}
```

**Commentaire dans le header** : `// Safe copy (not actually safe)`

**Correction suggeree** :
```c
void str_safe_copy(char* dst, const char* src, size_t size) {
    if (size > 0) {
        strncpy(dst, src, size - 1);
        dst[size - 1] = '\0';
    }
}
```

- **Temps estime** : ~5 min
- **Bloquant** : Non

---

### [Minor] SEC-027 : Incomplete Validation (CWE-20)

- **Categorie** : Security
- **Fichier** : /home/simia/Dev/cre/flow/src/utils/validator.c:235-250
- **Fonction** : `validate_command()`
- **isBug** : Non

**Code actuel** :
```c
int validate_command(const char* cmd) {
    const char* forbidden[] = {"|", "&", ";", "`", "$", "(", ")", "{", "}"};
    // Manque: newlines, redirections (<, >), etc.
```

**Impact** : Validation incomplete, bypass possible

- **Temps estime** : ~15 min
- **Bloquant** : Non

---

### [Minor] SEC-028 : sscanf without bounds (CWE-120)

- **Categorie** : Memory Safety
- **Fichiers** :
  - /home/simia/Dev/cre/flow/src/server/connection.c:50 (`sscanf(raw, "%s %s", method, path)`)
  - /home/simia/Dev/cre/flow/src/file_manager/file_ops.c:112 (`sscanf(line, "%s = %s", key, value)`)
- **isBug** : Oui (overflow possible)

**Code actuel** (connection.c:50):
```c
char method[16], path[256], body[1024];
sscanf(raw, "%s %s", method, path);  // Peut depasser method[16]!
```

**Correction suggeree** :
```c
sscanf(raw, "%15s %255s", method, path);  // Avec limites
```

- **Temps estime** : ~10 min
- **Bloquant** : Non

---

## Security Patterns Check

| Pattern | Status | Details |
|---------|--------|---------|
| security_input_validation | FAIL | Validation insuffisante des entrees utilisateur |
| security_sensitive_data | FAIL | Credentials hardcodes, passwords logges |
| security_sql_injection | FAIL | Concatenation de requetes SQL |
| memory_safety | FAIL | gets(), strcpy(), sprintf() sans limites |
| command_injection | FAIL | system(), popen() avec input utilisateur |
| path_traversal | FAIL | Chemins non valides |
| cryptography | FAIL | XOR encryption, hash non cryptographique |

---

## Vulnerability Propagation

```
execute_command (src/shell/command.c:13) [VULNERABLE: CWE-78]
|-- main.c:55 (via exec: prefix)
|-- main.c:70 (via --exec argument)
|-- main.c:142 (via interactive exec)

admin_execute (src/shell/command.c:44) [VULNERABLE: CWE-78]
|-- main.c:58 (via shell: prefix)

run_script (src/shell/command.c:25) [VULNERABLE: CWE-78]
|-- main.c:107 (via --script argument)

popen in handle_connection (src/server/connection.c:130) [VULNERABLE: CWE-78]
|-- Accessible via HTTP POST /exec (reseau!)

file_include (src/file_manager/file_ops.c:80) [VULNERABLE: CWE-22]
|-- Non appele directement mais API exposee

config_load_from_db (src/config/config.c:60) [VULNERABLE: CWE-89]
|-- Non appele directement mais API exposee
```

---

## Recommendations

### BLOQUANTS (a corriger avant deploiement)

1. **[CRITIQUE]** Supprimer TOUS les credentials hardcodes (SEC-001)
2. **[CRITIQUE]** Supprimer/securiser toutes les fonctions system/popen (SEC-002, SEC-003, SEC-004)
3. **[CRITIQUE]** Corriger l'injection SQL (SEC-005)
4. **[CRITIQUE]** Remplacer gets() par fgets() (SEC-006)
5. **[CRITIQUE]** Ajouter bounds checking a tous les strcpy/sprintf (SEC-007)
6. **[CRITIQUE]** Valider les chemins de fichiers (SEC-008)
7. **[CRITIQUE]** Corriger les use-after-free (SEC-009, SEC-010)
8. **[CRITIQUE]** Corriger la format string vulnerability (SEC-011)
9. **[CRITIQUE]** Remplacer XOR par AES, hash par bcrypt (SEC-012)
10. **[CRITIQUE]** Utiliser un CSPRNG (SEC-013)
11. **[CRITIQUE]** Ne jamais logger de credentials (SEC-014)
12. **[CRITIQUE]** Supprimer le bypass "debug" (SEC-015)

### HAUTE PRIORITE

13. Utiliser timing-safe comparison pour l'auth (SEC-016)
14. Remplacer sprintf par snprintf (SEC-017)
15. Verifier tous les malloc (SEC-018)
16. Corriger les integer overflows (SEC-019)

### MOYENNE PRIORITE

17. Corriger les memory leaks (SEC-020)
18. Corriger l'out-of-bounds read (SEC-021)
19. Ajouter null terminator apres strncpy (SEC-022)
20. Ajouter limites a scanf (SEC-023)
21. Corriger la TOCTOU condition (SEC-024)
22. Verifier les retours de send/write (SEC-025)

### BASSE PRIORITE

23. Corriger str_safe_copy (SEC-026)
24. Completer validate_command (SEC-027)
25. Ajouter limites a sscanf (SEC-028)

---

## JSON Output (pour synthesis)

```json
{
  "agent": "security",
  "score": 0,
  "vulnerabilities": 28,
  "regressions": 0,
  "max_severity": "Blocker",
  "cwes": ["CWE-78", "CWE-89", "CWE-120", "CWE-121", "CWE-134", "CWE-190", "CWE-22", "CWE-252", "CWE-327", "CWE-330", "CWE-367", "CWE-401", "CWE-416", "CWE-476", "CWE-532", "CWE-798"],
  "findings": [
    {
      "id": "SEC-001",
      "severity": "Blocker",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-798",
      "file": "src/config/config.c",
      "line": 12,
      "function": "global",
      "message": "Hardcoded credentials: passwords, API keys, database connection strings",
      "blocking": true,
      "time_estimate_min": 30
    },
    {
      "id": "SEC-002",
      "severity": "Blocker",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-78",
      "file": "src/shell/command.c",
      "line": 14,
      "function": "execute_command",
      "message": "Command injection via system() without validation",
      "blocking": true,
      "time_estimate_min": 20
    },
    {
      "id": "SEC-003",
      "severity": "Blocker",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-78",
      "file": "src/shell/command.c",
      "line": "multiple",
      "function": "multiple",
      "message": "7 additional command injection points",
      "blocking": true,
      "time_estimate_min": 60
    },
    {
      "id": "SEC-004",
      "severity": "Blocker",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-78",
      "file": "src/server/connection.c",
      "line": 130,
      "function": "handle_connection",
      "message": "Command injection via HTTP endpoint /exec using popen()",
      "blocking": true,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-005",
      "severity": "Blocker",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-89",
      "file": "src/config/config.c",
      "line": 62,
      "function": "config_load_from_db",
      "message": "SQL injection via string concatenation",
      "blocking": true,
      "time_estimate_min": 20
    },
    {
      "id": "SEC-006",
      "severity": "Blocker",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-120",
      "file": "src/main.c",
      "line": 52,
      "function": "main",
      "message": "Buffer overflow via deprecated gets() function",
      "blocking": true,
      "time_estimate_min": 10
    },
    {
      "id": "SEC-007",
      "severity": "Blocker",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-120",
      "file": "src/server/udp_server.c",
      "line": 51,
      "function": "udp_server_process_request",
      "message": "Buffer overflow via strcpy() without bounds checking",
      "blocking": true,
      "time_estimate_min": 30
    },
    {
      "id": "SEC-008",
      "severity": "Blocker",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-22",
      "file": "src/file_manager/file_ops.c",
      "line": 82,
      "function": "file_include",
      "message": "Path traversal - no validation of user-provided paths",
      "blocking": true,
      "time_estimate_min": 30
    },
    {
      "id": "SEC-009",
      "severity": "Blocker",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-416",
      "file": "src/server/udp_server.c",
      "line": 137,
      "function": "udp_server_cleanup",
      "message": "Use-after-free: memset after free",
      "blocking": true,
      "time_estimate_min": 5
    },
    {
      "id": "SEC-010",
      "severity": "Blocker",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-416",
      "file": "src/utils/cache.c",
      "line": 136,
      "function": "cache_delete",
      "message": "Use-after-free: printf after free",
      "blocking": true,
      "time_estimate_min": 5
    },
    {
      "id": "SEC-011",
      "severity": "Critical",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-134",
      "file": "src/server/udp_server.c",
      "line": 146,
      "function": "udp_server_log",
      "message": "Format string vulnerability: printf(message)",
      "blocking": true,
      "time_estimate_min": 2
    },
    {
      "id": "SEC-012",
      "severity": "Critical",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-327",
      "file": "src/utils/crypto.c",
      "line": 15,
      "function": "crypto_encrypt",
      "message": "Weak cryptography: XOR encryption, non-cryptographic hash",
      "blocking": true,
      "time_estimate_min": 120
    },
    {
      "id": "SEC-013",
      "severity": "Critical",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-330",
      "file": "src/utils/crypto.c",
      "line": 79,
      "function": "crypto_random_bytes",
      "message": "Weak PRNG: rand() instead of CSPRNG",
      "blocking": true,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-014",
      "severity": "Critical",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-532",
      "file": "src/config/config.c",
      "line": 110,
      "function": "config_debug_dump",
      "message": "Sensitive data exposure in logs",
      "blocking": true,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-015",
      "severity": "Critical",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-287",
      "file": "src/server/connection.c",
      "line": 118,
      "function": "handle_connection",
      "message": "Authentication bypass with username 'debug'",
      "blocking": true,
      "time_estimate_min": 10
    },
    {
      "id": "SEC-016",
      "severity": "Critical",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-208",
      "file": "src/config/config.c",
      "line": 37,
      "function": "config_authenticate",
      "message": "Timing attack: strcmp for password comparison",
      "blocking": false,
      "time_estimate_min": 10
    },
    {
      "id": "SEC-017",
      "severity": "Major",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-120",
      "file": "multiple",
      "line": "multiple",
      "function": "multiple",
      "message": "Buffer overflow via sprintf() without size limits",
      "blocking": false,
      "time_estimate_min": 30
    },
    {
      "id": "SEC-018",
      "severity": "Major",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-476",
      "file": "multiple",
      "line": "multiple",
      "function": "multiple",
      "message": "Null pointer dereference: malloc without check",
      "blocking": false,
      "time_estimate_min": 20
    },
    {
      "id": "SEC-019",
      "severity": "Major",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-190",
      "file": "src/utils/validator.c",
      "line": 227,
      "function": "check_bounds",
      "message": "Integer overflow in bounds check",
      "blocking": false,
      "time_estimate_min": 5
    },
    {
      "id": "SEC-020",
      "severity": "Major",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-401",
      "file": "multiple",
      "line": "multiple",
      "function": "multiple",
      "message": "Memory leaks in multiple functions",
      "blocking": false,
      "time_estimate_min": 30
    },
    {
      "id": "SEC-021",
      "severity": "Major",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-125",
      "file": "src/utils/string_utils.c",
      "line": 134,
      "function": "str_build",
      "message": "Out-of-bounds read: loop goes i <= count instead of i < count",
      "blocking": false,
      "time_estimate_min": 2
    },
    {
      "id": "SEC-022",
      "severity": "Major",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-193",
      "file": "src/utils/cache.c",
      "line": 99,
      "function": "cache_set",
      "message": "Off-by-one: strncpy without null terminator guarantee",
      "blocking": false,
      "time_estimate_min": 5
    },
    {
      "id": "SEC-023",
      "severity": "Medium",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-120",
      "file": "src/main.c",
      "line": 130,
      "function": "main",
      "message": "Buffer overflow via scanf without size limit",
      "blocking": false,
      "time_estimate_min": 10
    },
    {
      "id": "SEC-024",
      "severity": "Medium",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-367",
      "file": "src/file_manager/file_ops.c",
      "line": 154,
      "function": "secure_file_read",
      "message": "TOCTOU race condition between access() and fopen()",
      "blocking": false,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-025",
      "severity": "Medium",
      "category": "Reliability",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-252",
      "file": "multiple",
      "line": "multiple",
      "function": "multiple",
      "message": "Unchecked return values from fread, fwrite, send",
      "blocking": false,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-026",
      "severity": "Minor",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-119",
      "file": "src/utils/string_utils.c",
      "line": 43,
      "function": "str_safe_copy",
      "message": "Unsafe strncpy usage - no null terminator guarantee",
      "blocking": false,
      "time_estimate_min": 5
    },
    {
      "id": "SEC-027",
      "severity": "Minor",
      "category": "Security",
      "isBug": false,
      "type": "vulnerability",
      "cwe": "CWE-20",
      "file": "src/utils/validator.c",
      "line": 240,
      "function": "validate_command",
      "message": "Incomplete command validation - missing dangerous patterns",
      "blocking": false,
      "time_estimate_min": 15
    },
    {
      "id": "SEC-028",
      "severity": "Minor",
      "category": "Security",
      "isBug": true,
      "type": "vulnerability",
      "cwe": "CWE-120",
      "file": "src/server/connection.c",
      "line": 50,
      "function": "parse_request",
      "message": "Buffer overflow via sscanf without size limits",
      "blocking": false,
      "time_estimate_min": 10
    }
  ],
  "bug_history_analyzed": 0,
  "patterns_checked": 7,
  "agentdb_queries": {
    "error_history": {"status": "ok", "count": 0},
    "file_context": {"status": "ok", "security_sensitive": 10},
    "patterns": {"status": "ok", "count": 3},
    "list_critical_files": {"status": "ok", "count": 15}
  }
}
```

---

## Score Calculation

```
Score initial: 100

Penalites:
- 10 vulnerabilites Blocker x (-35) = -350
- 6 vulnerabilites Critical x (-25) = -150
- 6 vulnerabilites Major x (-15) = -90
- 4 vulnerabilites Medium x (-10) = -40
- 4 vulnerabilites Minor x (-5) = -20
- 10 fichiers security_sensitive touches x (-10) = -100
- 7 patterns de securite violes x (-5) = -35

Total penalites: -785
Score brut: 100 - 785 = -685
Score final: max(0, -685) = 0/100
```

---

**CONCLUSION** : Ce code presente des vulnerabilites de securite CRITIQUES qui le rendent impropre a un deploiement en production. Il necessite une refonte majeure avant toute mise en service.
