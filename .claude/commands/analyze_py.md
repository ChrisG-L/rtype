# Commande /analyze - Analyse Incrémentale

Tu dois orchestrer une analyse incrémentale intelligente du code en utilisant les 5 agents spécialisés et AgentDB.

## OBLIGATOIRE
Lance le --all tout le temps !

## ARGUMENT REÇU

$ARGUMENTS

---

## ÉTAPE 0 : Initialisation (logs + AgentDB)

**Avant toute analyse, nettoyer les logs et mettre à jour AgentDB.**

### 0a. Nettoyer les logs de la session précédente

```bash
# Réinitialiser le fichier de log pour cette session d'analyse
# Garde un header avec la date de début
LOG_FILE=".claude/logs/agentdb_queries.log"
mkdir -p .claude/logs 2>/dev/null || true
echo "[$(date '+%Y-%m-%d %H:%M:%S')] [system] === NEW ANALYSIS SESSION ===" > "$LOG_FILE"
```

### 0b. Mettre à jour AgentDB (incrémental)

```bash
# Mettre à jour AgentDB de manière incrémentale
# Cela ne réindexe que les fichiers modifiés depuis le dernier indexage
python .claude/scripts/bootstrap.py --incremental 2>/dev/null || true
```

**Comportement** :
- Logs : Réinitialisés à chaque analyse (garde uniquement la session courante)
- AgentDB : Si la base n'existe pas, affiche un avertissement mais continue
- Si aucun changement : retourne instantanément "Base already up to date"
- Si des fichiers ont changé : les réindexe en quelques secondes
- En cas d'erreur : continue l'analyse (AgentDB est optionnel)

**Important** : Cette étape est silencieuse en cas d'erreur pour ne pas bloquer l'analyse.

---

## ÉTAPE 1 : Déterminer le mode d'analyse

### Récupérer le contexte Git

```bash
# Branche actuelle
CURRENT_BRANCH=`git branch --show-current`

# HEAD actuel
HEAD_COMMIT=`git rev-parse HEAD`
HEAD_SHORT=`git rev-parse --short HEAD`
HEAD_MESSAGE=`git log -1 --format="%s" HEAD`

echo "Branche: $CURRENT_BRANCH"
echo "HEAD: $HEAD_SHORT - $HEAD_MESSAGE"
```

### Parser les arguments

| Argument | Mode | Description |
|----------|------|-------------|
| (vide) | `incremental` | Analyse depuis le checkpoint |
| `--all` | `full` | Analyse depuis le merge-base |
| `--reset` | `reset` | Met checkpoint à HEAD sans analyser |
| `--files <paths>` | `files` | Analyse fichiers spécifiques |
| `<hash>` | `commit` | Analyse un commit spécifique |

```bash
# Déterminer le mode d'analyse (POSIX-compatible pour bash/zsh)
MODE="incremental"  # Par défaut

case "$ARGUMENTS" in
    "--all")
        MODE="full"
        ;;
    "--reset")
        MODE="reset"
        ;;
    "--files"*)
        MODE="files"
        ;;
    ""|--*)
        # Vide ou autre option --xxx : rester en incremental
        ;;
    *)
        # Argument sans -- : probablement un hash de commit
        MODE="commit"
        ;;
esac

echo "Mode d'analyse: $MODE"
```

## ÉTAPE 2 : Récupérer ou calculer le point de départ

### Mode INCREMENTAL (par défaut, sans argument)

```bash
# Récupérer le checkpoint existant
CHECKPOINT=`bash .claude/agentdb/query.sh get_checkpoint "$CURRENT_BRANCH"`

if echo "$CHECKPOINT" | jq -e '.found == true' > /dev/null; then
    # Checkpoint trouvé
    LAST_COMMIT=`echo "$CHECKPOINT" | jq -r '.last_commit'`
    LAST_DATE=`echo "$CHECKPOINT" | jq -r '.last_analyzed_at'`
    LAST_VERDICT=`echo "$CHECKPOINT" | jq -r '.last_verdict'`

    echo "Checkpoint trouvé: $LAST_COMMIT ($LAST_DATE)"
    echo "Dernier verdict: $LAST_VERDICT"
else
    # Premier analyse sur cette branche
    # Utiliser le merge-base avec main/develop
    TARGET_BRANCH="main"
    if ! git rev-parse --verify main >/dev/null 2>&1; then
        TARGET_BRANCH="develop"
    fi
    if ! git rev-parse --verify $TARGET_BRANCH >/dev/null 2>&1; then
        TARGET_BRANCH="master"
    fi

    LAST_COMMIT=`git merge-base HEAD $TARGET_BRANCH 2>/dev/null || git rev-list --max-parents=0 HEAD`
    LAST_COMMIT_SHORT=`git rev-parse --short $LAST_COMMIT`
    echo "Premier analyse - Point de départ: $LAST_COMMIT_SHORT"
fi
```

### Mode FULL (`--all`)

```bash
# Ignorer le checkpoint, utiliser le merge-base
TARGET_BRANCH="main"
if ! git rev-parse --verify main >/dev/null 2>&1; then
    TARGET_BRANCH="develop"
fi

LAST_COMMIT=`git merge-base HEAD $TARGET_BRANCH 2>/dev/null || git rev-list --max-parents=0 HEAD`
LAST_COMMIT_SHORT=`git rev-parse --short $LAST_COMMIT`
echo "Mode --all: Analyse depuis $LAST_COMMIT_SHORT"
```

### Mode RESET (`--reset`)

```bash
# Mettre le checkpoint à HEAD sans analyser
bash .claude/agentdb/query.sh set_checkpoint "$CURRENT_BRANCH" "$HEAD_COMMIT" 0 "" ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                                                               ║"
echo "║  Checkpoint mis à jour : $HEAD_SHORT                          ║"
echo "║                                                               ║"
echo "║  Prochaine /analyze partira de ce point.                      ║"
echo "║                                                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

# TERMINER ICI - Ne pas continuer l'analyse
```

**IMPORTANT** : Si le mode est `--reset`, afficher le message ci-dessus et **TERMINER IMMÉDIATEMENT**. Ne pas lancer les agents.

### Mode FILES (`--files <paths>`)

Analyse des fichiers spécifiques sans utiliser le système de checkpoint.

```bash
# Exemple: /analyze --files src/server/UDPServer.cpp src/client/Client.cpp

# Extraire les fichiers de la liste d'arguments (POSIX-compatible)
FILES_TO_ANALYZE=""
PARSING_FILES=false
for arg in $ARGUMENTS; do
    if [ "$arg" = "--files" ]; then
        PARSING_FILES=true
        continue
    fi
    if [ "$PARSING_FILES" = "true" ]; then
        # Vérifier que le fichier existe
        if [ -f "$arg" ]; then
            FILES_TO_ANALYZE="$FILES_TO_ANALYZE $arg"
        else
            echo "⚠️  Fichier non trouvé: $arg"
        fi
    fi
done

# Valider qu'au moins un fichier est spécifié
if [ -z "$FILES_TO_ANALYZE" ]; then
    echo '{"error": "Aucun fichier valide spécifié. Usage: /analyze --files <file1> [file2] ..."}'
    # TERMINER
fi

FILES_COUNT=$(echo "$FILES_TO_ANALYZE" | wc -w)
```

**Workflow mode FILES** :
1. Ignorer complètement le checkpoint (ne pas le lire ni le mettre à jour)
2. Analyser uniquement les fichiers spécifiés
3. Utiliser HEAD comme référence pour le contexte
4. Ne PAS mettre à jour le checkpoint après l'analyse (c'est une analyse ponctuelle)

**Affichage** :
```
╔═══════════════════════════════════════════════════════════════╗
║  ANALYSE CIBLÉE (mode --files)                                ║
╠═══════════════════════════════════════════════════════════════╣
║  Branche      : {CURRENT_BRANCH}                              ║
║  HEAD         : {HEAD_SHORT}                                  ║
║  Fichiers     : {FILES_COUNT} fichiers spécifiés              ║
╠═══════════════════════════════════════════════════════════════╣
║  Fichiers à analyser :                                        ║
║  - src/server/UDPServer.cpp                                   ║
║  - src/client/Client.cpp                                      ║
╚═══════════════════════════════════════════════════════════════╝

⚠️  Note: Le checkpoint ne sera pas mis à jour (analyse ponctuelle)
```

### Mode COMMIT (`<hash>`)

Analyse un commit spécifique (diff entre parent et ce commit).

```bash
# Exemple: /analyze abc123
# ou: /analyze abc123..def456 (plage de commits)

COMMIT_ARG="$ARGUMENTS"

# Vérifier si c'est une plage (contient ..) - POSIX-compatible
case "$COMMIT_ARG" in
    *".."*)
        # Plage de commits: abc123..def456
        START_COMMIT="${COMMIT_ARG%%..*}"
        END_COMMIT="${COMMIT_ARG##*..}"
        ;;
    *)
        # Commit unique: analyser depuis son parent
        START_COMMIT=$(git rev-parse "$COMMIT_ARG^" 2>/dev/null)
        END_COMMIT="$COMMIT_ARG"
        ;;
esac

# Valider les commits
if ! git rev-parse --verify "$START_COMMIT" >/dev/null 2>&1; then
    echo '{"error": "Commit de départ invalide: '"$START_COMMIT"'"}'
    # TERMINER
fi

if ! git rev-parse --verify "$END_COMMIT" >/dev/null 2>&1; then
    echo '{"error": "Commit de fin invalide: '"$END_COMMIT"'"}'
    # TERMINER
fi

# Calculer le diff entre les deux commits
FILES_CHANGED=`git diff "$START_COMMIT".."$END_COMMIT" --name-only --diff-filter=ACMR | grep -E '\.(c|cpp|h|hpp|py|js|ts|go|rs|java)$' || true`

LAST_COMMIT="$START_COMMIT"
HEAD_COMMIT="$END_COMMIT"
```

**Workflow mode COMMIT** :
1. Ignorer le checkpoint (ne pas le lire)
2. Calculer le diff entre les commits spécifiés
3. Analyser les fichiers modifiés dans cette plage
4. Mettre à jour le checkpoint avec le commit de fin (optionnel, selon préférence)

**Affichage** :
```
╔═══════════════════════════════════════════════════════════════╗
║  ANALYSE DE COMMIT                                            ║
╠═══════════════════════════════════════════════════════════════╣
║  Branche      : {CURRENT_BRANCH}                              ║
║  Commit       : {END_COMMIT_SHORT}                            ║
║  Message      : {COMMIT_MESSAGE}                              ║
║  Diff depuis  : {START_COMMIT_SHORT}                          ║
║  Fichiers     : {FILES_COUNT} fichiers modifiés               ║
╚═══════════════════════════════════════════════════════════════╝
```

## ÉTAPE 3 : Calculer le diff unifié

```bash
# Calculer les fichiers modifiés entre LAST_COMMIT et HEAD
git diff $LAST_COMMIT..HEAD --name-only --diff-filter=ACMR
```

### Filtrer les fichiers de code

Garder uniquement :
- Extensions : `.c`, `.cpp`, `.h`, `.hpp`, `.py`, `.js`, `.ts`, `.go`, `.rs`, `.java`

Ignorer :
- `.md`, `.txt`, `.json`, `.yaml`, `.yml`, `.lock`
- Images, configs, etc.

### Vérifier s'il y a des changements

```bash
FILES_CHANGED=`git diff $LAST_COMMIT..HEAD --name-only --diff-filter=ACMR | grep -E '\.(c|cpp|h|hpp|py|js|ts|go|rs|java)$' || true`
FILES_COUNT=`echo "$FILES_CHANGED" | grep -c '.' || echo 0`
```

**Si FILES_COUNT == 0** :

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║  ✅ Rien à analyser depuis le dernier checkpoint              ║
║                                                               ║
║  Dernier checkpoint : {LAST_COMMIT_SHORT} ({LAST_DATE})       ║
║  HEAD actuel : {HEAD_SHORT}                                   ║
║                                                               ║
║  Utilisez /analyze --all pour forcer une analyse complète.    ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

**TERMINER ICI si aucun fichier à analyser.**

## ÉTAPE 3b : Transformer le rapport SonarQube (optionnel)

**Cette étape est optionnelle.** Si aucun rapport SonarQube n'est disponible, continuer normalement.

### Vérifier la présence du rapport SonarQube

```bash
# POSIX-compatible SonarQube detection
SONAR_INPUT=".claude/sonar/issues.json"
SONAR_AVAILABLE=false

if [ -f "$SONAR_INPUT" ]; then
    SONAR_AVAILABLE=true
    echo "📊 Rapport SonarQube détecté : $SONAR_INPUT"
else
    echo "ℹ️  Pas de rapport SonarQube trouvé, analyse sans SonarQube"
fi
```

### Si le rapport existe, le transformer

```bash
# POSIX-compatible SonarQube transformation
if [ "$SONAR_AVAILABLE" = "true" ]; then
    # Créer le dossier de rapport si nécessaire (utilisé plus tard aussi)
    DATE=$(date +%Y-%m-%d)
    REPORT_DIR=".claude/reports/${DATE}-${HEAD_SHORT}"
    mkdir -p "$REPORT_DIR"

    # Préparer la liste des fichiers du diff (séparés par des virgules)
    FILES_LIST=$(echo "$FILES_CHANGED" | tr '\n' ',' | sed 's/,$//')

    # Calculer la date du commit de départ pour filtrer SonarQube
    # Cette date est celle du checkpoint (ou merge-base si premier run)
    # Format ISO 8601 : 2025-12-10T14:32:15+01:00
    if [ "$MODE" = "full" ]; then
        # Mode --all : garder toutes les issues (pas de filtrage temporel)
        SINCE_ARG="--since none"
        echo "Mode --all : pas de filtrage temporel SonarQube"
    else
        # Mode incrémental : filtrer les issues depuis la date du commit de départ
        CHECKPOINT_DATE=$(git show -s --format=%cI "$LAST_COMMIT" 2>/dev/null)
        if [ -n "$CHECKPOINT_DATE" ]; then
            SINCE_ARG="--since $CHECKPOINT_DATE"
            echo "Filtrage SonarQube depuis : $CHECKPOINT_DATE"
        else
            # Fallback si pas de date disponible
            SINCE_ARG="--since 48h"
            echo "Fallback : filtrage SonarQube sur 48h"
        fi
    fi

    # Générer le rapport Markdown ET JSON filtré sur les fichiers du diff
    # Le script génère automatiquement :
    # - sonar.md (pour SYNTHESIS)
    # - sonar-issues.json (pour web-synthesizer)
    python .claude/scripts/transform-sonar.py "$SONAR_INPUT" \
        --files "$FILES_LIST" \
        $SINCE_ARG \
        --commit "$HEAD_SHORT" \
        --branch "$CURRENT_BRANCH" \
        --output "$REPORT_DIR/sonar.md"

    if [ $? -eq 0 ]; then
        SONAR_REPORT="$REPORT_DIR/sonar.md"
        SONAR_ISSUES_JSON="$REPORT_DIR/sonar-issues.json"
        echo "✅ Rapport SonarQube généré : $SONAR_REPORT"
        echo "✅ Issues SonarQube JSON : $SONAR_ISSUES_JSON"
    else
        echo "⚠️  Erreur lors de la transformation SonarQube, analyse sans SonarQube"
        SONAR_AVAILABLE=false
        SONAR_ISSUES_JSON=""
    fi
fi
```

**Comportement** :
- Si `.claude/sonar/issues.json` existe → transformer et filtrer sur les fichiers du diff
- Si le fichier n'existe pas → continuer sans SonarQube (message informatif)

**Filtrage temporel dynamique** :
- Mode incrémental : filtre les issues depuis la date du commit checkpoint
- Mode --all : pas de filtrage temporel (garde toutes les issues)
- La date est au format ISO 8601 (ex: `2025-12-10T14:32:15+01:00`)
- Si la transformation échoue → continuer sans SonarQube (avertissement)
- Le rapport `sonar.md` sera passé à SYNTHESIS comme input optionnel

---

## ÉTAPE 4 : Afficher le résumé avant analyse

```
╔═══════════════════════════════════════════════════════════════╗
║  ANALYSE INCRÉMENTALE                                         ║
╠═══════════════════════════════════════════════════════════════╣
║  Branche      : {CURRENT_BRANCH}                              ║
║  Checkpoint   : {LAST_COMMIT_SHORT} ({LAST_DATE})             ║
║  HEAD         : {HEAD_SHORT}                                  ║
║  Fichiers     : {FILES_COUNT} fichiers à analyser             ║
║  SonarQube    : {Disponible/Non disponible}                   ║
╠═══════════════════════════════════════════════════════════════╣
║  Fichiers modifiés :                                          ║
║  - src/server/UDPServer.cpp (modifié)                         ║
║  - src/server/UDPClient.cpp (ajouté)                          ║
║  - src/old/Legacy.cpp (supprimé - ignoré)                     ║
╚═══════════════════════════════════════════════════════════════╝
```

## ÉTAPE 5 : Récupérer le contexte Jira (optionnel)

Si le MCP Jira est configuré, extraire les informations du ticket associé au commit.

Utilise l'outil MCP `mcp__jira__get_issue_from_text` avec le message du commit pour extraire automatiquement le ticket Jira.

**Important** : L'absence de contexte Jira ne doit JAMAIS bloquer l'analyse.

## ÉTAPE 6 : Préparer le contexte pour les agents

Pour chaque fichier modifié, récupérer :

```bash
# Diff unifié (version finale)
git diff $LAST_COMMIT..HEAD -- "path/to/file.cpp"

# Stats
git diff $LAST_COMMIT..HEAD --stat -- "path/to/file.cpp"
```

## ÉTAPE 7 : Lancer les agents

### Ordre d'exécution OBLIGATOIRE (4 phases)

```
┌─────────────────────────────────────────────────────────────────┐
│                     PHASE 1 : PARALLÈLE                         │
│                                                                  │
│   ┌──────────┐   ┌──────────┐   ┌──────────┐                    │
│   │ ANALYZER │   │ SECURITY │   │ REVIEWER │                    │
│   └────┬─────┘   └────┬─────┘   └────┬─────┘                    │
│        │              │              │                           │
│        └──────────────┼──────────────┘                           │
│                       ▼                                          │
├─────────────────────────────────────────────────────────────────┤
│                     PHASE 2 : RISK puis PARALLÈLE               │
│                       ▼                                          │
│                 ┌──────────┐                                     │
│                 │   RISK   │  ← Reçoit les 3 rapports           │
│                 └────┬─────┘                                     │
│                      ▼                                           │
│        ┌─────────────┴─────────────┐                             │
│        │                           │                             │
│   ┌────┴─────┐               ┌─────┴────┐                        │
│   │SYNTHESIS │               │  SONAR   │ (si SonarQube dispo)   │
│   └────┬─────┘               └────┬─────┘                        │
│        │                          │                              │
│        └──────────┬───────────────┘                              │
│                   ▼                                              │
├─────────────────────────────────────────────────────────────────┤
│                     PHASE 3 : FUSION                            │
│                       ▼                                          │
│            ┌───────────────────┐                                 │
│            │  META-SYNTHESIS   │ ← Fusionne SYNTHESIS + SONAR   │
│            │  - Dédoublonne    │                                 │
│            │  - Complète       │                                 │
│            └────────┬──────────┘                                 │
│                     ▼                                            │
├─────────────────────────────────────────────────────────────────┤
│                     PHASE 4 : WEB EXPORT                        │
│                     ▼                                            │
│           ┌─────────────────┐                                    │
│           │ WEB SYNTHESIZER │  ← Transforme pour le site web    │
│           └─────────────────┘                                    │
│                     │                                            │
│                     ▼                                            │
│           reports/web-report-{date}-{commit}.json                │
└─────────────────────────────────────────────────────────────────┘
```

### PHASE 1 : Lancer ANALYZER, SECURITY, REVIEWER EN PARALLÈLE

**CRITIQUE** : Tu DOIS lancer ces 3 agents **dans un seul message** avec **3 appels Task tool simultanés**.

Chaque agent DOIT utiliser AgentDB. Vérifie dans chaque rapport la présence de la section "AgentDB Data Used".

#### Agents Phase 1 (parallèles)

| Agent | subagent_type | Query AgentDB obligatoires |
|-------|---------------|---------------------------|
| ANALYZER | `analyzer` | file_context, symbol_callers, file_impact |
| SECURITY | `security` | error_history, patterns (category=security) |
| REVIEWER | `reviewer` | patterns, file_metrics, architecture_decisions |

### PHASE 2 : Lancer RISK puis SYNTHESIS et SONAR (parallèle)

**Attendre** que les 3 agents de Phase 1 soient terminés.

1. **D'abord RISK** : Lancer l'agent RISK avec les résultats des 3 agents Phase 1
2. **Puis en parallèle** :
   - **SYNTHESIS** : Agrège les rapports des 4 agents (ANALYZER, SECURITY, REVIEWER, RISK)
   - **SONAR** (si SonarQube disponible) : Enrichit les issues SonarQube avec AgentDB

| Agent | subagent_type | Input | Condition |
|-------|---------------|-------|-----------|
| RISK | `risk` | Rapports ANALYZER, SECURITY, REVIEWER | Toujours |
| SYNTHESIS | `synthesis` | Rapports des 4 agents | Toujours |
| SONAR | `sonar` | sonar-issues.json + contexte | Si SonarQube disponible |

**CRITIQUE** : SYNTHESIS et SONAR sont lancés **en parallèle** dans un seul message avec **2 appels Task tool simultanés**.

**Gestion du cas sans SonarQube** :
- Si `.claude/sonar/issues.json` n'existe pas → Ne pas lancer SONAR, lancer seulement SYNTHESIS

### PHASE 3 : Lancer META-SYNTHESIS (après Phase 2)

**Attendre** que SYNTHESIS et SONAR (si lancé) soient terminés.

| Agent | subagent_type | Input |
|-------|---------------|-------|
| META-SYNTHESIS | `meta-synthesis` | Rapports SYNTHESIS + SONAR (si disponible) |

**L'agent META-SYNTHESIS** :
1. Lit le rapport SYNTHESIS (REPORT.md) avec tous les findings des agents
2. Lit le rapport SONAR enrichi (sonar-enriched.json) si disponible
3. Fusionne toutes les issues dans une liste unique
4. Détecte et fusionne les doublons (même fichier + ligne ±5 + même catégorie)
5. Génère les where/why/how pour les issues agents qui n'en ont pas
6. Vérifie que CHAQUE issue a where/why/how NON VIDES
7. Produit meta-synthesis.json pour WEB-SYNTHESIZER

### PHASE 4 : Lancer WEB SYNTHESIZER (après Phase 3)

**Attendre** que META-SYNTHESIS soit terminé.

| Agent | subagent_type | Input |
|-------|---------------|-------|
| WEB SYNTHESIZER | `web-synthesizer` | Rapport META-SYNTHESIS |

**L'agent WEB SYNTHESIZER** :
1. Lit le rapport META-SYNTHESIS (meta-synthesis.json)
2. Transforme en format JSON pour le site web
3. Crée `issues[]` et `issueDetails{}`
4. Vérifie que `issues.length === Object.keys(issueDetails).length`
5. Produit un fichier JSON dans `reports/web-report-{date}-{commit}.json`

**IMPORTANT** : WEB-SYNTHESIZER ne fait PLUS de dédoublonnage ni de fusion. Il reçoit des données déjà propres de META-SYNTHESIS.

## ÉTAPE 8 : Créer le dossier de rapport

```bash
# Note: Le dossier peut déjà exister si SonarQube était disponible (étape 3b)
DATE=`date +%Y-%m-%d`
COMMIT_SHORT=`git rev-parse --short HEAD`
REPORT_DIR=".claude/reports/${DATE}-${COMMIT_SHORT}"
mkdir -p "$REPORT_DIR"
```

## ÉTAPE 9 : Sauvegarder les rapports

```
.claude/reports/{date}-{commit}/
├── analyzer.md              # Phase 1 - Agent ANALYZER
├── security.md              # Phase 1 - Agent SECURITY
├── reviewer.md              # Phase 1 - Agent REVIEWER
├── risk.md                  # Phase 2 - Agent RISK
├── REPORT.md                # Phase 2 - Agent SYNTHESIS (rapport principal)
├── sonar.md                 # Phase 2 - Script transform-sonar.py (markdown)
├── sonar-issues.json        # Phase 2 - Script transform-sonar.py (JSON)
├── sonar-enriched.md        # Phase 2 - Agent SONAR (rapport lisible, optionnel)
├── sonar-enriched.json      # Phase 2 - Agent SONAR (JSON pour META-SYNTHESIS, optionnel)
├── meta-synthesis.json      # Phase 3 - Agent META-SYNTHESIS
└── meta-synthesis-report.md # Phase 3 - Agent META-SYNTHESIS (rapport lisible)
```

## ÉTAPE 9b : Valider le rapport web

**Après la génération du rapport web par WEB-SYNTHESIZER, valider le format JSON.**

```bash
# Valider le rapport web
DATE=$(date +%Y-%m-%d)
COMMIT_SHORT=$(git rev-parse --short HEAD)
WEB_REPORT="reports/web-report-${DATE}-${COMMIT_SHORT}.json"

echo "Validation du rapport web..."
python .claude/scripts/validate-web-report.py "$WEB_REPORT"

if [ $? -ne 0 ]; then
    echo "❌ ERREUR: Le rapport web ne respecte pas le format attendu"
    echo "Voir les erreurs ci-dessus et corriger"
    exit 1
fi

echo "✅ Rapport web validé avec succès"
```

**Règles de validation** :
- Structure JSON correcte (metadata, issues, issueDetails)
- `issues.length === Object.keys(issueDetails).length`
- Chaque `where` contient un snippet de code (```)
- Chaque `why` contient un diagramme Mermaid (```mermaid)
- `source` est toujours un tableau
- Toutes les valeurs sont dans les listes autorisées (verdict, severity, category)
- `isBug=true` uniquement pour les crashs

**Si la validation échoue** :
1. Identifier les erreurs dans le rapport
2. Corriger les issues problématiques
3. Re-générer le rapport web
4. Re-valider

## ÉTAPE 10 : Mettre à jour le checkpoint et enregistrer l'analyse

**APRÈS** avoir généré le rapport final et obtenu le verdict :

```bash
# Mettre à jour le checkpoint avec le résultat
bash .claude/agentdb/query.sh set_checkpoint \
    "$CURRENT_BRANCH" \
    "$HEAD_COMMIT" \
    "$FILES_COUNT" \
    "$VERDICT" \
    "$SCORE"

# Enregistrer l'analyse dans l'historique des pipeline_runs
bash .claude/agentdb/query.sh record_pipeline_run \
    "$CURRENT_BRANCH" \
    "$HEAD_COMMIT" \
    "$SCORE" \
    "$VERDICT" \
    "$FILES_COUNT"
```

**Note** : L'enregistrement du pipeline_run permet de suivre l'évolution des scores dans le temps et de détecter les tendances. Ces données sont consultables via `bash .claude/agentdb/query.sh list_pipeline_runs`.

## ÉTAPE 11 : Afficher le verdict final

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║     VERDICT: {emoji} {VERDICT}                                ║
║                                                               ║
║     Score global: {score}/100                                 ║
║                                                               ║
║     {résumé en 2-3 lignes}                                    ║
║                                                               ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║     Checkpoint mis à jour : {HEAD_SHORT}                      ║
║     Prochaine /analyze partira de ce point.                   ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

Rapport complet : .claude/reports/{date}-{commit}/REPORT.md
```

---

## Verdicts possibles

| Score | Verdict | Emoji | Signification |
|-------|---------|-------|---------------|
| ≥80 | APPROVE | 🟢 | Peut être mergé |
| ≥60 | REVIEW | 🟡 | Review humaine recommandée |
| ≥40 | CAREFUL | 🟠 | Review approfondie requise |
| <40 | REJECT | 🔴 | Ne pas merger en l'état |

### Règles de décision

```
SI (Security.max_severity == "CRITICAL") OU (Security.regressions > 0) :
    → REJECT (🔴)

SI (Security.max_severity == "HIGH") OU (Risk.score < 60) OU (issues bloquantes) :
    → CAREFUL (🟠)

SI (Reviewer.errors > 0) OU (Risk.score < 80) :
    → REVIEW (🟡)

SINON :
    → APPROVE (🟢)
```

---

## Prompts pour les agents

### Prompt ANALYZER

```
Analyse l'impact des modifications suivantes :

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Branche** : {CURRENT_BRANCH}

**Fichiers modifiés** :
{liste des fichiers avec leurs stats}

**Diff résumé** :
{diff --stat}

INSTRUCTIONS :
1. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="analyzer" bash .claude/agentdb/query.sh file_context "path"`
2. Identifie les fonctions modifiées dans chaque fichier
3. Pour CHAQUE fonction modifiée, appelle `AGENTDB_CALLER="analyzer" bash .claude/agentdb/query.sh symbol_callers "funcName"`
4. Calcule l'impact : LOCAL / MODULE / GLOBAL
5. Produis le rapport avec la section "AgentDB Data Used"

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/analyzer.md
```

### Prompt SECURITY

```
Audite la sécurité des modifications suivantes :

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Fichiers modifiés** :
{liste des fichiers}

INSTRUCTIONS :
1. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="security" bash .claude/agentdb/query.sh error_history "path"`
2. Vérifie s'il y a des patterns de bugs passés qui réapparaissent (RÉGRESSION)
3. Appelle `AGENTDB_CALLER="security" bash .claude/agentdb/query.sh patterns "" "security"` pour les patterns de sécurité
4. Scanne le code pour les vulnérabilités connues (CWE)
5. Produis le rapport avec la section "AgentDB Data Used"

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/security.md
```

### Prompt REVIEWER

```
Effectue une code review des modifications suivantes :

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Fichiers modifiés** :
{liste des fichiers}

INSTRUCTIONS :
1. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="reviewer" bash .claude/agentdb/query.sh patterns "path"`
2. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="reviewer" bash .claude/agentdb/query.sh file_metrics "path"`
3. Appelle `AGENTDB_CALLER="reviewer" bash .claude/agentdb/query.sh architecture_decisions` pour les ADRs
4. Vérifie les conventions, la qualité, et l'architecture
5. Produis le rapport avec la section "AgentDB Data Used"

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/reviewer.md
```

### Prompt RISK

```
Évalue le risque des modifications suivantes :

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Fichiers modifiés** :
{liste des fichiers}

**Résultats des agents précédents** :

ANALYZER :
{résumé du rapport analyzer}

SECURITY :
{résumé du rapport security}

REVIEWER :
{résumé du rapport reviewer}

INSTRUCTIONS :
1. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="risk" bash .claude/agentdb/query.sh file_context "path"` (criticité)
2. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="risk" bash .claude/agentdb/query.sh file_metrics "path"` (complexité)
3. Pour CHAQUE fichier, appelle `AGENTDB_CALLER="risk" bash .claude/agentdb/query.sh error_history "path" 90` (bugs récents)
4. Calcule le score de risque selon la formule
5. Produis le rapport avec la section "AgentDB Data Used"

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/risk.md
```

### Prompt SYNTHESIS

```
Synthétise les rapports d'analyse suivants :

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Branche** : {CURRENT_BRANCH}
**Date** : {date}

**RAPPORT ANALYZER** :
{rapport complet analyzer}

**RAPPORT SECURITY** :
{rapport complet security}

**RAPPORT REVIEWER** :
{rapport complet reviewer}

**RAPPORT RISK** :
{rapport complet risk}

INSTRUCTIONS :
1. Parse les scores et findings de chaque agent
2. Calcule le score global (Security×0.35 + Risk×0.25 + Reviewer×0.25 + Analyzer×0.15)
3. Détecte les contradictions entre agents
4. Détermine le verdict : APPROVE / REVIEW / CAREFUL / REJECT
5. Produis le rapport final avec TOUS les findings des agents

IMPORTANT - FORMAT DES FINDINGS :
Chaque finding DOIT inclure :
- id : Identifiant unique (SEC-001, ANA-001, REV-001, RISK-001)
- source : Tableau ["security"] ou ["analyzer"] etc.
- severity : Blocker | Critical | Major | Medium | Minor | Info
- category : Security | Reliability | Maintainability
- isBug : true si provoque crash/freeze, false sinon
- file : Chemin du fichier
- line : Numéro de ligne
- message : Description du problème

IMPORTANT - NOTE :
- SYNTHESIS ne fait PLUS de dédoublonnage avec SonarQube
- Le dédoublonnage sera fait par META-SYNTHESIS ensuite
- Produis un rapport avec TOUS les findings des 4 agents

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/synthesis.md
```

### Prompt SONAR

```
Enrichis les issues SonarQube avec le contexte du projet via AgentDB.

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Branche** : {CURRENT_BRANCH}
**Date** : {date}

**Dossier de rapport** : .claude/reports/{date}-{commit}/

**Fichiers du diff** :
{liste des fichiers modifiés}

**Fichier SonarQube issues** : .claude/reports/{date}-{commit}/sonar-issues.json
(Généré par transform-sonar.py avec les issues filtrées sur les fichiers du diff)

INSTRUCTIONS :
1. Lis le fichier sonar-issues.json qui contient les issues SonarQube
2. Pour CHAQUE issue, appelle AgentDB pour enrichir le contexte :
   - file_context : Comprendre le rôle du fichier
   - patterns : Trouver les patterns applicables
   - file_metrics : Obtenir les métriques
   - architecture_decisions : Vérifier les ADRs
3. Enrichis les sections where/why/how avec le contexte du projet
4. Vérifie que CHAQUE issue a where/why/how NON VIDES
5. Produis sonar-enriched.json pour META-SYNTHESIS

IMPORTANT - RÈGLE ABSOLUE :
Chaque issue DOIT avoir un where, why, how NON VIDE.
Si AgentDB ne répond pas, conserve les données basiques de transform-sonar.py.

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/sonar.md
```

### Prompt META-SYNTHESIS

```
Fusionne et dédoublonne les rapports SYNTHESIS et SONAR.

**Type d'analyse** : Diff unifié entre {LAST_COMMIT_SHORT} et {HEAD_SHORT}
**Branche** : {CURRENT_BRANCH}
**Date** : {date}

**Dossier de rapport** : .claude/reports/{date}-{commit}/

**Rapport SYNTHESIS** : .claude/reports/{date}-{commit}/REPORT.md
**Rapport SONAR** : {Si SONAR_AVAILABLE == true : ".claude/reports/{date}-{commit}/sonar-enriched.json", sinon : "Non disponible"}

INSTRUCTIONS :
1. Lis le rapport SYNTHESIS et extrait TOUS les findings
2. Si disponible, lis sonar-enriched.json avec les issues SonarQube
3. Fusionne toutes les issues dans une liste unique
4. Détecte les doublons (même fichier + ligne ±5 + même catégorie)
5. Fusionne les doublons en combinant leurs sources
6. Génère where/why/how pour les issues agents qui n'en ont pas
7. Utilise AgentDB si des données manquent
8. VÉRIFIE que CHAQUE issue a where/why/how NON VIDES
9. Produis meta-synthesis.json pour WEB-SYNTHESIZER

RÈGLE ABSOLUE :
`issues.length === nombre_issueDetails`
Chaque issue DOIT avoir where, why, how NON VIDES.

RÈGLES DE FUSION :
- ID : Garder l'ID agent (priorité sur SonarQube)
- source : Combiner les tableaux (ex: ["security", "sonarqube"])
- severity : Garder la plus haute
- isBug : true si l'un des deux est true

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/meta-synthesis.md
```

### Prompt WEB SYNTHESIZER

```
Transforme le rapport META-SYNTHESIS en format compatible avec le site web CRE Interface.

**Rapport META-SYNTHESIS** : .claude/reports/{date}-{commit}/meta-synthesis.json
**Date** : {date}
**Commit** : {HEAD_SHORT}
**Branche** : {CURRENT_BRANCH}

INSTRUCTIONS :
1. Lis le fichier meta-synthesis.json (déjà fusionné et dédoublonné par META-SYNTHESIS)
2. Transforme chaque issue en format attendu par le site web
3. Crée le tableau `issues[]` avec les bons champs
4. Crée l'objet `issueDetails{}` avec where/why/how pour CHAQUE issue
5. VÉRIFIE que `issues.length === Object.keys(issueDetails).length`
6. Sauvegarde dans reports/web-report-{date}-{commit}.json

IMPORTANT - CE QUE TU NE FAIS PLUS :
- Tu NE fais PLUS de dédoublonnage (déjà fait par META-SYNTHESIS)
- Tu NE fais PLUS de fusion des sources (déjà fait par META-SYNTHESIS)
- Tu NE génères PLUS les where/why/how (déjà générés par META-SYNTHESIS)
- Tu COPIES simplement les données de meta-synthesis.json vers le format web

RÈGLE ABSOLUE :
`issues.length === Object.keys(issueDetails).length`

Si cette règle n'est pas respectée → ERREUR, le JSON est invalide.

VÉRIFICATION FINALE :
Pour CHAQUE issue dans issues[] :
- issueDetails[issue.id] DOIT exister
- issueDetails[issue.id].where DOIT être non vide
- issueDetails[issue.id].why DOIT être non vide
- issueDetails[issue.id].how DOIT être non vide

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/web-synthesizer.md
```

---

## Gestion des erreurs

- Si un agent échoue, continuer avec les autres
- Signaler l'erreur dans le rapport final
- Ne jamais bloquer à cause d'AgentDB manquant

---

## Commandes utilitaires

### reset_checkpoint

La commande `reset_checkpoint` permet de supprimer le checkpoint d'analyse d'une branche dans AgentDB.

**Quand l'utiliser :**
- Forcer une ré-analyse complète sans utiliser `--all`
- Débugger un problème de checkpoint corrompu
- Tester le comportement "première analyse" sur une branche existante
- Nettoyer les checkpoints de branches supprimées

**Usage :**

```bash
# Supprimer le checkpoint de la branche courante
bash .claude/agentdb/query.sh reset_checkpoint "$(git branch --show-current)"

# Supprimer le checkpoint d'une branche spécifique
bash .claude/agentdb/query.sh reset_checkpoint "feature/my-branch"

# Lister tous les checkpoints existants
bash .claude/agentdb/query.sh list_checkpoints
```

**Ce que fait la commande :**
- Supprime l'entrée de la branche dans la table `analysis_checkpoints`
- La prochaine exécution de `/analyze` sur cette branche se comportera comme une première analyse
- Elle calculera le merge-base avec main/develop comme point de départ

**Différence avec `--reset` :**
- `/analyze --reset` : Met le checkpoint à HEAD **sans analyser** (prochaine analyse partira de HEAD)
- `reset_checkpoint` : Supprime le checkpoint (prochaine analyse partira du merge-base)

---

## Exécution

Maintenant, exécute l'analyse en suivant les étapes ci-dessus.

0. Mets à jour AgentDB (`python .claude/scripts/bootstrap.py --incremental`)
1. Parse les arguments ($ARGUMENTS)
2. Détermine le mode (incremental, full, reset, files, commit)
3. Si mode == reset : mets à jour le checkpoint et TERMINE
4. Calcule le diff unifié
5. Si aucun fichier : affiche "Rien à analyser" et TERMINE
6. Transforme le rapport SonarQube si disponible (transform-sonar.py)
7. Lance les agents :
   - **PHASE 1** : analyzer/security/reviewer EN PARALLÈLE
   - **PHASE 2** : risk, puis synthesis/sonar EN PARALLÈLE
   - **PHASE 3** : meta-synthesis (fusionne et dédoublonne)
   - **PHASE 4** : web-synthesizer (produit le JSON final)
8. Vérifie que CHAQUE issue a where/why/how
9. **Valide le rapport web** (`python .claude/scripts/validate-web-report.py`)
10. Mets à jour le checkpoint avec le verdict
11. Affiche le verdict final
