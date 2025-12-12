---
name: analyze
description: Lance une analyse incrémentale intelligente du code avec les 5 agents (analyzer, security, reviewer, risk, synthesis).
  Se souvient du dernier commit analysé et n'analyse que les changements depuis.

  Usage:
  - /analyze              : Analyse incrémentale depuis le dernier checkpoint
  - /analyze --all        : Analyse complète depuis le merge-base (ignore le checkpoint)
  - /analyze --reset      : Met le checkpoint à HEAD sans analyser
  - /analyze abc123       : Analyse un commit spécifique
  - /analyze --files src/file.cpp : Analyse des fichiers spécifiques
---

# Commande /analyze - Analyse Incrémentale

Tu dois orchestrer une analyse incrémentale intelligente du code en utilisant les 5 agents spécialisés et AgentDB.

## ARGUMENT REÇU

$ARGUMENTS

---

## ÉTAPE 0 : Mettre à jour AgentDB (incrémental)

**Avant toute analyse, s'assurer que la base AgentDB est à jour avec les derniers fichiers.**

```bash
# Mettre à jour AgentDB de manière incrémentale
# Cela ne réindexe que les fichiers modifiés depuis le dernier indexage
python .claude/scripts/bootstrap.py --incremental 2>/dev/null || true
```

**Comportement** :
- Si la base n'existe pas : affiche un avertissement mais continue (l'analyse fonctionnera sans AgentDB)
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

# Extraire les fichiers de la liste d'arguments
FILES_TO_ANALYZE=""
PARSING_FILES=false
for arg in $ARGUMENTS; do
    if [[ "$arg" == "--files" ]]; then
        PARSING_FILES=true
        continue
    fi
    if [[ "$PARSING_FILES" == true ]]; then
        # Vérifier que le fichier existe
        if [[ -f "$arg" ]]; then
            FILES_TO_ANALYZE="$FILES_TO_ANALYZE $arg"
        else
            echo "⚠️  Fichier non trouvé: $arg"
        fi
    fi
done

# Valider qu'au moins un fichier est spécifié
if [[ -z "$FILES_TO_ANALYZE" ]]; then
    echo '{"error": "Aucun fichier valide spécifié. Usage: /analyze --files <file1> [file2] ..."}'
    # TERMINER
fi

FILES_COUNT=`echo "$FILES_TO_ANALYZE" | wc -w`
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

# Vérifier si c'est une plage (contient ..)
if [[ "$COMMIT_ARG" == *".."* ]]; then
    # Plage de commits: abc123..def456
    START_COMMIT="${COMMIT_ARG%%..*}"
    END_COMMIT="${COMMIT_ARG##*..}"
else
    # Commit unique: analyser depuis son parent
    START_COMMIT=`git rev-parse "$COMMIT_ARG^" 2>/dev/null`
    END_COMMIT="$COMMIT_ARG"
fi

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

## ÉTAPE 4 : Afficher le résumé avant analyse

```
╔═══════════════════════════════════════════════════════════════╗
║  ANALYSE INCRÉMENTALE                                         ║
╠═══════════════════════════════════════════════════════════════╣
║  Branche      : {CURRENT_BRANCH}                              ║
║  Checkpoint   : {LAST_COMMIT_SHORT} ({LAST_DATE})             ║
║  HEAD         : {HEAD_SHORT}                                  ║
║  Fichiers     : {FILES_COUNT} fichiers à analyser             ║
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

### Ordre d'exécution OBLIGATOIRE

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
│                     PHASE 2 : SÉQUENTIEL                        │
│                       ▼                                          │
│                 ┌──────────┐                                     │
│                 │   RISK   │  ← Reçoit les 3 rapports           │
│                 └────┬─────┘                                     │
│                      ▼                                          │
│                ┌───────────┐                                     │
│                │ SYNTHESIS │  ← Reçoit les 4 rapports           │
│                └────┬──────┘                                     │
│                     ▼                                            │
├─────────────────────────────────────────────────────────────────┤
│                     PHASE 3 : WEB EXPORT                        │
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

### PHASE 2 : Lancer RISK puis SYNTHESIS (séquentiel)

**Attendre** que les 3 agents de Phase 1 soient terminés.

### PHASE 3 : Lancer WEB SYNTHESIZER (après SYNTHESIS)

**Attendre** que SYNTHESIS soit terminé.

| Agent | subagent_type | Input |
|-------|---------------|-------|
| WEB SYNTHESIZER | `web-synthesizer` | Rapport SYNTHESIS complet |

**L'agent WEB SYNTHESIZER** :
1. Lit le rapport SYNTHESIS (REPORT.md)
2. Extrait toutes les issues avec leurs métadonnées (severity, category, isBug)
3. Génère les détails where/why/how pour chaque issue
4. Produit un fichier JSON dans `reports/web-report-{date}-{commit}.json`

## ÉTAPE 8 : Créer le dossier de rapport

```bash
DATE=`date +%Y-%m-%d`
COMMIT_SHORT=`git rev-parse --short HEAD`
REPORT_DIR=".claude/reports/${DATE}-${COMMIT_SHORT}"
mkdir -p "$REPORT_DIR"
```

## ÉTAPE 9 : Sauvegarder les rapports

```
.claude/reports/{date}-{commit}/
├── analyzer.md
├── security.md
├── reviewer.md
├── risk.md
└── REPORT.md
```

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
5. Produis le rapport final

IMPORTANT - FORMAT DES FINDINGS :
Chaque finding DOIT inclure :
- severity : Blocker | Critical | Major | Medium | Minor | Info
- category : Security | Reliability | Maintainability
- isBug : true si provoque crash/freeze, false sinon

FORMAT DE SORTIE OBLIGATOIRE : Utilise le format défini dans .claude/agents/synthesis.md
```

### Prompt WEB SYNTHESIZER

```
Transforme le rapport SYNTHESIS en format compatible avec le site web CRE Interface.

**Rapport SYNTHESIS** : .claude/reports/{date}-{commit}/REPORT.md
**Date** : {date}
**Commit** : {HEAD_SHORT}
**Branche** : {CURRENT_BRANCH}

INSTRUCTIONS :
1. Lis le rapport SYNTHESIS complet
2. Extrait le bloc JSON contenant les findings
3. Pour chaque finding, génère les détails (where, why, how) en markdown avec mermaid
4. Assemble le rapport web au format JSON
5. Sauvegarde dans reports/web-report-{date}-{commit}.json

RÈGLES isBug :
- isBug = true UNIQUEMENT si l'issue provoque un crash/freeze/gel
- Buffer overflow, null pointer, division par zéro → isBug = true
- Vulnérabilités de sécurité sans crash → isBug = false
- Problèmes de qualité/maintenabilité → isBug = false

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
6. Lance les agents (PHASE 1: analyzer/security/reviewer, PHASE 2: risk/synthesis, PHASE 3: web-synthesizer)
7. Produis le rapport SYNTHESIS
8. Génère le rapport web (web-synthesizer)
9. Mets à jour le checkpoint avec le verdict
10. Affiche le verdict final
