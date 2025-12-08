-- ============================================================================
-- AGENTDB - SCHÉMA COMPLET
-- Version: 2.0
-- Description: Base de données contextuelle pour le système multi-agents
-- ============================================================================

-- ============================================================================
-- PRAGMA CONFIGURATION
-- ============================================================================

PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA cache_size = -64000;  -- 64MB cache
PRAGMA temp_store = MEMORY;

-- ============================================================================
-- PILIER 1 : LE GRAPHE DE DÉPENDANCES
-- ============================================================================

-- Table des fichiers
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- Identification
    path TEXT UNIQUE NOT NULL,
    filename TEXT NOT NULL,
    extension TEXT,

    -- Classification
    module TEXT,
    layer TEXT,
    file_type TEXT NOT NULL DEFAULT 'source',
    language TEXT,

    -- Criticité
    is_critical BOOLEAN DEFAULT 0,
    criticality_reason TEXT,
    security_sensitive BOOLEAN DEFAULT 0,

    -- Métriques de code
    lines_total INTEGER DEFAULT 0,
    lines_code INTEGER DEFAULT 0,
    lines_comment INTEGER DEFAULT 0,
    lines_blank INTEGER DEFAULT 0,
    complexity_sum INTEGER DEFAULT 0,
    complexity_avg REAL DEFAULT 0,
    complexity_max INTEGER DEFAULT 0,

    -- Métriques d'activité
    commits_30d INTEGER DEFAULT 0,
    commits_90d INTEGER DEFAULT 0,
    commits_365d INTEGER DEFAULT 0,
    contributors_json TEXT,
    last_modified TEXT,
    created_at TEXT,

    -- Métriques de qualité
    has_tests BOOLEAN DEFAULT 0,
    test_file_path TEXT,
    documentation_score INTEGER DEFAULT 0,
    technical_debt_score INTEGER DEFAULT 0,

    -- Métadonnées
    content_hash TEXT,
    indexed_at TEXT NOT NULL DEFAULT (datetime('now')),
    index_version INTEGER DEFAULT 1
);

-- Table des symboles
CREATE TABLE IF NOT EXISTS symbols (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_id INTEGER NOT NULL,

    -- Identification
    name TEXT NOT NULL,
    qualified_name TEXT,

    -- Classification
    kind TEXT NOT NULL,

    -- Localisation
    line_start INTEGER,
    line_end INTEGER,
    column_start INTEGER,
    column_end INTEGER,

    -- Signature (fonctions)
    signature TEXT,
    return_type TEXT,
    parameters_json TEXT,
    is_variadic BOOLEAN DEFAULT 0,

    -- Structure (struct/class/enum)
    fields_json TEXT,
    base_classes_json TEXT,
    size_bytes INTEGER,

    -- Visibilité
    visibility TEXT DEFAULT 'public',
    is_exported BOOLEAN DEFAULT 0,
    is_static BOOLEAN DEFAULT 0,
    is_inline BOOLEAN DEFAULT 0,

    -- Métriques
    complexity INTEGER DEFAULT 0,
    lines_of_code INTEGER DEFAULT 0,
    cognitive_complexity INTEGER DEFAULT 0,
    nesting_depth INTEGER DEFAULT 0,

    -- Documentation
    doc_comment TEXT,
    has_doc BOOLEAN DEFAULT 0,
    doc_quality INTEGER DEFAULT 0,

    -- Métadonnées
    attributes_json TEXT,
    hash TEXT,
    indexed_at TEXT NOT NULL DEFAULT (datetime('now')),

    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE,
    UNIQUE(file_id, name, kind, line_start)
);

-- Table des relations entre symboles
CREATE TABLE IF NOT EXISTS relations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_id INTEGER NOT NULL,
    target_id INTEGER NOT NULL,

    -- Type de relation
    relation_type TEXT NOT NULL,

    -- Localisation
    location_file_id INTEGER,
    location_line INTEGER,
    location_column INTEGER,

    -- Métadonnées
    count INTEGER DEFAULT 1,
    is_direct BOOLEAN DEFAULT 1,
    is_conditional BOOLEAN DEFAULT 0,
    context TEXT,

    FOREIGN KEY (source_id) REFERENCES symbols(id) ON DELETE CASCADE,
    FOREIGN KEY (target_id) REFERENCES symbols(id) ON DELETE CASCADE,
    FOREIGN KEY (location_file_id) REFERENCES files(id) ON DELETE SET NULL
);

-- Table des relations entre fichiers
CREATE TABLE IF NOT EXISTS file_relations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_file_id INTEGER NOT NULL,
    target_file_id INTEGER NOT NULL,
    relation_type TEXT NOT NULL,
    is_direct BOOLEAN DEFAULT 1,
    line_number INTEGER,

    FOREIGN KEY (source_file_id) REFERENCES files(id) ON DELETE CASCADE,
    FOREIGN KEY (target_file_id) REFERENCES files(id) ON DELETE CASCADE,
    UNIQUE(source_file_id, target_file_id, relation_type)
);

-- ============================================================================
-- PILIER 2 : LA MÉMOIRE HISTORIQUE
-- ============================================================================

-- Historique des erreurs
CREATE TABLE IF NOT EXISTS error_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- Identification
    file_id INTEGER,
    file_path TEXT NOT NULL,
    symbol_name TEXT,
    symbol_id INTEGER,

    -- Classification
    error_type TEXT NOT NULL,
    severity TEXT NOT NULL,
    cwe_id TEXT,

    -- Description
    title TEXT NOT NULL,
    description TEXT,
    root_cause TEXT,
    symptoms TEXT,

    -- Résolution
    resolution TEXT,
    prevention TEXT,
    fix_commit TEXT,
    fix_diff TEXT,

    -- Contexte
    discovered_at TEXT NOT NULL,
    resolved_at TEXT,
    discovered_by TEXT,
    reported_in TEXT,
    jira_ticket TEXT,
    environment TEXT,

    -- Commits
    introducing_commit TEXT,
    related_commits_json TEXT,

    -- Métadonnées
    is_regression BOOLEAN DEFAULT 0,
    original_error_id INTEGER,
    tags_json TEXT,
    extra_data_json TEXT,

    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE SET NULL,
    FOREIGN KEY (symbol_id) REFERENCES symbols(id) ON DELETE SET NULL,
    FOREIGN KEY (original_error_id) REFERENCES error_history(id) ON DELETE SET NULL
);

-- Historique des runs du pipeline
CREATE TABLE IF NOT EXISTS pipeline_runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id TEXT UNIQUE NOT NULL,

    -- Contexte Git
    commit_hash TEXT NOT NULL,
    commit_message TEXT,
    commit_author TEXT,
    branch_source TEXT,
    branch_target TEXT,
    merge_type TEXT,

    -- Contexte JIRA
    jira_key TEXT,
    jira_type TEXT,
    jira_summary TEXT,

    -- Résultats
    status TEXT NOT NULL,
    overall_score INTEGER,
    recommendation TEXT,

    -- Scores par agent
    score_analyzer INTEGER,
    score_security INTEGER,
    score_reviewer INTEGER,
    score_risk INTEGER,

    -- Issues
    issues_critical INTEGER DEFAULT 0,
    issues_high INTEGER DEFAULT 0,
    issues_medium INTEGER DEFAULT 0,
    issues_low INTEGER DEFAULT 0,
    issues_json TEXT,

    -- Fichiers
    files_analyzed INTEGER,
    files_json TEXT,

    -- Rapports
    report_path TEXT,
    report_json_path TEXT,
    context_path TEXT,

    -- Timing
    started_at TEXT NOT NULL,
    completed_at TEXT,
    duration_ms INTEGER,

    -- Métadonnées
    trigger TEXT,
    pipeline_version TEXT,
    agents_used_json TEXT
);

-- Snapshot temporaire
CREATE TABLE IF NOT EXISTS snapshot_symbols (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id TEXT NOT NULL,
    file_path TEXT NOT NULL,
    symbol_name TEXT NOT NULL,
    symbol_kind TEXT NOT NULL,
    signature TEXT,
    complexity INTEGER,
    line_start INTEGER,
    line_end INTEGER,
    hash TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- ============================================================================
-- PILIER 3 : LA BASE DE CONNAISSANCES
-- ============================================================================

-- Patterns du projet
CREATE TABLE IF NOT EXISTS patterns (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- Identification
    name TEXT UNIQUE NOT NULL,
    category TEXT NOT NULL,

    -- Scope
    scope TEXT DEFAULT 'project',
    module TEXT,
    file_pattern TEXT,

    -- Description
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    rationale TEXT,

    -- Exemples
    good_example TEXT,
    bad_example TEXT,
    explanation TEXT,

    -- Règles
    rules_json TEXT,

    -- Métadonnées
    severity TEXT DEFAULT 'warning',
    is_active BOOLEAN DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT,
    created_by TEXT,

    -- Références
    related_adr TEXT,
    external_link TEXT,
    examples_in_code_json TEXT
);

-- Décisions architecturales
CREATE TABLE IF NOT EXISTS architecture_decisions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    decision_id TEXT UNIQUE NOT NULL,

    -- Statut
    status TEXT NOT NULL,
    superseded_by TEXT,

    -- Contenu
    title TEXT NOT NULL,
    context TEXT NOT NULL,
    decision TEXT NOT NULL,
    consequences TEXT,
    alternatives TEXT,

    -- Scope
    affected_modules_json TEXT,
    affected_files_json TEXT,

    -- Métadonnées
    date_proposed TEXT,
    date_decided TEXT,
    decided_by TEXT,
    stakeholders_json TEXT,

    -- Liens
    related_adrs_json TEXT,
    jira_tickets_json TEXT,
    documentation_link TEXT
);

-- Chemins critiques
CREATE TABLE IF NOT EXISTS critical_paths (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    pattern TEXT UNIQUE NOT NULL,
    reason TEXT NOT NULL,
    severity TEXT DEFAULT 'high',
    added_by TEXT,
    added_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- ============================================================================
-- PILIER 4 : TABLES UTILITAIRES
-- ============================================================================

-- Métadonnées de la base
CREATE TABLE IF NOT EXISTS agentdb_meta (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- Insert initial meta
INSERT OR IGNORE INTO agentdb_meta (key, value) VALUES
    ('schema_version', '2.0'),
    ('created_at', datetime('now')),
    ('project_name', 'unknown'),
    ('project_language', 'unknown');

-- ============================================================================
-- INDEX POUR PERFORMANCE
-- ============================================================================

-- Index sur files
CREATE INDEX IF NOT EXISTS idx_files_module ON files(module);
CREATE INDEX IF NOT EXISTS idx_files_is_critical ON files(is_critical);
CREATE INDEX IF NOT EXISTS idx_files_language ON files(language);
CREATE INDEX IF NOT EXISTS idx_files_path_pattern ON files(path);

-- Index sur symbols
CREATE INDEX IF NOT EXISTS idx_symbols_file_id ON symbols(file_id);
CREATE INDEX IF NOT EXISTS idx_symbols_name ON symbols(name);
CREATE INDEX IF NOT EXISTS idx_symbols_kind ON symbols(kind);
CREATE INDEX IF NOT EXISTS idx_symbols_qualified ON symbols(qualified_name);
CREATE INDEX IF NOT EXISTS idx_symbols_file_kind ON symbols(file_id, kind);

-- Index sur relations
CREATE INDEX IF NOT EXISTS idx_relations_source ON relations(source_id);
CREATE INDEX IF NOT EXISTS idx_relations_target ON relations(target_id);
CREATE INDEX IF NOT EXISTS idx_relations_type ON relations(relation_type);
CREATE INDEX IF NOT EXISTS idx_relations_source_type ON relations(source_id, relation_type);
CREATE INDEX IF NOT EXISTS idx_relations_target_type ON relations(target_id, relation_type);

-- Index sur file_relations
CREATE INDEX IF NOT EXISTS idx_file_relations_source ON file_relations(source_file_id);
CREATE INDEX IF NOT EXISTS idx_file_relations_target ON file_relations(target_file_id);

-- Index sur error_history
CREATE INDEX IF NOT EXISTS idx_errors_file_id ON error_history(file_id);
CREATE INDEX IF NOT EXISTS idx_errors_file_path ON error_history(file_path);
CREATE INDEX IF NOT EXISTS idx_errors_type ON error_history(error_type);
CREATE INDEX IF NOT EXISTS idx_errors_severity ON error_history(severity);
CREATE INDEX IF NOT EXISTS idx_errors_discovered ON error_history(discovered_at);

-- Index sur pipeline_runs
CREATE INDEX IF NOT EXISTS idx_runs_commit ON pipeline_runs(commit_hash);
CREATE INDEX IF NOT EXISTS idx_runs_jira ON pipeline_runs(jira_key);
CREATE INDEX IF NOT EXISTS idx_runs_status ON pipeline_runs(status);
CREATE INDEX IF NOT EXISTS idx_runs_started ON pipeline_runs(started_at);

-- Index sur patterns
CREATE INDEX IF NOT EXISTS idx_patterns_category ON patterns(category);
CREATE INDEX IF NOT EXISTS idx_patterns_module ON patterns(module);
CREATE INDEX IF NOT EXISTS idx_patterns_active ON patterns(is_active);

-- Index sur architecture_decisions
CREATE INDEX IF NOT EXISTS idx_adr_status ON architecture_decisions(status);

-- Index sur snapshot
CREATE INDEX IF NOT EXISTS idx_snapshot_run ON snapshot_symbols(run_id);

-- ============================================================================
-- VUES UTILITAIRES
-- ============================================================================

-- Vue : Fichiers avec leurs stats de symboles
CREATE VIEW IF NOT EXISTS v_files_with_stats AS
SELECT
    f.*,
    COUNT(s.id) as symbol_count,
    SUM(CASE WHEN s.kind = 'function' THEN 1 ELSE 0 END) as function_count,
    SUM(CASE WHEN s.kind IN ('struct', 'class') THEN 1 ELSE 0 END) as type_count,
    AVG(s.complexity) as avg_complexity
FROM files f
LEFT JOIN symbols s ON s.file_id = f.id
GROUP BY f.id;

-- Vue : Symboles avec leur contexte fichier
CREATE VIEW IF NOT EXISTS v_symbols_with_context AS
SELECT
    s.*,
    f.path as file_path,
    f.module as file_module,
    f.is_critical as file_is_critical,
    f.language as file_language
FROM symbols s
JOIN files f ON s.file_id = f.id;

-- Vue : Relations avec noms des symboles
CREATE VIEW IF NOT EXISTS v_relations_named AS
SELECT
    r.id,
    r.relation_type,
    r.count,
    r.location_line,
    src.name as source_name,
    src.kind as source_kind,
    src_f.path as source_file,
    tgt.name as target_name,
    tgt.kind as target_kind,
    tgt_f.path as target_file
FROM relations r
JOIN symbols src ON r.source_id = src.id
JOIN symbols tgt ON r.target_id = tgt.id
JOIN files src_f ON src.file_id = src_f.id
JOIN files tgt_f ON tgt.file_id = tgt_f.id;

-- Vue : Erreurs récentes (30 jours)
CREATE VIEW IF NOT EXISTS v_recent_errors AS
SELECT * FROM error_history
WHERE discovered_at >= datetime('now', '-30 days')
ORDER BY discovered_at DESC;

-- Vue : Fichiers à risque (critiques + erreurs récentes)
CREATE VIEW IF NOT EXISTS v_high_risk_files AS
SELECT
    f.id,
    f.path,
    f.module,
    f.is_critical,
    f.complexity_avg,
    COUNT(e.id) as error_count,
    MAX(e.severity) as max_severity
FROM files f
LEFT JOIN error_history e ON e.file_id = f.id
    AND e.discovered_at >= datetime('now', '-180 days')
WHERE f.is_critical = 1 OR e.id IS NOT NULL
GROUP BY f.id
ORDER BY f.is_critical DESC, error_count DESC;
