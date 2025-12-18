#!/usr/bin/env bash
# =============================================================================
# AgentDB Query Script
# =============================================================================
#
# Script helper pour interroger la base AgentDB depuis les sous-agents.
# Retourne du JSON parsable.
#
# Compatibilité: bash 4+, zsh 5+ (via emulation bash)
#
# Usage:
#   bash .claude/agentdb/query.sh <command> [arguments...]
#
# Commands:
#   file_context <path>              - Contexte complet d'un fichier
#   file_metrics <path>              - Métriques détaillées d'un fichier
#   file_impact <path>               - Impact d'une modification
#   symbol_callers <name> [file]     - Qui appelle ce symbole
#   symbol_callees <name> [file]     - Ce que ce symbole appelle
#   error_history <path> [days]      - Historique des bugs (défaut: 180j)
#   patterns [file] [category]       - Patterns applicables
#   architecture_decisions [module]  - ADRs du projet
#   module_summary <module>          - Résumé d'un module
#   search_symbols <query> [kind]    - Recherche de symboles
#   list_modules                     - Liste tous les modules
#   list_critical_files              - Liste les fichiers critiques
#
# Incremental Analysis Commands:
#   get_checkpoint <branch>                          - Récupère le checkpoint d'analyse
#   set_checkpoint <branch> <commit> [files] [verdict] [score] - Met à jour le checkpoint
#   reset_checkpoint <branch>                        - Supprime le checkpoint
#   list_checkpoints                                 - Liste tous les checkpoints
#
# Pipeline Recording Commands:
#   record_pipeline_run <branch> <commit> <score> <verdict> <files_count> - Enregistre une analyse
#
# Examples:
#   bash .claude/agentdb/query.sh file_context "src/server/UDPServer.cpp"
#   bash .claude/agentdb/query.sh symbol_callers "sendPacket"
#   bash .claude/agentdb/query.sh error_history "src/server/UDPServer.cpp" 90
#   bash .claude/agentdb/query.sh search_symbols "UDP*" function
#
# Environment Variables:
#   AGENTDB_CALLER     - Identifier for the calling agent (default: "unknown")
#   AGENTDB_LOG_LEVEL  - 0=off, 1=basic (default), 2=verbose (includes results)
#
# Logging:
#   Logs are written to .claude/logs/agentdb_queries.log
#   Format: [timestamp] [caller] START/END command (duration, size, status)
#
# Example with caller identification:
#   AGENTDB_CALLER="analyzer-agent" bash .claude/agentdb/query.sh file_context "file.cpp"
#
# =============================================================================

# Force bash emulation if running under zsh
if [ -n "$ZSH_VERSION" ]; then
    emulate -L bash
    setopt KSH_ARRAYS BASH_REMATCH
fi

set -e

# Chemin vers la base de données (relatif au repo root)
# Compatible bash et zsh
if [ -n "$BASH_SOURCE" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
elif [ -n "$ZSH_VERSION" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
else
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
fi
DB_PATH="$SCRIPT_DIR/db.sqlite"

# =============================================================================
# LOGGING
# =============================================================================

# Répertoire des logs (créé si nécessaire)
LOG_DIR="${SCRIPT_DIR}/../logs"
LOG_FILE="${LOG_DIR}/agentdb_queries.log"

# Taille max du fichier de log en octets (50KB)
LOG_MAX_SIZE="${AGENTDB_LOG_MAX_SIZE:-51200}"

# Créer le répertoire de logs s'il n'existe pas
mkdir -p "$LOG_DIR" 2>/dev/null || true

# Niveau de log: 0=off, 1=basic, 2=verbose (inclut les résultats)
LOG_LEVEL="${AGENTDB_LOG_LEVEL:-1}"

# Rotation des logs si le fichier dépasse la taille max
rotate_log_if_needed() {
    if [[ -f "$LOG_FILE" ]]; then
        local file_size
        file_size=$(stat -f%z "$LOG_FILE" 2>/dev/null || stat -c%s "$LOG_FILE" 2>/dev/null || echo 0)
        if [[ "$file_size" -gt "$LOG_MAX_SIZE" ]]; then
            # Rotation: garder les 100 dernières lignes
            tail -n 100 "$LOG_FILE" > "${LOG_FILE}.tmp" 2>/dev/null && \
            mv "${LOG_FILE}.tmp" "$LOG_FILE" 2>/dev/null
            echo "[$(date '+%Y-%m-%d %H:%M:%S')] [system] LOG ROTATED (was ${file_size} bytes)" >> "$LOG_FILE" 2>/dev/null || true
        fi
    fi
}

# Rotation au démarrage du script (vérifie une seule fois)
rotate_log_if_needed

# Fonction de logging
log_entry() {
    local level="$1"
    local message="$2"

    [[ "$LOG_LEVEL" -lt "$level" ]] && return 0

    local timestamp=$(date '+%Y-%m-%d %H:%M:%S.%3N')
    local caller="${AGENTDB_CALLER:-unknown}"

    echo "[$timestamp] [$caller] $message" >> "$LOG_FILE" 2>/dev/null || true
}

# Fonction pour obtenir le timestamp en millisecondes (compatible bash/zsh/macOS/Linux)
get_timestamp_ms() {
    if date +%s%3N 2>/dev/null | grep -qE '^[0-9]+$'; then
        date +%s%3N
    else
        # Fallback pour macOS ou systèmes sans %N
        echo "$(($(date +%s) * 1000))"
    fi
}

# Log le début d'une commande
log_start() {
    local cmd="$1"
    shift
    local args="$*"

    START_TIME=$(get_timestamp_ms)
    log_entry 1 "START $cmd ${args:+(args: $args)}"
}

# Log la fin d'une commande
log_end() {
    local cmd="$1"
    local exit_code="${2:-0}"
    local result_size="${3:-0}"

    local end_time=$(get_timestamp_ms)
    local duration=$((end_time - START_TIME))

    if [[ "$exit_code" -eq 0 ]]; then
        log_entry 1 "END   $cmd (${duration}ms, ${result_size} bytes, OK)"
    else
        log_entry 1 "END   $cmd (${duration}ms, FAILED: exit $exit_code)"
    fi
}

# Log un résultat (mode verbose)
log_result() {
    local result="$1"
    log_entry 2 "RESULT: $result"
}

# =============================================================================
# VALIDATION
# =============================================================================

# Vérifier que la base existe
if [[ ! -f "$DB_PATH" ]]; then
    log_entry 1 "ERROR: Database not found at $DB_PATH"
    echo '{"error": "Database not found at '"$DB_PATH"'"}'
    exit 1
fi

# Fonction helper pour exécuter une requête SQLite et retourner du JSON
sqlite_json() {
    local result
    result=$(sqlite3 -json "$DB_PATH" "$1" 2>/dev/null)
    if [[ -z "$result" ]]; then
        echo "[]"
    else
        echo "$result"
    fi
}

# Fonction helper pour une seule ligne
sqlite_json_one() {
    local result
    result=$(sqlite3 -json "$DB_PATH" "$1" 2>/dev/null)
    if [[ -z "$result" ]]; then
        echo "{}"
    else
        echo "$result" | jq '.[0] // {}' 2>/dev/null || echo "{}"
    fi
}

# =============================================================================
# SÉCURITÉ : Échappement SQL
# =============================================================================

# Échappe les guillemets simples pour prévenir les injections SQL
# Usage: safe_string=$(sql_escape "$user_input")
sql_escape() {
    local input="$1"
    # Double les guillemets simples (standard SQL)
    echo "${input//\'/\'\'}"
}

# Valide qu'une chaîne est un hash git valide (hexadécimal)
validate_git_hash() {
    local hash="$1"
    if [[ "$hash" =~ ^[a-fA-F0-9]+$ ]]; then
        return 0
    else
        return 1
    fi
}

# Valide qu'une chaîne est un nom de branche valide
validate_branch_name() {
    local branch="$1"
    # Autorise: lettres, chiffres, /, -, _, .
    if [[ "$branch" =~ ^[a-zA-Z0-9/_.-]+$ ]]; then
        return 0
    else
        return 1
    fi
}

# Valide qu'une chaîne est un chemin de fichier valide
validate_file_path() {
    local path="$1"
    # Interdit: ;, ', ", $, `, \, et les séquences ..
    if [[ "$path" =~ [\;\'\"\$\`\\] ]] || [[ "$path" =~ \.\. ]]; then
        return 1
    else
        return 0
    fi
}

# =============================================================================
# COMMANDES
# =============================================================================

cmd_file_context() {
    local path="$1"

    if [[ -z "$path" ]]; then
        echo '{"error": "Usage: file_context <path>"}'
        return 1
    fi

    # Info fichier
    local file_info=$(sqlite_json_one "
        SELECT
            id, path, module, language,
            is_critical, security_sensitive,
            lines_total, lines_code, lines_comment,
            complexity_avg, complexity_max,
            commits_30d, commits_90d, last_modified
        FROM files
        WHERE path = '$path'
    ")

    if [[ "$file_info" == "{}" ]]; then
        echo '{"error": "File not found: '"$path"'"}'
        return 1
    fi

    local file_id=$(echo "$file_info" | jq -r '.id')

    # Symboles du fichier
    local symbols=$(sqlite_json "
        SELECT
            name, kind, signature,
            complexity, line_start, line_end,
            CASE WHEN doc_comment IS NOT NULL THEN 1 ELSE 0 END as has_doc
        FROM symbols
        WHERE file_id = $file_id
        ORDER BY line_start
    ")

    # Fichiers qui incluent ce fichier
    local included_by=$(sqlite_json "
        SELECT f.path
        FROM file_relations fr
        JOIN files f ON fr.source_file_id = f.id
        WHERE fr.target_file_id = $file_id
        AND fr.relation_type = 'includes'
    " | jq '[.[].path]')

    # Fichiers inclus par ce fichier
    local includes=$(sqlite_json "
        SELECT f.path
        FROM file_relations fr
        JOIN files f ON fr.target_file_id = f.id
        WHERE fr.source_file_id = $file_id
        AND fr.relation_type = 'includes'
    " | jq '[.[].path]')

    # Erreurs récentes
    local errors=$(sqlite_json "
        SELECT error_type, severity, title, resolved_at, resolution
        FROM error_history
        WHERE file_path = '$path' OR file_id = $file_id
        ORDER BY discovered_at DESC
        LIMIT 5
    ")

    # Patterns applicables
    local patterns=$(sqlite_json "
        SELECT name, title, description
        FROM patterns
        WHERE is_active = 1
        AND (file_pattern IS NULL OR '$path' GLOB file_pattern)
    ")

    # Construire le JSON final
    jq -n \
        --argjson file "$file_info" \
        --argjson symbols "$symbols" \
        --argjson included_by "$included_by" \
        --argjson includes "$includes" \
        --argjson errors "$errors" \
        --argjson patterns "$patterns" \
        '{
            file: {
                path: $file.path,
                module: $file.module,
                language: $file.language,
                is_critical: ($file.is_critical == 1),
                security_sensitive: ($file.security_sensitive == 1),
                metrics: {
                    lines_total: $file.lines_total,
                    lines_code: $file.lines_code,
                    lines_comment: $file.lines_comment,
                    complexity_avg: $file.complexity_avg,
                    complexity_max: $file.complexity_max
                },
                activity: {
                    commits_30d: $file.commits_30d,
                    commits_90d: $file.commits_90d,
                    last_modified: $file.last_modified
                }
            },
            symbols: $symbols,
            dependencies: {
                includes: $includes,
                included_by: $included_by
            },
            error_history: $errors,
            patterns: $patterns
        }'
}

cmd_file_metrics() {
    local path="$1"

    if [[ -z "$path" ]]; then
        echo '{"error": "Usage: file_metrics <path>"}'
        return 1
    fi

    local file_info=$(sqlite_json_one "
        SELECT
            f.*,
            (SELECT COUNT(*) FROM symbols WHERE file_id = f.id AND kind IN ('function', 'method')) as func_count,
            (SELECT COUNT(*) FROM symbols WHERE file_id = f.id AND kind IN ('struct', 'class', 'enum')) as type_count,
            (SELECT COUNT(*) FROM symbols WHERE file_id = f.id AND kind = 'macro') as macro_count,
            (SELECT COUNT(*) FROM symbols WHERE file_id = f.id AND kind IN ('variable', 'constant')) as var_count
        FROM files f
        WHERE f.path = '$path'
    ")

    if [[ "$file_info" == "{}" ]]; then
        echo '{"error": "File not found: '"$path"'"}'
        return 1
    fi

    echo "$file_info" | jq '{
        file: .path,
        size: {
            lines_total: .lines_total,
            lines_code: .lines_code,
            lines_comment: .lines_comment,
            lines_blank: .lines_blank
        },
        complexity: {
            cyclomatic_total: .complexity_sum,
            cyclomatic_avg: .complexity_avg,
            cyclomatic_max: .complexity_max
        },
        structure: {
            functions: .func_count,
            types: .type_count,
            macros: .macro_count,
            variables: .var_count
        },
        quality: {
            documentation_score: .documentation_score,
            has_tests: (.has_tests == 1),
            technical_debt_score: .technical_debt_score
        },
        activity: {
            commits_30d: .commits_30d,
            commits_90d: .commits_90d,
            commits_365d: .commits_365d,
            last_modified: .last_modified
        },
        flags: {
            is_critical: (.is_critical == 1),
            security_sensitive: (.security_sensitive == 1)
        }
    }'
}

cmd_file_impact() {
    local path="$1"

    if [[ -z "$path" ]]; then
        echo '{"error": "Usage: file_impact <path>"}'
        return 1
    fi

    local file_id=$(sqlite_json_one "SELECT id FROM files WHERE path = '$path'" | jq -r '.id')

    if [[ -z "$file_id" || "$file_id" == "null" ]]; then
        echo '{"error": "File not found: '"$path"'"}'
        return 1
    fi

    # Fichiers qui incluent ce fichier (impact include)
    local include_impact=$(sqlite_json "
        SELECT DISTINCT f.path, f.is_critical
        FROM file_relations fr
        JOIN files f ON fr.source_file_id = f.id
        WHERE fr.target_file_id = $file_id
        AND fr.relation_type = 'includes'
    ")

    # Fichiers qui appellent des symboles de ce fichier (impact direct)
    local direct_impact=$(sqlite_json "
        SELECT DISTINCT
            f.path,
            f.is_critical,
            GROUP_CONCAT(DISTINCT s_target.name) as symbols_called
        FROM relations r
        JOIN symbols s_source ON r.source_id = s_source.id
        JOIN symbols s_target ON r.target_id = s_target.id
        JOIN files f ON s_source.file_id = f.id
        WHERE s_target.file_id = $file_id
        AND r.relation_type = 'calls'
        AND s_source.file_id != $file_id
        GROUP BY f.path
    ")

    # Compter les fichiers critiques impactés
    local critical_count=$(echo "$direct_impact" "$include_impact" | jq -s 'add | map(select(.is_critical == 1)) | length')
    local total_count=$(echo "$direct_impact" "$include_impact" | jq -s 'add | length')

    jq -n \
        --arg path "$path" \
        --argjson direct "$direct_impact" \
        --argjson include "$include_impact" \
        --argjson total "$total_count" \
        --argjson critical "$critical_count" \
        '{
            file: $path,
            direct_impact: $direct,
            include_impact: $include,
            summary: {
                total_files_impacted: $total,
                critical_files_impacted: $critical
            }
        }'
}

cmd_symbol_callers() {
    local symbol_name="$1"
    local file_path="$2"

    if [[ -z "$symbol_name" ]]; then
        echo '{"error": "Usage: symbol_callers <name> [file_path]"}'
        return 1
    fi

    local file_filter=""
    if [[ -n "$file_path" ]]; then
        file_filter="AND f.path = '$file_path'"
    fi

    # Trouver le symbole cible
    local target=$(sqlite_json_one "
        SELECT s.id, s.name, s.kind, f.path as file
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE s.name = '$symbol_name'
        $file_filter
        LIMIT 1
    ")

    if [[ "$target" == "{}" ]]; then
        echo '{"error": "Symbol not found: '"$symbol_name"'"}'
        return 1
    fi

    local target_id=$(echo "$target" | jq -r '.id')

    # Niveau 1 : appelants directs
    local level_1=$(sqlite_json "
        SELECT
            s.name, s.kind, f.path as file, r.location_line as line,
            f.is_critical
        FROM relations r
        JOIN symbols s ON r.source_id = s.id
        JOIN files f ON s.file_id = f.id
        WHERE r.target_id = $target_id
        AND r.relation_type = 'calls'
    ")

    # Niveau 2 : appelants des appelants (si niveau 1 non vide)
    local level_2="[]"
    local l1_ids=$(echo "$level_1" | jq -r '.[].name' | head -20 | tr '\n' ',' | sed 's/,$//')
    if [[ -n "$l1_ids" ]]; then
        level_2=$(sqlite_json "
            SELECT DISTINCT
                s2.name, s2.kind, f2.path as file, r2.location_line as line,
                f2.is_critical
            FROM relations r
            JOIN symbols s1 ON r.target_id = s1.id
            JOIN relations r2 ON r2.target_id = s1.id
            JOIN symbols s2 ON r2.source_id = s2.id
            JOIN files f2 ON s2.file_id = f2.id
            WHERE r.target_id = $target_id
            AND r.relation_type = 'calls'
            AND r2.relation_type = 'calls'
            AND s2.id NOT IN (SELECT source_id FROM relations WHERE target_id = $target_id)
            LIMIT 50
        ")
    fi

    # Compter les fichiers affectés
    local files_affected=$(echo "$level_1" "$level_2" | jq -s 'add | [.[].file] | unique')
    local critical_count=$(echo "$level_1" "$level_2" | jq -s 'add | map(select(.is_critical == 1)) | length')

    jq -n \
        --argjson symbol "$target" \
        --argjson l1 "$level_1" \
        --argjson l2 "$level_2" \
        --argjson files "$files_affected" \
        --argjson critical "$critical_count" \
        '{
            symbol: {
                name: $symbol.name,
                file: $symbol.file,
                kind: $symbol.kind
            },
            callers: {
                level_1: $l1,
                level_2: $l2
            },
            summary: {
                total_callers: (($l1 | length) + ($l2 | length)),
                critical_callers: $critical,
                files_affected: $files
            }
        }'
}

cmd_symbol_callees() {
    local symbol_name="$1"
    local file_path="$2"

    if [[ -z "$symbol_name" ]]; then
        echo '{"error": "Usage: symbol_callees <name> [file_path]"}'
        return 1
    fi

    local file_filter=""
    if [[ -n "$file_path" ]]; then
        file_filter="AND f.path = '$file_path'"
    fi

    # Trouver le symbole source
    local source=$(sqlite_json_one "
        SELECT s.id, s.name, s.kind, f.path as file
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE s.name = '$symbol_name'
        $file_filter
        LIMIT 1
    ")

    if [[ "$source" == "{}" ]]; then
        echo '{"error": "Symbol not found: '"$symbol_name"'"}'
        return 1
    fi

    local source_id=$(echo "$source" | jq -r '.id')

    # Symboles appelés
    local callees=$(sqlite_json "
        SELECT
            s.name, s.kind, f.path as file, r.location_line as line
        FROM relations r
        JOIN symbols s ON r.target_id = s.id
        JOIN files f ON s.file_id = f.id
        WHERE r.source_id = $source_id
        AND r.relation_type = 'calls'
    ")

    # Types utilisés
    local types_used=$(sqlite_json "
        SELECT
            s.name, f.path as file
        FROM relations r
        JOIN symbols s ON r.target_id = s.id
        JOIN files f ON s.file_id = f.id
        WHERE r.source_id = $source_id
        AND r.relation_type = 'uses_type'
    ")

    jq -n \
        --argjson symbol "$source" \
        --argjson callees "$callees" \
        --argjson types "$types_used" \
        '{
            symbol: {
                name: $symbol.name,
                file: $symbol.file
            },
            callees: {
                level_1: $callees
            },
            types_used: $types
        }'
}

cmd_error_history() {
    local file_path="$1"
    local days="${2:-180}"

    if [[ -z "$file_path" ]]; then
        echo '{"error": "Usage: error_history <file_path> [days]"}'
        return 1
    fi

    # Compatible GNU date (Linux) et BSD date (macOS)
    local cutoff_date
    cutoff_date=$(date -d "$days days ago" +%Y-%m-%d 2>/dev/null) || \
    cutoff_date=$(date -v-"${days}"d +%Y-%m-%d 2>/dev/null) || \
    cutoff_date=$(date +%Y-%m-%d)  # Fallback: aujourd'hui

    local errors=$(sqlite_json "
        SELECT
            id, error_type as type, severity, title, description,
            discovered_at, resolved_at, resolution, prevention,
            is_regression, jira_ticket
        FROM error_history
        WHERE (file_path = '$file_path' OR file_path LIKE '%$file_path')
        AND discovered_at >= '$cutoff_date'
        ORDER BY discovered_at DESC
        LIMIT 20
    ")

    # Statistiques
    local stats=$(sqlite_json_one "
        SELECT
            COUNT(*) as total,
            SUM(CASE WHEN severity = 'critical' THEN 1 ELSE 0 END) as critical,
            SUM(CASE WHEN severity = 'high' THEN 1 ELSE 0 END) as high,
            SUM(CASE WHEN severity = 'medium' THEN 1 ELSE 0 END) as medium,
            SUM(CASE WHEN severity = 'low' THEN 1 ELSE 0 END) as low,
            SUM(CASE WHEN is_regression = 1 THEN 1 ELSE 0 END) as regressions
        FROM error_history
        WHERE (file_path = '$file_path' OR file_path LIKE '%$file_path')
        AND discovered_at >= '$cutoff_date'
    ")

    jq -n \
        --arg file "$file_path" \
        --arg days "$days" \
        --argjson errors "$errors" \
        --argjson stats "$stats" \
        '{
            query: {
                file_path: $file,
                days: ($days | tonumber)
            },
            errors: $errors,
            statistics: {
                total_errors: $stats.total,
                by_severity: {
                    critical: $stats.critical,
                    high: $stats.high,
                    medium: $stats.medium,
                    low: $stats.low
                },
                regressions: $stats.regressions
            }
        }'
}

cmd_patterns() {
    local file_path="$1"
    local category="$2"

    local where_clause="WHERE is_active = 1"

    if [[ -n "$category" ]]; then
        where_clause="$where_clause AND category = '$category'"
    fi

    local patterns=$(sqlite_json "
        SELECT
            name, category, title, description, severity,
            good_example, bad_example, rationale
        FROM patterns
        $where_clause
        ORDER BY severity DESC, name
    ")

    # Séparer en patterns applicables et globaux
    if [[ -n "$file_path" ]]; then
        jq -n \
            --arg file "$file_path" \
            --argjson patterns "$patterns" \
            '{
                query: { file_path: $file },
                applicable_patterns: ($patterns | map(select(.file_pattern == null or ($file | test(.file_pattern // ".*"))))),
                project_patterns: ($patterns | map(select(.scope == "project" or .module == null)))
            }'
    else
        jq -n \
            --argjson patterns "$patterns" \
            '{
                patterns: $patterns
            }'
    fi
}

cmd_architecture_decisions() {
    local module="$1"

    local where_clause="WHERE status = 'accepted'"

    if [[ -n "$module" ]]; then
        where_clause="$where_clause AND (affected_modules_json LIKE '%\"$module\"%' OR affected_modules_json IS NULL)"
    fi

    local decisions=$(sqlite_json "
        SELECT
            decision_id as id, title, status, context, decision,
            consequences, date_decided, decided_by
        FROM architecture_decisions
        $where_clause
        ORDER BY date_decided DESC
    ")

    jq -n \
        --arg module "$module" \
        --argjson decisions "$decisions" \
        '{
            query: { module: $module },
            decisions: $decisions
        }'
}

cmd_module_summary() {
    local module="$1"

    if [[ -z "$module" ]]; then
        echo '{"error": "Usage: module_summary <module>"}'
        return 1
    fi

    # Stats fichiers
    local file_stats=$(sqlite_json_one "
        SELECT
            COUNT(*) as total,
            SUM(CASE WHEN extension IN ('.c', '.cpp', '.py', '.js', '.ts', '.go', '.rs') THEN 1 ELSE 0 END) as sources,
            SUM(CASE WHEN extension IN ('.h', '.hpp', '.pyi') THEN 1 ELSE 0 END) as headers,
            SUM(CASE WHEN path LIKE '%test%' THEN 1 ELSE 0 END) as tests,
            SUM(CASE WHEN is_critical = 1 THEN 1 ELSE 0 END) as critical,
            SUM(lines_code) as total_lines,
            AVG(complexity_avg) as avg_complexity
        FROM files
        WHERE module = '$module'
    ")

    if [[ "$(echo "$file_stats" | jq '.total')" == "0" ]]; then
        echo '{"error": "Module not found: '"$module"'"}'
        return 1
    fi

    # Stats symboles
    local symbol_stats=$(sqlite_json_one "
        SELECT
            SUM(CASE WHEN s.kind IN ('function', 'method') THEN 1 ELSE 0 END) as functions,
            SUM(CASE WHEN s.kind IN ('struct', 'class', 'enum') THEN 1 ELSE 0 END) as types,
            SUM(CASE WHEN s.kind = 'macro' THEN 1 ELSE 0 END) as macros
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE f.module = '$module'
    ")

    # Erreurs récentes
    local error_count=$(sqlite_json_one "
        SELECT COUNT(*) as count
        FROM error_history e
        JOIN files f ON e.file_id = f.id
        WHERE f.module = '$module'
        AND e.discovered_at >= date('now', '-90 days')
    " | jq '.count')

    # Dépendances
    local depends_on=$(sqlite_json "
        SELECT DISTINCT f2.module
        FROM file_relations fr
        JOIN files f1 ON fr.source_file_id = f1.id
        JOIN files f2 ON fr.target_file_id = f2.id
        WHERE f1.module = '$module'
        AND f2.module != '$module'
        AND f2.module IS NOT NULL
    " | jq '[.[].module]')

    local depended_by=$(sqlite_json "
        SELECT DISTINCT f1.module
        FROM file_relations fr
        JOIN files f1 ON fr.source_file_id = f1.id
        JOIN files f2 ON fr.target_file_id = f2.id
        WHERE f2.module = '$module'
        AND f1.module != '$module'
        AND f1.module IS NOT NULL
    " | jq '[.[].module]')

    jq -n \
        --arg module "$module" \
        --argjson files "$file_stats" \
        --argjson symbols "$symbol_stats" \
        --argjson errors "$error_count" \
        --argjson depends_on "$depends_on" \
        --argjson depended_by "$depended_by" \
        '{
            module: $module,
            files: {
                total: $files.total,
                sources: $files.sources,
                headers: $files.headers,
                tests: $files.tests,
                critical: $files.critical
            },
            symbols: $symbols,
            metrics: {
                lines_total: $files.total_lines,
                complexity_avg: $files.avg_complexity
            },
            health: {
                errors_last_90d: $errors
            },
            dependencies: {
                depends_on: $depends_on,
                depended_by: $depended_by
            }
        }'
}

cmd_search_symbols() {
    local query="$1"
    local kind="$2"

    if [[ -z "$query" ]]; then
        echo '{"error": "Usage: search_symbols <query> [kind]"}'
        return 1
    fi

    # Convertir les wildcards glob en SQL LIKE
    local sql_pattern=$(echo "$query" | sed 's/\*/%/g; s/\?/_/g')

    local kind_filter=""
    if [[ -n "$kind" ]]; then
        kind_filter="AND s.kind = '$kind'"
    fi

    local results=$(sqlite_json "
        SELECT
            s.name, s.kind, f.path as file, s.signature, s.line_start as line
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE s.name LIKE '$sql_pattern'
        $kind_filter
        ORDER BY s.name
        LIMIT 50
    ")

    local total=$(sqlite_json_one "
        SELECT COUNT(*) as count
        FROM symbols s
        JOIN files f ON s.file_id = f.id
        WHERE s.name LIKE '$sql_pattern'
        $kind_filter
    " | jq '.count')

    jq -n \
        --arg query "$query" \
        --arg kind "$kind" \
        --argjson results "$results" \
        --argjson total "$total" \
        '{
            query: $query,
            kind_filter: $kind,
            results: $results,
            total: $total,
            returned: ($results | length)
        }'
}

cmd_list_modules() {
    local modules=$(sqlite_json "
        SELECT
            module,
            COUNT(*) as file_count,
            SUM(lines_code) as total_lines,
            SUM(CASE WHEN is_critical = 1 THEN 1 ELSE 0 END) as critical_files
        FROM files
        WHERE module IS NOT NULL
        GROUP BY module
        ORDER BY file_count DESC
    ")

    jq -n --argjson modules "$modules" '{ modules: $modules }'
}

cmd_list_critical_files() {
    local files=$(sqlite_json "
        SELECT
            path, module, language,
            is_critical, security_sensitive, criticality_reason,
            lines_code, complexity_max
        FROM files
        WHERE is_critical = 1 OR security_sensitive = 1
        ORDER BY is_critical DESC, security_sensitive DESC, path
    ")

    jq -n --argjson files "$files" '{ critical_files: $files }'
}

# =============================================================================
# COMMANDES POUR L'ANALYSE INCRÉMENTALE
# =============================================================================

cmd_get_checkpoint() {
    local branch="$1"

    if [[ -z "$branch" ]]; then
        echo '{"error": "Usage: get_checkpoint <branch>"}'
        return 1
    fi

    # Validation du nom de branche
    if ! validate_branch_name "$branch"; then
        echo '{"error": "Invalid branch name format"}'
        return 1
    fi

    local safe_branch=$(sql_escape "$branch")

    local checkpoint=$(sqlite_json_one "
        SELECT
            branch,
            last_commit,
            last_commit_short,
            last_commit_message,
            last_analyzed_at,
            analysis_count,
            files_analyzed,
            merge_base,
            target_branch,
            last_run_id,
            last_verdict,
            last_score
        FROM analysis_checkpoints
        WHERE branch = '$safe_branch'
    ")

    if [[ "$checkpoint" == "{}" ]]; then
        # Pas de checkpoint pour cette branche
        jq -n \
            --arg branch "$branch" \
            '{
                found: false,
                branch: $branch,
                message: "No checkpoint found for this branch"
            }'
    else
        echo "$checkpoint" | jq '. + {found: true}'
    fi
}

cmd_set_checkpoint() {
    local branch="$1"
    local commit="$2"
    local files_count="${3:-0}"
    local verdict="${4:-}"
    local score="${5:-}"

    if [[ -z "$branch" || -z "$commit" ]]; then
        echo '{"error": "Usage: set_checkpoint <branch> <commit> [files_count] [verdict] [score]"}'
        return 1
    fi

    # Validation des entrées
    if ! validate_branch_name "$branch"; then
        echo '{"error": "Invalid branch name format"}'
        return 1
    fi

    if ! validate_git_hash "$commit"; then
        echo '{"error": "Invalid commit hash format (must be hexadecimal)"}'
        return 1
    fi

    # Valider files_count est un nombre
    if ! [[ "$files_count" =~ ^[0-9]+$ ]]; then
        files_count=0
    fi

    # Valider score est un nombre ou vide
    if [[ -n "$score" ]] && ! [[ "$score" =~ ^[0-9]+$ ]]; then
        score=""
    fi

    # Échapper les valeurs pour SQL
    local safe_branch=$(sql_escape "$branch")
    local safe_verdict=$(sql_escape "$verdict")

    # Récupérer les infos du commit
    local commit_short=$(git rev-parse --short "$commit" 2>/dev/null || echo "$commit")
    local commit_message=$(git log -1 --format="%s" "$commit" 2>/dev/null || echo "")
    local safe_commit_message=$(sql_escape "$commit_message")

    # Trouver la branche cible (main ou develop)
    local target_branch=""
    if git rev-parse --verify main >/dev/null 2>&1; then
        target_branch="main"
    elif git rev-parse --verify develop >/dev/null 2>&1; then
        target_branch="develop"
    elif git rev-parse --verify master >/dev/null 2>&1; then
        target_branch="master"
    fi

    # Calculer le merge-base si possible
    local merge_base=""
    if [[ -n "$target_branch" ]]; then
        merge_base=$(git merge-base "$commit" "$target_branch" 2>/dev/null || echo "")
    fi

    # Générer un run_id
    local run_id="run-$(date +%Y%m%d-%H%M%S)-$(echo $RANDOM | md5sum | head -c 8)"

    # Upsert avec SQLite (valeurs déjà validées et échappées)
    sqlite3 "$DB_PATH" "
        INSERT INTO analysis_checkpoints (
            branch, last_commit, last_commit_short, last_commit_message,
            last_analyzed_at, analysis_count, files_analyzed,
            merge_base, target_branch, last_run_id, last_verdict, last_score
        ) VALUES (
            '$safe_branch', '$commit', '$commit_short', '$safe_commit_message',
            datetime('now'), 1, $files_count,
            '$merge_base', '$target_branch', '$run_id', '$safe_verdict', ${score:-NULL}
        )
        ON CONFLICT(branch) DO UPDATE SET
            last_commit = excluded.last_commit,
            last_commit_short = excluded.last_commit_short,
            last_commit_message = excluded.last_commit_message,
            last_analyzed_at = excluded.last_analyzed_at,
            analysis_count = analysis_count + 1,
            files_analyzed = excluded.files_analyzed,
            merge_base = excluded.merge_base,
            last_run_id = excluded.last_run_id,
            last_verdict = excluded.last_verdict,
            last_score = excluded.last_score;
    "

    # Retourner le checkpoint mis à jour
    cmd_get_checkpoint "$branch"
}

cmd_reset_checkpoint() {
    local branch="$1"

    if [[ -z "$branch" ]]; then
        echo '{"error": "Usage: reset_checkpoint <branch>"}'
        return 1
    fi

    # Validation du nom de branche
    if ! validate_branch_name "$branch"; then
        echo '{"error": "Invalid branch name format"}'
        return 1
    fi

    local safe_branch=$(sql_escape "$branch")

    # Supprimer le checkpoint
    sqlite3 "$DB_PATH" "DELETE FROM analysis_checkpoints WHERE branch = '$safe_branch';"

    jq -n \
        --arg branch "$branch" \
        '{
            success: true,
            branch: $branch,
            message: "Checkpoint deleted for branch"
        }'
}

cmd_list_checkpoints() {
    local checkpoints=$(sqlite_json "
        SELECT
            branch,
            last_commit_short,
            last_analyzed_at,
            analysis_count,
            files_analyzed,
            last_verdict,
            last_score
        FROM analysis_checkpoints
        ORDER BY last_analyzed_at DESC
    ")

    jq -n --argjson checkpoints "$checkpoints" '{ checkpoints: $checkpoints }'
}

# =============================================================================
# COMMANDES POUR L'ENREGISTREMENT DES ANALYSES
# =============================================================================

cmd_record_pipeline_run() {
    local branch="$1"
    local commit="$2"
    local score="${3:-0}"
    local verdict="${4:-UNKNOWN}"
    local files_count="${5:-0}"

    if [[ -z "$branch" || -z "$commit" ]]; then
        echo '{"error": "Usage: record_pipeline_run <branch> <commit> <score> <verdict> <files_count>"}'
        return 1
    fi

    # Validation des entrées
    if ! validate_branch_name "$branch"; then
        echo '{"error": "Invalid branch name format"}'
        return 1
    fi

    if ! validate_git_hash "$commit"; then
        echo '{"error": "Invalid commit hash format (must be hexadecimal)"}'
        return 1
    fi

    # Valider score est un nombre
    if ! [[ "$score" =~ ^[0-9]+$ ]]; then
        score=0
    fi

    # Valider files_count est un nombre
    if ! [[ "$files_count" =~ ^[0-9]+$ ]]; then
        files_count=0
    fi

    # Échapper les valeurs pour SQL
    local safe_branch=$(sql_escape "$branch")
    local safe_verdict=$(sql_escape "$verdict")

    # Récupérer les infos du commit
    local commit_short=$(git rev-parse --short "$commit" 2>/dev/null || echo "$commit")
    local commit_message=$(git log -1 --format="%s" "$commit" 2>/dev/null || echo "")
    local commit_author=$(git log -1 --format="%an" "$commit" 2>/dev/null || echo "")
    local safe_commit_message=$(sql_escape "$commit_message")
    local safe_commit_author=$(sql_escape "$commit_author")

    # Générer un run_id unique
    local run_id="run-$(date +%Y%m%d-%H%M%S)-$(echo $RANDOM | md5sum | head -c 8)"
    local started_at=$(date -Iseconds)

    # Calculer les compteurs de sévérité basés sur le score
    local issues_critical=0
    local issues_high=0
    local issues_medium=0
    local issues_low=0

    # Estimation basique basée sur le score
    if [[ "$score" -lt 40 ]]; then
        issues_critical=1
    elif [[ "$score" -lt 60 ]]; then
        issues_high=1
    elif [[ "$score" -lt 80 ]]; then
        issues_medium=1
    else
        issues_low=0
    fi

    # Mapper le verdict vers un run_status
    local run_status="completed"

    # Insérer dans pipeline_runs
    sqlite3 "$DB_PATH" "
        INSERT INTO pipeline_runs (
            run_id, commit_hash, commit_message, commit_author,
            branch_source, status, overall_score, recommendation,
            issues_critical, issues_high, issues_medium, issues_low,
            files_analyzed, started_at, completed_at
        ) VALUES (
            '$run_id', '$commit', '$safe_commit_message', '$safe_commit_author',
            '$safe_branch', '$run_status', $score, '$safe_verdict',
            $issues_critical, $issues_high, $issues_medium, $issues_low,
            $files_count, '$started_at', datetime('now')
        );
    "

    # Retourner le résultat
    jq -n \
        --arg run_id "$run_id" \
        --arg branch "$branch" \
        --arg commit "$commit" \
        --argjson score "$score" \
        --arg verdict "$verdict" \
        --argjson files "$files_count" \
        '{
            success: true,
            run_id: $run_id,
            branch: $branch,
            commit: $commit,
            score: $score,
            verdict: $verdict,
            files_analyzed: $files,
            message: "Pipeline run recorded successfully"
        }'
}

cmd_list_pipeline_runs() {
    local limit="${1:-10}"

    # Valider limit est un nombre
    if ! [[ "$limit" =~ ^[0-9]+$ ]]; then
        limit=10
    fi

    local runs=$(sqlite_json "
        SELECT
            run_id,
            branch_source as branch,
            commit_hash,
            overall_score as score,
            recommendation as verdict,
            files_analyzed,
            started_at
        FROM pipeline_runs
        ORDER BY started_at DESC
        LIMIT $limit
    ")

    jq -n \
        --argjson runs "$runs" \
        --argjson limit "$limit" \
        '{
            limit: $limit,
            runs: $runs
        }'
}

# =============================================================================
# MAIN
# =============================================================================

cmd="$1"
shift || true

# Wrapper pour exécuter une commande avec logging
run_with_logging() {
    local cmd_name="$1"
    shift
    local cmd_func="$1"
    shift

    log_start "$cmd_name" "$@"

    local result
    local exit_code=0
    result=$("$cmd_func" "$@") || exit_code=$?

    local result_size=${#result}
    log_end "$cmd_name" "$exit_code" "$result_size"

    if [[ "$LOG_LEVEL" -ge 2 ]]; then
        log_result "$result"
    fi

    echo "$result"
    return $exit_code
}

case "$cmd" in
    file_context)
        run_with_logging "file_context" cmd_file_context "$@"
        ;;
    file_metrics)
        run_with_logging "file_metrics" cmd_file_metrics "$@"
        ;;
    file_impact)
        run_with_logging "file_impact" cmd_file_impact "$@"
        ;;
    symbol_callers)
        run_with_logging "symbol_callers" cmd_symbol_callers "$@"
        ;;
    symbol_callees)
        run_with_logging "symbol_callees" cmd_symbol_callees "$@"
        ;;
    error_history)
        run_with_logging "error_history" cmd_error_history "$@"
        ;;
    patterns)
        run_with_logging "patterns" cmd_patterns "$@"
        ;;
    architecture_decisions)
        run_with_logging "architecture_decisions" cmd_architecture_decisions "$@"
        ;;
    module_summary)
        run_with_logging "module_summary" cmd_module_summary "$@"
        ;;
    search_symbols)
        run_with_logging "search_symbols" cmd_search_symbols "$@"
        ;;
    list_modules)
        run_with_logging "list_modules" cmd_list_modules
        ;;
    list_critical_files)
        run_with_logging "list_critical_files" cmd_list_critical_files
        ;;
    get_checkpoint)
        run_with_logging "get_checkpoint" cmd_get_checkpoint "$@"
        ;;
    set_checkpoint)
        run_with_logging "set_checkpoint" cmd_set_checkpoint "$@"
        ;;
    reset_checkpoint)
        run_with_logging "reset_checkpoint" cmd_reset_checkpoint "$@"
        ;;
    list_checkpoints)
        run_with_logging "list_checkpoints" cmd_list_checkpoints
        ;;
    record_pipeline_run)
        run_with_logging "record_pipeline_run" cmd_record_pipeline_run "$@"
        ;;
    list_pipeline_runs)
        run_with_logging "list_pipeline_runs" cmd_list_pipeline_runs "$@"
        ;;
    help|--help|-h)
        head -40 "$SCRIPT_DIR/query.sh" | tail -n +2 | sed 's/^# //' | sed 's/^#//'
        ;;
    *)
        log_entry 1 "ERROR: Unknown command: $cmd"
        echo '{"error": "Unknown command: '"$cmd"'. Use --help for usage."}'
        exit 1
        ;;
esac
