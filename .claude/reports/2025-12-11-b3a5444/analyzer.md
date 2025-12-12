# Rapport Analyzer

## Résumé

- **Score** : 25/100
- **Impact global** : GLOBAL
- **Fichiers analysés** : 13
- **Fonctions impactées** : 48
- **Fichiers critiques modifiés** : 3 (src/config/config.c, src/config/config.h, src/utils/string_utils.c)
- **Total appelants identifiés** : 28

## AgentDB Data Used

| Query | Fichier/Symbole | Résultat |
|-------|-----------------|----------|
| file_context | src/config/config.c | ✅ OK - 16 symbols |
| file_context | src/config/config.h | ✅ OK - 15 symbols |
| file_context | src/file_manager/file_ops.c | ✅ OK - 9 symbols |
| file_context | src/file_manager/file_ops.h | ✅ OK - 1 symbol |
| file_context | src/server/udp_server.c | ✅ OK - 10 symbols |
| file_context | src/server/udp_server.h | ✅ OK - 9 symbols |
| file_context | src/shell/command.c | ✅ OK - 7 symbols |
| file_context | src/shell/command.h | ✅ OK - 1 symbol |
| file_context | src/utils/memory.c | ✅ OK - 14 symbols |
| file_context | src/utils/memory.h | ✅ OK - 6 symbols |
| file_context | src/utils/string_utils.c | ✅ OK - 10 symbols |
| file_context | src/utils/string_utils.h | ✅ OK - 1 symbol |
| file_context | src/Makefile | ❌ NOT FOUND |
| file_impact | src/config/config.c | ✅ OK - 1 file |
| file_impact | src/config/config.h | ✅ OK - 2 files (2 critical) |
| file_impact | src/utils/string_utils.c | ✅ OK - 1 file (1 critical) |
| symbol_callers | config_init | ✅ OK - 2 callers |
| symbol_callers | config_load | ✅ OK - 2 callers |
| symbol_callers | execute_command | ✅ OK - 3 callers |
| symbol_callers | str_dup | ✅ OK - 11 callers (11 critical) |

## Analyse par fichier

### src/config/config.c
- **Impact** : GLOBAL
- **Lignes** : +144
- **Criticité** : CRITICAL, security_sensitive
- **Complexité max** : 20
- **Risque** : HIGH

**Variables sensibles exposées** :
- `ADMIN_USERNAME`, `ADMIN_PASSWORD` (lignes 12-13)
- `BACKUP_PASSWORD`, `ROOT_TOKEN` (lignes 14-15)
- `DATABASE_CONNECTION_STRING` (ligne 17)
- `AWS_ACCESS_KEY`, `AWS_SECRET_KEY` (lignes 19-20)
- `PRIVATE_KEY` (ligne 21)

### src/config/config.h
- **Impact** : GLOBAL
- **Lignes** : +27
- **Criticité** : CRITICAL, security_sensitive
- **Macros sensibles** : `DB_PASSWORD`, `API_SECRET_KEY`, `ENCRYPTION_KEY`
- **Risque** : HIGH

### src/server/udp_server.c
- **Impact** : MODULE
- **Lignes** : +153
- **Complexité max** : 35 (très élevé!)
- **Risque** : HIGH
- **Problèmes** : gets() DANGEREUX, format string vulnerability, buffer overflow avec scanf

### src/shell/command.c
- **Impact** : GLOBAL
- **Lignes** : +97
- **Note** : TRÈS DANGEREUX - 8 fonctions avec COMMAND INJECTION
- **Risque** : CRITICAL

### src/utils/string_utils.c
- **Impact** : GLOBAL
- **Lignes** : +147
- **Criticité** : CRITICAL, security_sensitive
- **Risque** : HIGH
- **Note** : str_dup a 11 appelants critiques

## Issues

| Sévérité | ID | Fichier | Ligne | Description |
|----------|------|---------|-------|-------------|
| CRITICAL | ANA-001 | src/config/config.h | 4-6 | Credentials en dur : DB_PASSWORD, API_SECRET_KEY, ENCRYPTION_KEY |
| CRITICAL | ANA-002 | src/config/config.c | 12-21 | Secrets hardcodés : mots de passe admin, tokens, clés AWS, clé privée RSA |
| CRITICAL | ANA-003 | src/shell/command.c | 13-67 | Injection de commandes via system() sans validation |
| CRITICAL | ANA-004 | src/server/udp_server.c | 96 | Utilisation de gets() - buffer overflow garanti |
| HIGH | ANA-005 | src/config/config.c | 62 | SQL Injection dans config_load_from_db() |
| HIGH | ANA-006 | src/server/udp_server.c | 146 | Format string vulnerability dans udp_server_log() |
| HIGH | ANA-007 | src/server/udp_server.c | 152 | Buffer overflow potentiel avec scanf("%s") |
| HIGH | ANA-008 | src/server/udp_server.c | 51-53 | Buffer overflows avec strcpy/strcat non bornés |
| HIGH | ANA-009 | src/server/udp_server.c | 137 | Use-after-free : memset après free() |
| MEDIUM | ANA-010 | src/config/config.c | 110-112 | Exposition de données sensibles dans config_debug_dump() |
| MEDIUM | ANA-011 | src/utils/string_utils.c | 100-108 | Memory leaks dans str_process_complex() |
| MEDIUM | ANA-012 | src/utils/string_utils.c | 111-147 | Memory leaks dans str_tokenize_and_process() |
| MEDIUM | ANA-013 | src/utils/string_utils.c | 43-44 | str_safe_copy ne null-termine pas toujours |
| LOW | ANA-014 | src/server/udp_server.c | 47 | Complexité cyclomatique excessive (35) |
| LOW | ANA-015 | src/utils/memory.c | - | Fonctions pool_* non utilisées |

## Recommandations

### [BLOQUANT] Sécurité critique

1. **Supprimer TOUS les secrets hardcodés** (ANA-001, ANA-002)
2. **Corriger les injections de commandes** (ANA-003)
3. **Remplacer gets()** (ANA-004)

### [HAUTE] Vulnérabilités à corriger

4. Corriger la SQL injection (ANA-005)
5. Corriger les format string vulnerabilities (ANA-006)
6. Sécuriser les buffer operations (ANA-007, ANA-008)
7. Corriger use-after-free (ANA-009)

### [MOYENNE] Améliorations

8. Corriger les memory leaks (ANA-011, ANA-012)
9. Améliorer str_safe_copy (ANA-013)
10. Refactorer le code complexe (ANA-014)
