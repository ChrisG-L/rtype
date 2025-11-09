# Introduction à la Ruche d'Agents R-Type

## Vue d'Ensemble

La **Ruche d'Agents** est un système d'intelligence artificielle collaborative conçu pour automatiser les tâches techniques et organisationnelles autour du développement du projet R-Type. Elle est composée d'agents spécialisés coordonnés par une **Reine** orchestratrice.

## 🎯 Philosophie

**Objectif** : Les développeurs se concentrent sur le code métier, l'IA gère le reste.

- ✅ **Automatisation** : Commits, documentation, tickets, tests, sécurité
- ✅ **Qualité** : Standards professionnels constants
- ✅ **Cohérence** : Synchronisation parfaite entre code, docs et tickets
- ✅ **Rapidité** : Workflows optimisés et parallélisés

## 👑 Architecture de la Ruche

```
          👑 Reine des Abeilles (Orchestratrice)
                      |
       ┌──────────────┼──────────────┐
       |              |              |
   🐝 Tests       🐝 Sécurité    🐝 Git Commit
       |              |              |
   🐝 Docs        🐝 Linear      (+ autres)
```

### Les Agents

| Agent                    | Rôle                                  | Sortie                          |
| ------------------------ | ------------------------------------- | ------------------------------- |
| **👑 Queen Bee**         | Orchestre tous les agents             | Rapport final (MD + JSON)       |
| **🐝 Tests & Qualité**   | Exécute tests, mesure couverture      | `tests_report.json`             |
| **🐝 Sécurité & CI**     | Scan vulnérabilités, analyse statique | `security_report.json`          |
| **🐝 Git Commit**        | Crée commits atomiques propres        | `commit_plan.json`              |
| **🐝 Documentation**     | Maintient documentation MkDocs        | `docs_report.json`              |
| **🐝 Linear Manager**    | Gère tickets et organisation          | `linear_report.json`            |

## 📁 Structure du Projet

```
rtype/
├── .claude/                  # Agents IA (prompts)
│   └── agents/
│       ├── queen_bee_agent.md
│       ├── tests_quality_agent.md
│       ├── security_ci_agent.md
│       ├── git_commit_agent.md
│       ├── docs_agent.md
│       └── linear_manager_agent.md
│
├── hive/                     # Infrastructure de la ruche
│   ├── events/               # Contextes à traiter
│   ├── reports/              # Rapports JSON des agents
│   ├── policies/             # Règles et seuils de qualité
│   │   ├── quality_gates.json
│   │   ├── commit_conventions.json
│   │   └── security_thresholds.json
│   ├── state/                # État persistant de la ruche
│   │   └── hive_health.json
│   └── schemas/              # Documentation des formats
│       ├── agent_report_schema.md
│       └── commit_plan_schema.md
│
├── scripts/                  # Scripts d'orchestration
│   ├── hive_run.sh           # Orchestrateur principal
│   ├── run_tests.sh          # Lance les tests
│   ├── install_git_hooks.sh  # Installe les hooks Git
│   └── git-hooks/            # Hooks Git sources
│       ├── commit-msg
│       └── pre-push
│
├── settings.local.json       # Configuration globale
└── docs/                     # Documentation MkDocs
    └── hive/                 # Documentation de la ruche
```

## 🔄 Workflows (Flows)

### Flow "Feature" (Complet)

Quand tu termines une fonctionnalité :

```bash
./scripts/hive_run.sh --flow=feature
```

**Agents exécutés** :

1. 🔒 Sécurité (scan rapide)
2. 🧪 Tests (tous + couverture)
3. 📝 Git Commit (plan atomique)
4. 📚 Documentation (vérification)
5. 📋 Linear (tickets)

**Résultat** : Rapport complet OK/WARN/FAIL

### Flow "Bugfix" (Urgent)

Pour un bugfix rapide :

```bash
./scripts/hive_run.sh --flow=bugfix
```

**Agents exécutés** :

1. 🧪 Tests (prioritaires)
2. 📝 Git Commit
3. 🔒 Sécurité (si code sensible)

### Flow "Audit" (Hebdomadaire)

Rapport de santé global :

```bash
./scripts/hive_run.sh --flow=audit
```

**Sortie** : Métriques, tendances, recommandations

### Flow "Pre-Push" (Hook)

Automatique avant chaque `git push` :

```bash
# Appelé automatiquement par le hook
./scripts/hive_run.sh --flow=pre-push
```

**Mode** : Rapide, warnings seulement

## 🚦 Quality Gates

Chaque agent vérifie des **seuils de qualité** (gates) :

| Gate                  | Seuil | Mode     | Description                |
| --------------------- | ----- | -------- | -------------------------- |
| `min_coverage`        | 70%   | blocking | Couverture minimale        |
| `max_failed_tests`    | 0     | blocking | Aucun test échoué          |
| `max_critical_vulns`  | 0     | blocking | Aucune vulnérabilité crit. |
| `max_high_vulns`      | 0     | blocking | Aucune vulnérabilité haute |
| `max_medium_vulns`    | 3     | warning  | Max 3 vulnérabilités moy.  |

**Mode blocking** : Échec → merge bloqué
**Mode warning** : Échec → avertissement seulement

## 💻 Utilisation avec Claude Code

### Avec la Reine

```
"Reine, j'ai terminé la feature système de particules, assure-toi que tout est en ordre"
```

La Reine va :

1. Orchestrer tous les agents
2. Lire leurs rapports JSON
3. Vérifier les quality gates
4. Te donner un rapport final

### Avec un Agent Spécifique

```
"Agent Tests, lance tous les tests et donne-moi un rapport"

"Agent Git, analyse mes changements et propose un plan de commits"

"Agent Docs, vérifie que toute la doc est à jour"
```

## 📊 Rapports

Tous les rapports sont dans `hive/reports/` :

- `tests_report.json` - Résultats des tests
- `security_report.json` - Scan de sécurité
- `commit_plan.json` - Plan de commits proposé
- `docs_report.json` - État de la documentation
- `linear_report.json` - Tickets créés/mis à jour
- **`queen_final_report.md`** - Rapport final humain
- **`queen_final_report.json`** - Rapport final machine

## 🔧 Installation

### 1. Installer les Hooks Git

```bash
./scripts/install_git_hooks.sh
```

Cela installe :

- `commit-msg` : Valide le format des commits
- `pre-push` : Lance vérifications rapides

### 2. Configurer les Policies

Éditer `hive/policies/*.json` selon tes besoins :

- `quality_gates.json` : Seuils de qualité
- `commit_conventions.json` : Format des commits
- `security_thresholds.json` : Seuils de sécurité

### 3. Configuration Globale

Éditer `settings.local.json` :

```json
{
  "hive": {
    "default_flow": "feature"
  },
  "quality_gates": {
    "mode": "strict"
  }
}
```

## 🎓 Pour Aller Plus Loin

- [Agents](agents.md) - Description détaillée de chaque agent
- [Workflows](workflows.md) - Guide des différents flows
- [Configuration](configuration.md) - Options de configuration
- [Rapports](rapports.md) - Format des rapports JSON
- [Guide Contributing](../guides/contributing_ruche.md) - Workflow de développement

## 🆘 Aide et Support

- Lire la doc complète : `mkdocs serve` puis http://localhost:8000
- Issues GitHub : (lien vers le repo)
- Slack/Discord : (si applicable)

---

**🐝 Bienvenue dans la Ruche R-Type ! L'IA travaille pour toi, tu te concentres sur le code.**
