# 🧠 AGENTDB - La Mémoire Contextuelle du Système Multi-Agents

---

# PRÉAMBULE : CE QUE TU VAS CONSTRUIRE

AgentDB n'est pas une simple base de données.

C'est le **cerveau partagé** de tous les agents Claude. C'est ce qui transforme une analyse de code basique en une analyse **contextuelle et intelligente**.

Sans AgentDB, un agent voit :
```
"strcpy(buffer, input);"
→ "Utilisation de strcpy, potentiellement dangereux"
```

Avec AgentDB, un agent voit :
```
"strcpy(buffer, input);"
→ "Utilisation de strcpy dans lcd_write.c
   CONTEXTE :
   • Ce fichier a eu un buffer overflow en mars 2024 (corrigé commit abc123)
   • Cette fonction est appelée par 12 autres fonctions
   • 3 de ces appelants sont dans des chemins critiques
   • Le pattern du module exige l'utilisation de strncpy
   
   CONCLUSION : Régression probable d'un bug déjà corrigé.
                Sévérité : CRITIQUE
                Confiance : TRÈS ÉLEVÉE"
```

**C'est cette différence que tu vas implémenter.**

---

# TABLE DES MATIÈRES

1. [Architecture Conceptuelle](#partie-1--architecture-conceptuelle)
2. [Le Graphe de Dépendances](#partie-2--le-graphe-de-dépendances)
3. [La Mémoire Historique](#partie-3--la-mémoire-historique)
4. [La Base de Connaissances](#partie-4--la-base-de-connaissances)
5. [Le Schéma SQL Complet](#partie-5--le-schéma-sql-complet)
6. [Les Requêtes de Traversée](#partie-6--les-requêtes-de-traversée)
7. [Le Serveur MCP](#partie-7--le-serveur-mcp)
8. [Le Bootstrap](#partie-8--le-bootstrap)
9. [La Maintenance](#partie-9--la-maintenance)
10. [Instructions d'Implémentation](#partie-10--instructions-dimplémentation)

---

# PARTIE 1 : ARCHITECTURE CONCEPTUELLE

## 1.1 Les Quatre Piliers d'AgentDB

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              AGENTDB                                        │
│                     "La Mémoire des Agents"                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────┐              ┌─────────────────────┐               │
│  │                     │              │                     │               │
│  │   PILIER 1          │              │   PILIER 2          │               │
│  │   LE GRAPHE         │              │   LA MÉMOIRE        │               │
│  │                     │              │                     │               │
│  │   Structure du      │              │   Historique des    │               │
│  │   code :            │              │   événements :      │               │
│  │   • Fichiers        │              │   • Erreurs/bugs    │               │
│  │   • Symboles        │              │   • Corrections     │               │
│  │   • Relations       │              │   • Runs pipeline   │               │
│  │                     │              │                     │               │
│  │   Répond à :        │              │   Répond à :        │               │
│  │   "Qui appelle      │              │   "Qu'est-ce qui    │               │
│  │   qui ?"            │              │   s'est passé       │               │
│  │   "Quel impact ?"   │              │   avant ?"          │               │
│  │                     │              │                     │               │
│  └─────────────────────┘              └─────────────────────┘               │
│                                                                             │
│  ┌─────────────────────┐              ┌─────────────────────┐               │
│  │                     │              │                     │               │
│  │   PILIER 3          │              │   PILIER 4          │               │
│  │   LA CONNAISSANCE   │              │   LES MÉTRIQUES     │               │
│  │                     │              │                     │               │
│  │   Savoir accumulé : │              │   Mesures :         │               │
│  │   • Patterns        │              │   • Complexité      │               │
│  │   • Conventions     │              │   • Lignes de code  │               │
│  │   • Décisions       │              │   • Couverture      │               │
│  │     architecturales │              │   • Activité        │               │
│  │                     │              │                     │               │
│  │   Répond à :        │              │   Répond à :        │               │
│  │   "Comment ça       │              │   "C'est gros ?     │               │
│  │   doit être fait ?" │              │   C'est complexe ?" │               │
│  │                     │              │                     │               │
│  └─────────────────────┘              └─────────────────────┘               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 1.2 Comment les Agents Utilisent Chaque Pilier

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    UTILISATION PAR LES AGENTS                                │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  AGENT ANALYZER                                                              │
│  ─────────────                                                               │
│  Utilise principalement : GRAPHE                                             │
│  • Traverse les relations pour calculer l'impact                             │
│  • Identifie les chemins critiques                                           │
│  • Compte les appelants/appelés                                              │
│                                                                              │
│  Exemple de requête :                                                        │
│  "Donne-moi tous les appelants de lcd_init() jusqu'à 3 niveaux"              │
│                                                                              │
│  ────────────────────────────────────────────────────────────────────────    │
│                                                                              │
│  AGENT SECURITY                                                              │
│  ──────────────                                                              │
│  Utilise principalement : MÉMOIRE                                            │
│  • Consulte l'historique des vulnérabilités                                  │
│  • Détecte les régressions                                                   │
│  • Corrèle avec des bugs passés similaires                                   │
│                                                                              │
│  Exemple de requête :                                                        │
│  "Y a-t-il eu des buffer overflows dans ce fichier ou ce module ?"           │
│                                                                              │
│  ────────────────────────────────────────────────────────────────────────    │
│                                                                              │
│  AGENT REVIEWER                                                              │
│  ──────────────                                                              │
│  Utilise principalement : CONNAISSANCE                                       │
│  • Vérifie le respect des patterns                                           │
│  • Compare aux conventions établies                                          │
│  • Consulte les décisions architecturales                                    │
│                                                                              │
│  Exemple de requête :                                                        │
│  "Quels sont les patterns du module 'lcd' ?"                                 │
│                                                                              │
│  ────────────────────────────────────────────────────────────────────────    │
│                                                                              │
│  AGENT RISK                                                                  │
│  ──────────────                                                              │
│  Utilise principalement : MÉTRIQUES + MÉMOIRE                                │
│  • Évalue la criticité des fichiers                                          │
│  • Calcule les deltas de complexité                                          │
│  • Consulte l'historique des problèmes                                       │
│                                                                              │
│  Exemple de requête :                                                        │
│  "Ce fichier est-il critique ? Combien de bugs a-t-il eu ?"                  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 1.3 Le Flux de Données

```
                    ┌─────────────────────────────────────┐
                    │           CODE SOURCE               │
                    │                                     │
                    │  src/                               │
                    │  ├── lcd/                           │
                    │  ├── security/                      │
                    │  └── ...                            │
                    └──────────────┬──────────────────────┘
                                   │
                    ┌──────────────▼──────────────────────┐
                    │           INDEXEUR                  │
                    │                                     │
                    │  • Parse les fichiers               │
                    │  • Extrait les symboles             │
                    │  • Détecte les relations            │
                    │  • Calcule les métriques            │
                    └──────────────┬──────────────────────┘
                                   │
                    ┌──────────────▼──────────────────────┐
                    │           AGENTDB                   │
                    │                                     │
                    │  SQLite : .claude/agentdb/db.sqlite │
                    │                                     │
                    │  ┌────────────────────────────────┐ │
                    │  │ files | symbols | relations   │ │
                    │  │ errors | patterns | decisions │ │
                    │  │ metrics | runs                │ │
                    │  └────────────────────────────────┘ │
                    └──────────────┬──────────────────────┘
                                   │
                    ┌──────────────▼──────────────────────┐
                    │         SERVEUR MCP                 │
                    │                                     │
                    │  Expose les outils :                │
                    │  • get_file_context()               │
                    │  • get_symbol_callers()             │
                    │  • get_error_history()              │
                    │  • get_patterns()                   │
                    │  • ...                              │
                    └──────────────┬──────────────────────┘
                                   │
         ┌─────────────────────────┼─────────────────────────┐
         │              │          │          │              │
         ▼              ▼          ▼          ▼              ▼
    ┌─────────┐   ┌─────────┐ ┌─────────┐ ┌─────────┐  ┌─────────┐
    │ AGENT   │   │ AGENT   │ │ AGENT   │ │ AGENT   │  │ AGENT   │
    │ANALYZER │   │SECURITY │ │REVIEWER │ │  RISK   │  │SYNTHESIS│
    └─────────┘   └─────────┘ └─────────┘ └─────────┘  └─────────┘
```

---

# PARTIE 2 : LE GRAPHE DE DÉPENDANCES

## 2.1 Concept

Le graphe de dépendances répond à LA question fondamentale de l'analyse d'impact :

> **"Si je modifie X, qu'est-ce qui peut casser ?"**

C'est un graphe orienté où :
- Les **nœuds** sont les symboles (fonctions, types, variables, macros)
- Les **arêtes** sont les relations (appelle, inclut, utilise, modifie)

## 2.2 Les Nœuds : Fichiers et Symboles

### Table `files` - Les fichiers du projet

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              TABLE : files                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY    Identifiant unique                  │
│  path            TEXT UNIQUE NOT NULL   Chemin relatif depuis la racine     │
│                                         Ex: "src/lcd/lcd_init.c"            │
│  filename        TEXT NOT NULL          Nom du fichier seul                 │
│                                         Ex: "lcd_init.c"                    │
│  extension       TEXT                   Extension du fichier                │
│                                         Ex: ".c", ".h", ".py"               │
│                                                                             │
│  CLASSIFICATION                                                             │
│  ──────────────                                                             │
│  module          TEXT                   Module logique (déduit du path)     │
│                                         Ex: "lcd", "security", "core"       │
│  layer           TEXT                   Couche architecturale               │
│                                         Ex: "driver", "service", "api"      │
│  file_type       TEXT NOT NULL          Type de fichier                     │
│                                         "source", "header", "test",         │
│                                         "config", "doc"                     │
│  language        TEXT                   Langage de programmation            │
│                                         "c", "cpp", "python", "js"          │
│                                                                             │
│  CRITICITÉ                                                                  │
│  ─────────                                                                  │
│  is_critical     BOOLEAN DEFAULT 0      Fichier marqué comme critique       │
│  criticality_reason TEXT                Pourquoi c'est critique             │
│                                         Ex: "Gestion authentification"      │
│  security_sensitive BOOLEAN DEFAULT 0   Contient du code sensible           │
│                                         (crypto, auth, etc.)                │
│                                                                             │
│  MÉTRIQUES DE CODE                                                          │
│  ────────────────                                                           │
│  lines_total     INTEGER DEFAULT 0      Lignes totales                      │
│  lines_code      INTEGER DEFAULT 0      Lignes de code (sans blancs/comm)   │
│  lines_comment   INTEGER DEFAULT 0      Lignes de commentaires              │
│  lines_blank     INTEGER DEFAULT 0      Lignes vides                        │
│  complexity_sum  INTEGER DEFAULT 0      Somme complexité des fonctions      │
│  complexity_avg  REAL DEFAULT 0         Complexité moyenne                  │
│  complexity_max  INTEGER DEFAULT 0      Complexité max (pire fonction)      │
│                                                                             │
│  MÉTRIQUES D'ACTIVITÉ                                                       │
│  ────────────────────                                                       │
│  commits_30d     INTEGER DEFAULT 0      Commits sur 30 derniers jours       │
│  commits_90d     INTEGER DEFAULT 0      Commits sur 90 derniers jours       │
│  commits_365d    INTEGER DEFAULT 0      Commits sur 365 derniers jours      │
│  contributors_json TEXT                 JSON: [{name, email, commits}]      │
│  last_modified   TEXT                   Date dernière modification          │
│  created_at      TEXT                   Date création du fichier            │
│                                                                             │
│  MÉTRIQUES DE QUALITÉ                                                       │
│  ────────────────────                                                       │
│  has_tests       BOOLEAN DEFAULT 0      A des tests associés                │
│  test_file_path  TEXT                   Chemin du fichier de test           │
│  documentation_score INTEGER DEFAULT 0  Score doc (0-100)                   │
│  technical_debt_score INTEGER DEFAULT 0 Score dette technique (0-100)       │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  content_hash    TEXT                   Hash SHA256 du contenu              │
│                                         (pour détecter les changements)     │
│  indexed_at      TEXT NOT NULL          Date/heure de l'indexation          │
│  index_version   INTEGER DEFAULT 1      Version du schéma d'indexation      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Table `symbols` - Les symboles du code

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              TABLE : symbols                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY    Identifiant unique                  │
│  file_id         INTEGER NOT NULL       FK vers files.id                    │
│  name            TEXT NOT NULL          Nom du symbole                      │
│                                         Ex: "lcd_init"                      │
│  qualified_name  TEXT                   Nom qualifié complet                │
│                                         Ex: "lcd::lcd_init" ou              │
│                                         "module.submodule.function"         │
│                                                                             │
│  CLASSIFICATION                                                             │
│  ──────────────                                                             │
│  kind            TEXT NOT NULL          Type de symbole :                   │
│                                         "function"    - fonction/méthode    │
│                                         "struct"      - structure           │
│                                         "class"       - classe              │
│                                         "enum"        - énumération         │
│                                         "typedef"     - alias de type       │
│                                         "macro"       - macro préprocesseur │
│                                         "variable"    - variable globale    │
│                                         "constant"    - constante           │
│                                         "interface"   - interface           │
│                                         "module"      - module/namespace    │
│                                                                             │
│  LOCALISATION                                                               │
│  ────────────                                                               │
│  line_start      INTEGER                Ligne de début                      │
│  line_end        INTEGER                Ligne de fin                        │
│  column_start    INTEGER                Colonne de début                    │
│  column_end      INTEGER                Colonne de fin                      │
│                                                                             │
│  SIGNATURE (pour les fonctions)                                             │
│  ─────────────────────────────                                              │
│  signature       TEXT                   Signature complète                  │
│                                         Ex: "int lcd_init(LCD_Config* cfg)" │
│  return_type     TEXT                   Type de retour                      │
│                                         Ex: "int", "void*", "LCD_Error"     │
│  parameters_json TEXT                   JSON des paramètres :               │
│                                         [{"name": "cfg",                    │
│                                           "type": "LCD_Config*",            │
│                                           "default": null}]                 │
│  is_variadic     BOOLEAN DEFAULT 0      Fonction variadic (...)             │
│                                                                             │
│  STRUCTURE (pour struct/class/enum)                                         │
│  ─────────────────────────────────                                          │
│  fields_json     TEXT                   JSON des champs :                   │
│                                         [{"name": "width",                  │
│                                           "type": "int",                    │
│                                           "offset": 0}]                     │
│  base_classes_json TEXT                 JSON des classes parentes           │
│  size_bytes      INTEGER                Taille en bytes (si connue)         │
│                                                                             │
│  VISIBILITÉ                                                                 │
│  ──────────                                                                 │
│  visibility      TEXT DEFAULT 'public'  "public", "private", "protected",   │
│                                         "internal", "static"                │
│  is_exported     BOOLEAN DEFAULT 0      Exporté (API publique)              │
│  is_static       BOOLEAN DEFAULT 0      Statique (interne au fichier)       │
│  is_inline       BOOLEAN DEFAULT 0      Fonction inline                     │
│                                                                             │
│  MÉTRIQUES                                                                  │
│  ─────────                                                                  │
│  complexity      INTEGER DEFAULT 0      Complexité cyclomatique             │
│  lines_of_code   INTEGER DEFAULT 0      Lignes de code du symbole           │
│  cognitive_complexity INTEGER DEFAULT 0 Complexité cognitive                │
│  nesting_depth   INTEGER DEFAULT 0      Profondeur max d'imbrication        │
│                                                                             │
│  DOCUMENTATION                                                              │
│  ─────────────                                                              │
│  doc_comment     TEXT                   Commentaire de documentation        │
│                                         (Doxygen, JSDoc, docstring, etc.)   │
│  has_doc         BOOLEAN DEFAULT 0      A une documentation                 │
│  doc_quality     INTEGER DEFAULT 0      Qualité doc (0-100)                 │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  attributes_json TEXT                   Attributs additionnels              │
│                                         Ex: {"deprecated": true,            │
│                                              "since": "v2.0"}               │
│  hash            TEXT                   Hash du contenu du symbole          │
│  indexed_at      TEXT NOT NULL          Date/heure de l'indexation          │
│                                                                             │
│  CONTRAINTE UNIQUE                                                          │
│  ─────────────────                                                          │
│  UNIQUE(file_id, name, kind, line_start)                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2.3 Les Arêtes : Relations

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            TABLE : relations                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY    Identifiant unique                  │
│  source_id       INTEGER NOT NULL       FK vers symbols.id (qui initie)     │
│  target_id       INTEGER NOT NULL       FK vers symbols.id (qui est ciblé)  │
│                                                                             │
│  TYPE DE RELATION                                                           │
│  ────────────────                                                           │
│  relation_type   TEXT NOT NULL          Type de la relation :               │
│                                                                             │
│                  "calls"          A appelle B (fonction → fonction)         │
│                  "includes"       A inclut B (fichier → fichier)            │
│                  "imports"        A importe B (module → module)             │
│                  "uses_type"      A utilise le type B                       │
│                  "returns_type"   A retourne le type B                      │
│                  "has_param_type" A a un paramètre de type B                │
│                  "inherits"       A hérite de B (classe → classe)           │
│                  "implements"     A implémente B (classe → interface)       │
│                  "uses_variable"  A utilise la variable B                   │
│                  "modifies"       A modifie B (écriture)                    │
│                  "reads"          A lit B (lecture seule)                   │
│                  "instantiates"   A crée une instance de B                  │
│                  "uses_macro"     A utilise la macro B                      │
│                  "contains"       A contient B (struct → field)             │
│                  "references"     A référence B (générique)                 │
│                                                                             │
│  LOCALISATION                                                               │
│  ────────────                                                               │
│  location_file_id INTEGER               FK vers files.id où la relation     │
│                                         est établie (peut différer de       │
│                                         source si relation cross-file)      │
│  location_line   INTEGER                Ligne où la relation est établie    │
│  location_column INTEGER                Colonne                             │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  count           INTEGER DEFAULT 1      Nombre d'occurrences                │
│                                         (ex: A appelle B 5 fois)            │
│  is_direct       BOOLEAN DEFAULT 1      Relation directe (pas via ptr)      │
│  is_conditional  BOOLEAN DEFAULT 0      Dans un bloc conditionnel           │
│  context         TEXT                   Contexte additionnel                │
│                                         Ex: "in_loop", "in_error_handler"   │
│                                                                             │
│  CONTRAINTES                                                                │
│  ───────────                                                                │
│  FOREIGN KEY (source_id) REFERENCES symbols(id) ON DELETE CASCADE           │
│  FOREIGN KEY (target_id) REFERENCES symbols(id) ON DELETE CASCADE           │
│  UNIQUE(source_id, target_id, relation_type, location_line)                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2.4 Relations entre Fichiers

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TABLE : file_relations                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Pour les relations de haut niveau entre fichiers                           │
│  (complémente la table relations pour une vue macro)                        │
│                                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  source_file_id  INTEGER NOT NULL       FK vers files.id                    │
│  target_file_id  INTEGER NOT NULL       FK vers files.id                    │
│  relation_type   TEXT NOT NULL          "includes", "imports", "depends"    │
│  is_direct       BOOLEAN DEFAULT 1      Inclusion directe vs transitive     │
│  line_number     INTEGER                Ligne de l'include/import           │
│                                                                             │
│  UNIQUE(source_file_id, target_file_id, relation_type)                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2.5 Visualisation du Graphe

```
EXEMPLE : Graphe autour de lcd_init()

                         ┌─────────────────┐
                         │     main()      │
                         │   src/main.c    │
                         └────────┬────────┘
                                  │ calls
                                  ▼
                         ┌─────────────────┐
                         │  system_init()  │
                         │ src/system.c    │
                         └────────┬────────┘
                                  │ calls
         ┌────────────────────────┼────────────────────────┐
         │                        │                        │
         ▼                        ▼                        ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   lcd_init()    │     │   uart_init()   │     │   gpio_init()   │
│ src/lcd/init.c  │     │  src/uart.c     │     │  src/gpio.c     │
└────────┬────────┘     └─────────────────┘     └────────┬────────┘
         │                                               │
         │ calls                                         │ uses_type
         │                                               │
         ▼                                               ▼
┌─────────────────┐                            ┌─────────────────┐
│ alloc_buffer()  │                            │   GPIO_Config   │
│ src/memory.c    │                            │  src/gpio.h     │
└────────┬────────┘                            └─────────────────┘
         │
         │ uses_type
         ▼
┌─────────────────┐
│  LCD_Buffer     │
│ src/lcd/types.h │
└─────────────────┘


QUESTION : "Si je modifie LCD_Buffer, quel est l'impact ?"

RÉPONSE (traversée du graphe vers le haut) :
  - alloc_buffer() utilise LCD_Buffer        → IMPACTÉ
  - lcd_init() appelle alloc_buffer()        → POTENTIELLEMENT IMPACTÉ
  - system_init() appelle lcd_init()         → POTENTIELLEMENT IMPACTÉ
  - main() appelle system_init()             → POTENTIELLEMENT IMPACTÉ

IMPACT TOTAL : 4 fonctions, 4 fichiers
```

---

# PARTIE 3 : LA MÉMOIRE HISTORIQUE

## 3.1 Concept

La mémoire historique répond à la question :

> **"Qu'est-ce qui s'est passé avant sur ce code ?"**

Elle permet de :
- Détecter les **régressions** (bugs qui reviennent)
- Apprendre des **erreurs passées**
- Identifier les **zones à risque** (fichiers avec beaucoup de bugs)

## 3.2 Historique des Erreurs

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          TABLE : error_history                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  file_id         INTEGER                FK vers files.id (peut être NULL    │
│                                         si fichier supprimé)                │
│  file_path       TEXT NOT NULL          Chemin du fichier (pour historique) │
│  symbol_name     TEXT                   Fonction/symbole concerné           │
│  symbol_id       INTEGER                FK vers symbols.id (si existe)      │
│                                                                             │
│  CLASSIFICATION DE L'ERREUR                                                 │
│  ──────────────────────────                                                 │
│  error_type      TEXT NOT NULL          Catégorie principale :              │
│                                         "buffer_overflow"                   │
│                                         "null_pointer"                      │
│                                         "memory_leak"                       │
│                                         "use_after_free"                    │
│                                         "race_condition"                    │
│                                         "sql_injection"                     │
│                                         "xss"                               │
│                                         "auth_bypass"                       │
│                                         "logic_error"                       │
│                                         "performance"                       │
│                                         "crash"                             │
│                                         "data_corruption"                   │
│                                         "regression"                        │
│                                         "other"                             │
│                                                                             │
│  severity        TEXT NOT NULL          "critical", "high", "medium", "low" │
│                                                                             │
│  cwe_id          TEXT                   CWE ID si applicable                │
│                                         Ex: "CWE-120", "CWE-89"             │
│                                                                             │
│  DESCRIPTION                                                                │
│  ───────────                                                                │
│  title           TEXT NOT NULL          Titre court de l'erreur             │
│  description     TEXT                   Description détaillée               │
│  root_cause      TEXT                   Cause racine identifiée             │
│  symptoms        TEXT                   Comment l'erreur se manifestait     │
│                                                                             │
│  RÉSOLUTION                                                                 │
│  ──────────                                                                 │
│  resolution      TEXT                   Comment ça a été corrigé            │
│  prevention      TEXT                   Comment éviter à l'avenir           │
│  fix_commit      TEXT                   Hash du commit de correction        │
│  fix_diff        TEXT                   Diff de la correction (optionnel)   │
│                                                                             │
│  CONTEXTE                                                                   │
│  ────────                                                                   │
│  discovered_at   TEXT NOT NULL          Date de découverte                  │
│  resolved_at     TEXT                   Date de résolution                  │
│  discovered_by   TEXT                   Qui a découvert (pipeline, humain)  │
│  reported_in     TEXT                   Où rapporté (JIRA, GitHub, etc.)    │
│  jira_ticket     TEXT                   Ticket JIRA associé                 │
│  environment     TEXT                   "production", "staging", "dev"      │
│                                                                             │
│  COMMITS ASSOCIÉS                                                           │
│  ────────────────                                                           │
│  introducing_commit TEXT                Commit qui a introduit le bug       │
│  related_commits_json TEXT              JSON: ["commit1", "commit2"]        │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  is_regression   BOOLEAN DEFAULT 0      Est-ce une régression ?             │
│  original_error_id INTEGER              FK si régression d'un bug passé     │
│  tags_json       TEXT                   JSON: ["security", "urgent"]        │
│  extra_data_json TEXT                   Données additionnelles              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 3.3 Historique des Runs du Pipeline

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          TABLE : pipeline_runs                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  run_id          TEXT UNIQUE NOT NULL   UUID du run                         │
│                                         Ex: "run-20241205-143052-abc123"    │
│                                                                             │
│  CONTEXTE GIT                                                               │
│  ────────────                                                               │
│  commit_hash     TEXT NOT NULL          Hash du commit analysé              │
│  commit_message  TEXT                   Message du commit                   │
│  commit_author   TEXT                   Auteur du commit                    │
│  branch_source   TEXT                   Branche source (feature/xxx)        │
│  branch_target   TEXT                   Branche cible (develop, main)       │
│  merge_type      TEXT                   "feature", "hotfix", "release"      │
│                                                                             │
│  CONTEXTE JIRA                                                              │
│  ─────────────                                                              │
│  jira_key        TEXT                   Ticket JIRA associé                 │
│  jira_type       TEXT                   Type: Story, Bug, Task              │
│  jira_summary    TEXT                   Résumé du ticket                    │
│                                                                             │
│  RÉSULTATS                                                                  │
│  ─────────                                                                  │
│  status          TEXT NOT NULL          "success", "warning", "failed"      │
│  overall_score   INTEGER                Score global 0-100                  │
│  recommendation  TEXT                   "approve", "hold", "reject"         │
│                                                                             │
│  SCORES PAR AGENT                                                           │
│  ────────────────                                                           │
│  score_analyzer  INTEGER                Score de l'agent Analyzer           │
│  score_security  INTEGER                Score de l'agent Security           │
│  score_reviewer  INTEGER                Score de l'agent Reviewer           │
│  score_risk      INTEGER                Score de l'agent Risk               │
│                                                                             │
│  ISSUES                                                                     │
│  ──────                                                                     │
│  issues_critical INTEGER DEFAULT 0      Nombre d'issues critiques           │
│  issues_high     INTEGER DEFAULT 0      Nombre d'issues high                │
│  issues_medium   INTEGER DEFAULT 0      Nombre d'issues medium              │
│  issues_low      INTEGER DEFAULT 0      Nombre d'issues low                 │
│  issues_json     TEXT                   JSON complet des issues             │
│                                                                             │
│  FICHIERS ANALYSÉS                                                          │
│  ─────────────────                                                          │
│  files_analyzed  INTEGER                Nombre de fichiers analysés         │
│  files_json      TEXT                   JSON: ["file1.c", "file2.c"]        │
│                                                                             │
│  RAPPORTS                                                                   │
│  ────────                                                                   │
│  report_path     TEXT                   Chemin du rapport Markdown          │
│  report_json_path TEXT                  Chemin du rapport JSON              │
│  context_path    TEXT                   Chemin du contexte utilisé          │
│                                                                             │
│  TIMING                                                                     │
│  ──────                                                                     │
│  started_at      TEXT NOT NULL          Début du run                        │
│  completed_at    TEXT                   Fin du run                          │
│  duration_ms     INTEGER                Durée en millisecondes              │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  trigger         TEXT                   "hook", "command", "manual"         │
│  pipeline_version TEXT                  Version du pipeline                 │
│  agents_used_json TEXT                  JSON: ["analyzer", "security", ...] │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 3.4 Snapshot Temporaire

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TABLE : snapshot_symbols                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Table temporaire pour comparer l'état N-1 avec l'état N                    │
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  run_id          TEXT NOT NULL          UUID du run en cours                │
│  file_path       TEXT NOT NULL          Chemin du fichier                   │
│                                                                             │
│  SYMBOLE CAPTURÉ                                                            │
│  ───────────────                                                            │
│  symbol_name     TEXT NOT NULL          Nom du symbole                      │
│  symbol_kind     TEXT NOT NULL          Type du symbole                     │
│  signature       TEXT                   Signature (avant modification)      │
│  complexity      INTEGER                Complexité (avant modification)     │
│  line_start      INTEGER                Position (avant modification)       │
│  line_end        INTEGER                                                    │
│  hash            TEXT                   Hash du contenu                     │
│                                                                             │
│  LIFECYCLE                                                                  │
│  ─────────                                                                  │
│  created_at      TEXT NOT NULL          Date de création du snapshot        │
│                                                                             │
│  Note : Cette table est VIDÉE après chaque run (cleanup)                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

# PARTIE 4 : LA BASE DE CONNAISSANCES

## 4.1 Concept

La base de connaissances répond à la question :

> **"Comment ce code doit-il être écrit ?"**

Elle stocke :
- Les **patterns** établis du projet
- Les **conventions** de code
- Les **décisions architecturales** (ADR)
- Les **anti-patterns** à éviter

## 4.2 Patterns du Projet

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            TABLE : patterns                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  name            TEXT UNIQUE NOT NULL   Nom du pattern                      │
│                                         Ex: "error_handling_lcd"            │
│  category        TEXT NOT NULL          Catégorie :                         │
│                                         "error_handling"                    │
│                                         "memory_management"                 │
│                                         "naming_convention"                 │
│                                         "api_design"                        │
│                                         "security"                          │
│                                         "performance"                       │
│                                         "testing"                           │
│                                         "documentation"                     │
│                                                                             │
│  SCOPE                                                                      │
│  ─────                                                                      │
│  scope           TEXT DEFAULT 'project' Portée du pattern :                 │
│                                         "project" - tout le projet          │
│                                         "module"  - un module spécifique    │
│                                         "file"    - un fichier spécifique   │
│  module          TEXT                   Module concerné (si scope=module)   │
│  file_pattern    TEXT                   Glob pattern des fichiers           │
│                                         Ex: "src/lcd/*.c"                   │
│                                                                             │
│  DESCRIPTION                                                                │
│  ───────────                                                                │
│  title           TEXT NOT NULL          Titre lisible                       │
│                                         Ex: "Gestion des erreurs LCD"       │
│  description     TEXT NOT NULL          Description complète                │
│  rationale       TEXT                   Pourquoi ce pattern existe          │
│                                                                             │
│  EXEMPLES                                                                   │
│  ────────                                                                   │
│  good_example    TEXT                   Exemple de code correct             │
│  bad_example     TEXT                   Exemple de code incorrect           │
│                                         (anti-pattern)                      │
│  explanation     TEXT                   Explication de la différence        │
│                                                                             │
│  RÈGLES                                                                     │
│  ──────                                                                     │
│  rules_json      TEXT                   JSON des règles vérifiables :       │
│                                         [{"type": "must_use",               │
│                                           "pattern": "LCD_ERR_*"},          │
│                                          {"type": "must_not_use",           │
│                                           "pattern": "exit()"}]             │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  severity        TEXT DEFAULT 'warning' "error", "warning", "info"          │
│  is_active       BOOLEAN DEFAULT 1      Pattern actif ou désactivé          │
│  created_at      TEXT NOT NULL                                              │
│  updated_at      TEXT                                                       │
│  created_by      TEXT                   Qui a créé ce pattern               │
│                                                                             │
│  RÉFÉRENCES                                                                 │
│  ───────────                                                                │
│  related_adr     TEXT                   ADR associé (si applicable)         │
│  external_link   TEXT                   Lien vers doc externe               │
│  examples_in_code_json TEXT             JSON: chemins de bons exemples      │
│                                         dans le codebase                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 4.3 Décisions Architecturales (ADR)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     TABLE : architecture_decisions                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  IDENTIFICATION                                                             │
│  ──────────────                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  decision_id     TEXT UNIQUE NOT NULL   ID de la décision                   │
│                                         Ex: "ADR-007"                       │
│                                                                             │
│  STATUT                                                                     │
│  ──────                                                                     │
│  status          TEXT NOT NULL          "proposed"   - en discussion        │
│                                         "accepted"   - validée              │
│                                         "deprecated" - obsolète             │
│                                         "superseded" - remplacée            │
│  superseded_by   TEXT                   ADR qui remplace celle-ci           │
│                                                                             │
│  CONTENU                                                                    │
│  ───────                                                                    │
│  title           TEXT NOT NULL          Titre de la décision                │
│                                         Ex: "Utiliser SQLite pour AgentDB"  │
│  context         TEXT NOT NULL          Contexte / problème à résoudre      │
│  decision        TEXT NOT NULL          La décision prise                   │
│  consequences    TEXT                   Conséquences positives/négatives    │
│  alternatives    TEXT                   Alternatives considérées            │
│                                                                             │
│  SCOPE                                                                      │
│  ─────                                                                      │
│  affected_modules_json TEXT             JSON: ["lcd", "security"]           │
│  affected_files_json TEXT               JSON: ["src/lcd/*", "src/api/*"]    │
│                                                                             │
│  MÉTADONNÉES                                                                │
│  ───────────                                                                │
│  date_proposed   TEXT                   Date de proposition                 │
│  date_decided    TEXT                   Date de décision                    │
│  decided_by      TEXT                   Qui a pris la décision              │
│  stakeholders_json TEXT                 JSON: ["Alice", "Bob"]              │
│                                                                             │
│  LIENS                                                                      │
│  ─────                                                                      │
│  related_adrs_json TEXT                 JSON: ["ADR-003", "ADR-005"]        │
│  jira_tickets_json TEXT                 JSON: ["ARCH-123", "ARCH-456"]      │
│  documentation_link TEXT                Lien vers doc complète              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 4.4 Configuration des Chemins Critiques

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TABLE : critical_paths                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Définit quels chemins/patterns sont considérés critiques                   │
│                                                                             │
│  id              INTEGER PRIMARY KEY                                        │
│  pattern         TEXT UNIQUE NOT NULL   Glob pattern                        │
│                                         Ex: "src/security/**"               │
│                                         Ex: "src/*/auth*.c"                 │
│  reason          TEXT NOT NULL          Pourquoi c'est critique             │
│  severity        TEXT DEFAULT 'high'    "critical", "high", "medium"        │
│  added_by        TEXT                   Qui a ajouté ce pattern             │
│  added_at        TEXT NOT NULL                                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

# PARTIE 5 : LE SCHÉMA SQL COMPLET

## 5.1 Création des Tables

Voici le schéma SQL complet à implémenter dans `.claude/agentdb/schema.sql` :

```sql
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
```

---

# PARTIE 6 : LES REQUÊTES DE TRAVERSÉE

## 6.1 Requêtes Fondamentales du Graphe

Ces requêtes sont le cœur de l'analyse d'impact. Elles doivent être implémentées comme des fonctions réutilisables.

### Trouver les appelants d'une fonction (upstream)

```
OBJECTIF : "Qui appelle cette fonction ?"

Utilisé par : Agent ANALYZER pour calculer l'impact

REQUÊTE RÉCURSIVE (CTE) :

WITH RECURSIVE callers AS (
    -- Cas de base : appelants directs
    SELECT 
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        r.location_line,
        1 as depth
    FROM symbols s
    JOIN relations r ON r.source_id = s.id
    JOIN files f ON s.file_id = f.id
    WHERE r.target_id = :symbol_id
    AND r.relation_type = 'calls'
    
    UNION ALL
    
    -- Cas récursif : appelants des appelants
    SELECT 
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        r.location_line,
        c.depth + 1 as depth
    FROM symbols s
    JOIN relations r ON r.source_id = s.id
    JOIN files f ON s.file_id = f.id
    JOIN callers c ON r.target_id = c.id
    WHERE r.relation_type = 'calls'
    AND c.depth < :max_depth  -- Limiter la profondeur
)
SELECT DISTINCT * FROM callers
ORDER BY depth, name;

PARAMÈTRES :
- :symbol_id  = ID du symbole cible
- :max_depth  = Profondeur max (défaut: 3)

RETOUR :
[
  { "id": 1, "name": "main", "file_path": "src/main.c", "depth": 2 },
  { "id": 2, "name": "system_init", "file_path": "src/system.c", "depth": 1 },
  ...
]
```

### Trouver les appelés d'une fonction (downstream)

```
OBJECTIF : "Que appelle cette fonction ?"

Utilisé par : Agent ANALYZER pour comprendre les dépendances

REQUÊTE (même structure, direction inversée) :

WITH RECURSIVE callees AS (
    SELECT 
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        r.location_line,
        1 as depth
    FROM symbols s
    JOIN relations r ON r.target_id = s.id
    JOIN files f ON s.file_id = f.id
    WHERE r.source_id = :symbol_id
    AND r.relation_type = 'calls'
    
    UNION ALL
    
    SELECT 
        s.id,
        s.name,
        s.kind,
        f.path as file_path,
        r.location_line,
        c.depth + 1 as depth
    FROM symbols s
    JOIN relations r ON r.target_id = s.id
    JOIN files f ON s.file_id = f.id
    JOIN callees c ON r.source_id = c.id
    WHERE r.relation_type = 'calls'
    AND c.depth < :max_depth
)
SELECT DISTINCT * FROM callees
ORDER BY depth, name;
```

### Calculer l'impact d'un fichier

```
OBJECTIF : "Si je modifie ce fichier, quels autres fichiers sont impactés ?"

Utilisé par : Agent ANALYZER, Agent RISK

REQUÊTE EN 2 ÉTAPES :

-- Étape 1 : Fichiers qui incluent/importent ce fichier
SELECT DISTINCT f2.id, f2.path, 'includes' as reason
FROM files f1
JOIN file_relations fr ON fr.target_file_id = f1.id
JOIN files f2 ON fr.source_file_id = f2.id
WHERE f1.path = :file_path
AND fr.relation_type IN ('includes', 'imports')

UNION

-- Étape 2 : Fichiers dont les symboles appellent des symboles de ce fichier
SELECT DISTINCT f2.id, f2.path, 'calls' as reason
FROM files f1
JOIN symbols s1 ON s1.file_id = f1.id
JOIN relations r ON r.target_id = s1.id
JOIN symbols s2 ON r.source_id = s2.id
JOIN files f2 ON s2.file_id = f2.id
WHERE f1.path = :file_path
AND r.relation_type = 'calls'
AND f2.id != f1.id;

RETOUR :
[
  { "id": 5, "path": "src/main.c", "reason": "calls" },
  { "id": 8, "path": "src/test/test_lcd.c", "reason": "includes" },
  ...
]
```

### Trouver les utilisateurs d'un type

```
OBJECTIF : "Quelles fonctions utilisent ce type/struct ?"

Utilisé par : Agent ANALYZER pour les breaking changes de types

REQUÊTE :

SELECT DISTINCT
    s.id,
    s.name,
    s.kind,
    f.path,
    r.relation_type,
    r.location_line
FROM symbols s
JOIN relations r ON r.source_id = s.id
JOIN files f ON s.file_id = f.id
WHERE r.target_id = :type_symbol_id
AND r.relation_type IN ('uses_type', 'returns_type', 'has_param_type', 'instantiates')
ORDER BY f.path, s.name;
```

### Obtenir l'arbre des includes

```
OBJECTIF : "Quel est l'arbre d'inclusion de ce fichier ?"

REQUÊTE RÉCURSIVE :

WITH RECURSIVE include_tree AS (
    SELECT 
        f.id,
        f.path,
        0 as depth,
        f.path as root_path
    FROM files f
    WHERE f.path = :file_path
    
    UNION ALL
    
    SELECT 
        f2.id,
        f2.path,
        it.depth + 1,
        it.root_path
    FROM file_relations fr
    JOIN include_tree it ON fr.source_file_id = it.id
    JOIN files f2 ON fr.target_file_id = f2.id
    WHERE fr.relation_type = 'includes'
    AND it.depth < :max_depth
)
SELECT * FROM include_tree
WHERE depth > 0
ORDER BY depth, path;
```

## 6.2 Requêtes pour l'Historique

### Erreurs récentes sur un fichier

```
OBJECTIF : "Quels bugs ont été trouvés sur ce fichier ?"

Utilisé par : Agent SECURITY, Agent RISK

REQUÊTE :

SELECT 
    id,
    error_type,
    severity,
    title,
    description,
    resolution,
    discovered_at,
    resolved_at,
    is_regression
FROM error_history
WHERE file_path = :file_path
   OR file_id = (SELECT id FROM files WHERE path = :file_path)
ORDER BY discovered_at DESC
LIMIT :limit;

PARAMÈTRES :
- :file_path = Chemin du fichier
- :limit = Nombre max de résultats (défaut: 10)
```

### Erreurs par type sur un module

```
OBJECTIF : "Quels types de bugs sont fréquents dans ce module ?"

Utilisé par : Agent SECURITY pour patterns de vulnérabilités

REQUÊTE :

SELECT 
    error_type,
    severity,
    COUNT(*) as count,
    MAX(discovered_at) as last_occurrence
FROM error_history e
JOIN files f ON e.file_id = f.id OR e.file_path LIKE :module_pattern
WHERE f.module = :module_name
GROUP BY error_type, severity
ORDER BY count DESC, severity DESC;
```

## 6.3 Requêtes pour les Patterns

### Patterns applicables à un fichier

```
OBJECTIF : "Quels patterns dois-je vérifier pour ce fichier ?"

Utilisé par : Agent REVIEWER

REQUÊTE :

SELECT * FROM patterns
WHERE is_active = 1
AND (
    scope = 'project'
    OR (scope = 'module' AND module = :module_name)
    OR (scope = 'file' AND :file_path GLOB file_pattern)
)
ORDER BY severity DESC, category;
```

### ADRs applicables à un module

```
OBJECTIF : "Quelles décisions architecturales concernent ce module ?"

Utilisé par : Tous les agents pour le contexte

REQUÊTE :

SELECT * FROM architecture_decisions
WHERE status = 'accepted'
AND (
    affected_modules_json LIKE '%"' || :module_name || '"%'
    OR affected_files_json LIKE '%' || :file_pattern || '%'
)
ORDER BY date_decided DESC;
```

---

# PARTIE 7 : LE SERVEUR MCP

## 7.1 Architecture du Serveur

Le serveur MCP expose AgentDB aux agents Claude via des outils standardisés.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MCP SERVER AGENTDB                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  TRANSPORT                                                                  │
│  ─────────                                                                  │
│  Protocole : stdio (standard input/output)                                  │
│  Format : JSON-RPC 2.0                                                      │
│                                                                             │
│  CONNEXION DB                                                               │
│  ───────────                                                                │
│  Type : SQLite                                                              │
│  Path : .claude/agentdb/db.sqlite                                           │
│  Mode : WAL (Write-Ahead Logging) pour performance                          │
│                                                                             │
│  OUTILS EXPOSÉS (10)                                                        │
│  ───────────────────                                                        │
│                                                                             │
│  1. get_file_context        Contexte complet d'un fichier                   │
│  2. get_symbol_callers      Appelants d'un symbole (récursif)               │
│  3. get_symbol_callees      Appelés d'un symbole (récursif)                 │
│  4. get_file_impact         Fichiers impactés par une modification          │
│  5. get_error_history       Historique des erreurs                          │
│  6. get_patterns            Patterns applicables                            │
│  7. get_architecture_decisions  ADRs applicables                            │
│  8. search_symbols          Recherche de symboles                           │
│  9. get_file_metrics        Métriques d'un fichier                          │
│  10. get_module_summary     Résumé d'un module                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 7.2 Spécification des Outils

### Outil 1 : get_file_context

```
NOM : get_file_context

DESCRIPTION :
Récupère le contexte complet d'un fichier : métadonnées, symboles, 
dépendances, historique d'erreurs, métriques, et patterns applicables.

C'est l'outil le plus utilisé - il donne une vue 360° d'un fichier.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "path": {
      "type": "string",
      "description": "Chemin du fichier relatif à la racine du projet"
    },
    "include_symbols": {
      "type": "boolean",
      "default": true,
      "description": "Inclure la liste des symboles"
    },
    "include_dependencies": {
      "type": "boolean", 
      "default": true,
      "description": "Inclure les dépendances (includes, appelants)"
    },
    "include_history": {
      "type": "boolean",
      "default": true,
      "description": "Inclure l'historique des erreurs"
    },
    "include_patterns": {
      "type": "boolean",
      "default": true,
      "description": "Inclure les patterns applicables"
    }
  },
  "required": ["path"]
}

OUTPUT :
{
  "file": {
    "path": "src/lcd/lcd_init.c",
    "module": "lcd",
    "language": "c",
    "is_critical": false,
    "security_sensitive": false,
    "metrics": {
      "lines_total": 245,
      "lines_code": 180,
      "lines_comment": 45,
      "complexity_avg": 8.5,
      "complexity_max": 15
    },
    "activity": {
      "commits_30d": 3,
      "commits_90d": 8,
      "last_modified": "2024-11-28",
      "contributors": ["alice", "bob"]
    }
  },
  "symbols": [
    {
      "name": "lcd_init",
      "kind": "function",
      "signature": "int lcd_init(LCD_Config* cfg)",
      "complexity": 12,
      "has_doc": true,
      "line_start": 45,
      "line_end": 98
    },
    ...
  ],
  "dependencies": {
    "includes": ["lcd.h", "hardware/gpio.h"],
    "included_by": ["main.c", "test_lcd.c"],
    "calls_to": ["alloc_buffer", "configure_pins"],
    "called_by": ["system_init", "main"]
  },
  "error_history": [
    {
      "type": "buffer_overflow",
      "severity": "critical",
      "title": "Buffer overflow in lcd_write",
      "resolved_at": "2024-03-15",
      "resolution": "Replaced strcpy with strncpy"
    }
  ],
  "patterns": [
    {
      "name": "error_handling_lcd",
      "title": "Gestion des erreurs LCD",
      "description": "Utiliser les codes LCD_ERR_*"
    }
  ],
  "architecture_decisions": [
    {
      "id": "ADR-007",
      "title": "Singleton pour LCD Controller"
    }
  ]
}
```

### Outil 2 : get_symbol_callers

```
NOM : get_symbol_callers

DESCRIPTION :
Trouve tous les symboles qui appellent le symbole donné, avec traversée 
récursive jusqu'à une profondeur configurable.

Essentiel pour l'analyse d'impact : "Si je modifie cette fonction, 
qu'est-ce qui peut casser ?"

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "symbol_name": {
      "type": "string",
      "description": "Nom du symbole (fonction, variable, etc.)"
    },
    "file_path": {
      "type": "string",
      "description": "Fichier du symbole (pour désambiguïser)"
    },
    "max_depth": {
      "type": "integer",
      "default": 3,
      "minimum": 1,
      "maximum": 10,
      "description": "Profondeur maximale de traversée"
    },
    "include_indirect": {
      "type": "boolean",
      "default": true,
      "description": "Inclure les appels indirects (via pointeurs)"
    }
  },
  "required": ["symbol_name"]
}

OUTPUT :
{
  "symbol": {
    "name": "lcd_init",
    "file": "src/lcd/lcd_init.c",
    "kind": "function"
  },
  "callers": {
    "level_1": [
      {
        "name": "system_init",
        "file": "src/system/init.c",
        "line": 45,
        "is_direct": true
      }
    ],
    "level_2": [
      {
        "name": "main",
        "file": "src/main.c",
        "line": 23,
        "is_direct": true
      }
    ],
    "level_3": []
  },
  "summary": {
    "total_callers": 2,
    "max_depth_reached": 2,
    "critical_callers": 1,
    "files_affected": ["src/system/init.c", "src/main.c"]
  }
}
```

### Outil 3 : get_symbol_callees

```
NOM : get_symbol_callees

DESCRIPTION :
Trouve tous les symboles appelés par le symbole donné.
Utile pour comprendre les dépendances d'une fonction.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "symbol_name": {
      "type": "string",
      "description": "Nom du symbole"
    },
    "file_path": {
      "type": "string",
      "description": "Fichier du symbole (optionnel)"
    },
    "max_depth": {
      "type": "integer",
      "default": 2,
      "description": "Profondeur de traversée"
    }
  },
  "required": ["symbol_name"]
}

OUTPUT :
{
  "symbol": {
    "name": "lcd_init",
    "file": "src/lcd/lcd_init.c"
  },
  "callees": {
    "level_1": [
      {
        "name": "alloc_buffer",
        "file": "src/memory/alloc.c",
        "kind": "function"
      },
      {
        "name": "configure_pins",
        "file": "src/hardware/gpio.c",
        "kind": "function"
      }
    ],
    "level_2": [
      {
        "name": "malloc",
        "file": "stdlib",
        "kind": "function",
        "external": true
      }
    ]
  },
  "types_used": [
    {
      "name": "LCD_Config",
      "file": "src/lcd/types.h"
    }
  ]
}
```

### Outil 4 : get_file_impact

```
NOM : get_file_impact

DESCRIPTION :
Calcule l'impact complet de la modification d'un fichier.
Combine : fichiers qui incluent + fichiers avec symboles appelants.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "path": {
      "type": "string",
      "description": "Chemin du fichier"
    },
    "include_transitive": {
      "type": "boolean",
      "default": true,
      "description": "Inclure les impacts transitifs"
    }
  },
  "required": ["path"]
}

OUTPUT :
{
  "file": "src/lcd/lcd_init.c",
  "direct_impact": [
    {
      "file": "src/main.c",
      "reason": "calls lcd_init",
      "symbols": ["main"]
    },
    {
      "file": "src/system/init.c", 
      "reason": "calls lcd_init",
      "symbols": ["system_init"]
    }
  ],
  "transitive_impact": [
    {
      "file": "src/boot/boot.c",
      "reason": "calls system_init",
      "depth": 2
    }
  ],
  "include_impact": [
    {
      "file": "src/test/test_lcd.c",
      "reason": "includes lcd.h"
    }
  ],
  "summary": {
    "total_files_impacted": 4,
    "critical_files_impacted": 1,
    "max_depth": 2
  }
}
```

### Outil 5 : get_error_history

```
NOM : get_error_history

DESCRIPTION :
Récupère l'historique des erreurs/bugs pour un fichier, un symbole, 
ou un module entier.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "file_path": {
      "type": "string",
      "description": "Filtrer par fichier"
    },
    "symbol_name": {
      "type": "string",
      "description": "Filtrer par symbole"
    },
    "module": {
      "type": "string",
      "description": "Filtrer par module"
    },
    "error_type": {
      "type": "string",
      "description": "Filtrer par type d'erreur"
    },
    "severity": {
      "type": "string",
      "enum": ["critical", "high", "medium", "low"],
      "description": "Filtrer par sévérité minimum"
    },
    "days": {
      "type": "integer",
      "default": 180,
      "description": "Période en jours"
    },
    "limit": {
      "type": "integer",
      "default": 20,
      "description": "Nombre max de résultats"
    }
  }
}

OUTPUT :
{
  "query": {
    "file_path": "src/lcd/lcd_init.c",
    "days": 180
  },
  "errors": [
    {
      "id": 42,
      "type": "buffer_overflow",
      "severity": "critical",
      "title": "Buffer overflow in lcd_write",
      "description": "strcpy without bounds checking",
      "discovered_at": "2024-03-10",
      "resolved_at": "2024-03-15",
      "resolution": "Replaced strcpy with strncpy",
      "prevention": "Always use bounded string functions",
      "is_regression": false,
      "jira_ticket": "SEC-123"
    }
  ],
  "statistics": {
    "total_errors": 3,
    "by_type": {
      "buffer_overflow": 2,
      "null_pointer": 1
    },
    "by_severity": {
      "critical": 1,
      "high": 1,
      "medium": 1
    },
    "regression_rate": 0.0
  }
}
```

### Outil 6 : get_patterns

```
NOM : get_patterns

DESCRIPTION :
Récupère les patterns de code applicables à un fichier ou module.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "file_path": {
      "type": "string",
      "description": "Fichier pour lequel récupérer les patterns"
    },
    "module": {
      "type": "string",
      "description": "Module pour lequel récupérer les patterns"
    },
    "category": {
      "type": "string",
      "description": "Catégorie de patterns"
    }
  }
}

OUTPUT :
{
  "applicable_patterns": [
    {
      "name": "error_handling_lcd",
      "category": "error_handling",
      "title": "Gestion des erreurs LCD",
      "description": "Toutes les fonctions du module LCD doivent...",
      "severity": "error",
      "good_example": "if (status != LCD_OK) return LCD_ERR_INIT;",
      "bad_example": "if (status != 0) exit(1);",
      "rules": [
        {"type": "must_use", "pattern": "LCD_ERR_*"},
        {"type": "must_not_use", "pattern": "exit()"}
      ]
    }
  ],
  "project_patterns": [
    {
      "name": "naming_functions",
      "category": "naming_convention",
      "title": "Nommage des fonctions",
      "description": "Préfixer par le nom du module"
    }
  ]
}
```

### Outil 7 : get_architecture_decisions

```
NOM : get_architecture_decisions

DESCRIPTION :
Récupère les décisions architecturales (ADR) applicables.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "module": {
      "type": "string",
      "description": "Filtrer par module"
    },
    "file_path": {
      "type": "string",
      "description": "Filtrer par fichier"
    },
    "status": {
      "type": "string",
      "enum": ["accepted", "proposed", "deprecated"],
      "default": "accepted"
    }
  }
}

OUTPUT :
{
  "decisions": [
    {
      "id": "ADR-007",
      "title": "Utiliser le pattern Singleton pour LCD",
      "status": "accepted",
      "context": "Le LCD ne peut être initialisé qu'une fois...",
      "decision": "Implémenter LCD Controller comme singleton...",
      "consequences": "Une seule instance, thread-safe required...",
      "date_decided": "2024-01-15",
      "decided_by": "Architecture Team"
    }
  ]
}
```

### Outil 8 : search_symbols

```
NOM : search_symbols

DESCRIPTION :
Recherche des symboles par nom, type, ou pattern.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "query": {
      "type": "string",
      "description": "Pattern de recherche (supporte * et ?)"
    },
    "kind": {
      "type": "string",
      "enum": ["function", "struct", "class", "enum", "macro", "variable"],
      "description": "Type de symbole"
    },
    "module": {
      "type": "string",
      "description": "Filtrer par module"
    },
    "limit": {
      "type": "integer",
      "default": 50
    }
  },
  "required": ["query"]
}

OUTPUT :
{
  "query": "lcd_*",
  "results": [
    {
      "name": "lcd_init",
      "kind": "function",
      "file": "src/lcd/lcd_init.c",
      "signature": "int lcd_init(LCD_Config*)",
      "line": 45
    },
    {
      "name": "lcd_write",
      "kind": "function", 
      "file": "src/lcd/lcd_write.c",
      "signature": "int lcd_write(uint8_t*, size_t)",
      "line": 23
    }
  ],
  "total": 8,
  "returned": 2
}
```

### Outil 9 : get_file_metrics

```
NOM : get_file_metrics

DESCRIPTION :
Récupère les métriques détaillées d'un fichier.

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "path": {
      "type": "string",
      "description": "Chemin du fichier"
    }
  },
  "required": ["path"]
}

OUTPUT :
{
  "file": "src/lcd/lcd_init.c",
  "size": {
    "lines_total": 245,
    "lines_code": 180,
    "lines_comment": 45,
    "lines_blank": 20,
    "bytes": 8432
  },
  "complexity": {
    "cyclomatic_total": 45,
    "cyclomatic_avg": 8.5,
    "cyclomatic_max": 15,
    "cognitive_total": 38,
    "nesting_max": 4
  },
  "structure": {
    "functions": 5,
    "types": 1,
    "macros": 3,
    "variables": 2
  },
  "quality": {
    "documentation_score": 72,
    "has_tests": true,
    "technical_debt_score": 25
  },
  "activity": {
    "commits_30d": 3,
    "commits_90d": 8,
    "commits_365d": 24,
    "contributors": ["alice", "bob", "charlie"],
    "last_modified": "2024-11-28",
    "age_days": 412
  }
}
```

### Outil 10 : get_module_summary

```
NOM : get_module_summary

DESCRIPTION :
Récupère un résumé complet d'un module (ensemble de fichiers).

INPUT SCHEMA :
{
  "type": "object",
  "properties": {
    "module": {
      "type": "string",
      "description": "Nom du module"
    }
  },
  "required": ["module"]
}

OUTPUT :
{
  "module": "lcd",
  "files": {
    "total": 8,
    "sources": 5,
    "headers": 2,
    "tests": 1,
    "critical": 1
  },
  "symbols": {
    "functions": 23,
    "types": 5,
    "macros": 12
  },
  "metrics": {
    "lines_total": 1245,
    "complexity_avg": 9.2,
    "documentation_score": 68
  },
  "health": {
    "errors_last_90d": 2,
    "test_coverage": "partial",
    "technical_debt": "medium"
  },
  "patterns": ["error_handling_lcd", "naming_lcd"],
  "adrs": ["ADR-007", "ADR-012"],
  "dependencies": {
    "depends_on": ["hardware", "memory"],
    "depended_by": ["app", "test"]
  }
}
```

## 7.3 Configuration MCP

Le serveur MCP doit être configuré dans `.claude/settings.json` :

```json
{
  "mcpServers": {
    "agentdb": {
      "command": "python",
      "args": [
        "-m",
        "agentdb.mcp_server"
      ],
      "cwd": "${workspaceFolder}/.claude",
      "env": {
        "AGENTDB_PATH": "${workspaceFolder}/.claude/agentdb/db.sqlite",
        "AGENTDB_LOG_LEVEL": "INFO"
      }
    }
  }
}
```

---

# PARTIE 8 : LE BOOTSTRAP

## 8.1 Vue d'Ensemble

Le bootstrap est le processus d'initialisation d'AgentDB sur un projet existant.

```
BOOTSTRAP PIPELINE
──────────────────

┌─────────────────┐
│  1. STRUCTURE   │  Créer les dossiers et fichiers
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  2. SCHEMA      │  Créer les tables SQLite
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  3. SCAN        │  Parcourir tous les fichiers
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  4. INDEX       │  Indexer symboles et relations
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  5. METRICS     │  Calculer les métriques
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  6. ACTIVITY    │  Analyser l'historique Git
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  7. CRITICALITY │  Marquer les fichiers critiques
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  8. PATTERNS    │  Importer les patterns initiaux
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  9. VERIFY      │  Vérifier l'intégrité
└─────────────────┘
```

## 8.2 Étapes Détaillées

### Étape 1 : Créer la Structure

```
CRÉER :

.claude/
├── agentdb/
│   ├── db.sqlite           (sera créé)
│   └── schema.sql          (à partir du schéma fourni)
├── mcp/
│   └── agentdb/
│       ├── __init__.py
│       ├── server.py       (serveur MCP)
│       ├── tools.py        (implémentation des outils)
│       └── queries.py      (requêtes SQL)
├── config/
│   └── agentdb.yaml        (configuration)
└── logs/
    └── agentdb.log         (logs)
```

### Étape 2 : Créer le Schéma

```
EXÉCUTER :

sqlite3 .claude/agentdb/db.sqlite < .claude/agentdb/schema.sql

VÉRIFIER :

- Toutes les tables créées
- Tous les index créés
- Toutes les vues créées
- Métadonnées initialisées
```

### Étape 3 : Scanner les Fichiers

```
POUR CHAQUE FICHIER DU PROJET :

1. Vérifier s'il doit être indexé :
   - Pas dans les exclusions (build/, vendor/, node_modules/, etc.)
   - Extension reconnue (.c, .h, .py, .js, etc.)

2. Extraire les métadonnées :
   - Chemin, nom, extension
   - Module (déduit du chemin)
   - Langage (déduit de l'extension)

3. Calculer les métriques de base :
   - Lignes (total, code, commentaires, blancs)
   - Hash du contenu

4. Insérer dans la table `files`
```

### Étape 4 : Indexer les Symboles

```
POUR CHAQUE FICHIER SOURCE :

1. Parser le fichier selon le langage :
   - C/C++ : utiliser ctags, ou tree-sitter, ou clang
   - Python : utiliser ast
   - JavaScript : utiliser tree-sitter ou babel

2. Extraire les symboles :
   - Fonctions : nom, signature, paramètres, ligne
   - Types : structs, classes, enums, typedefs
   - Variables globales
   - Macros

3. Insérer dans la table `symbols`

4. Extraire les relations :
   - Appels de fonction
   - Inclusions/imports
   - Utilisations de types
   - Héritages

5. Insérer dans la table `relations`
```

### Étape 5 : Calculer les Métriques

```
POUR CHAQUE FICHIER :

1. Complexité cyclomatique :
   - Par fonction
   - Total, moyenne, max

2. Profondeur d'imbrication

3. Score de documentation :
   - % de fonctions documentées
   - Qualité des commentaires

4. Mettre à jour `files` et `symbols`
```

### Étape 6 : Analyser l'Activité Git

```
POUR CHAQUE FICHIER :

1. Compter les commits :
   - 30 derniers jours
   - 90 derniers jours
   - 365 derniers jours

2. Identifier les contributeurs :
   - Nom, email
   - Nombre de commits

3. Date de dernière modification

4. Mettre à jour `files`

COMMANDES GIT :

git log --oneline --since="30 days ago" -- <file> | wc -l
git log --format="%an|%ae" -- <file> | sort | uniq -c
git log -1 --format="%ai" -- <file>
```

### Étape 7 : Marquer les Fichiers Critiques

```
RÈGLES PAR DÉFAUT :

1. Patterns de chemin :
   - */security/* → critique
   - */auth/* → critique
   - */crypto/* → critique
   - */api/* → haute importance
   - */core/* → haute importance

2. Patterns de nom :
   - *password* → security_sensitive
   - *secret* → security_sensitive
   - *key* → security_sensitive
   - *token* → security_sensitive

3. Analyse du contenu :
   - Contient des fonctions crypto → security_sensitive
   - Contient des accès DB → importante

4. Mettre à jour `files.is_critical` et `files.security_sensitive`
```

### Étape 8 : Importer les Patterns Initiaux

```
PATTERNS PAR DÉFAUT À CRÉER :

1. error_handling :
   - Vérifier les retours de malloc
   - Vérifier les retours de fopen
   - Ne pas ignorer les codes d'erreur

2. memory_safety :
   - Préférer strncpy à strcpy
   - Vérifier les bounds des tableaux
   - Free après malloc

3. naming :
   - Fonctions en snake_case (C)
   - Préfixe par module

4. documentation :
   - Fonctions publiques documentées
   - Paramètres décrits

INSÉRER DANS `patterns`
```

### Étape 9 : Vérifier l'Intégrité

```
VÉRIFICATIONS :

1. Intégrité référentielle :
   - Toutes les FK valides
   - Pas d'orphelins

2. Cohérence :
   - Tous les fichiers ont des symboles (sauf headers vides)
   - Toutes les relations ont source et target

3. Complétude :
   - Nombre de fichiers indexés vs fichiers du projet
   - % de symboles avec documentation

4. Performance :
   - Test des requêtes principales
   - Temps de réponse < 100ms

RAPPORT :

{
  "status": "success",
  "files_indexed": 245,
  "symbols_indexed": 1823,
  "relations_indexed": 4521,
  "errors": [],
  "warnings": ["12 files skipped (binary)"],
  "duration_seconds": 45
}
```

## 8.3 Configuration du Bootstrap

Fichier `.claude/config/agentdb.yaml` :

```yaml
# Configuration AgentDB

project:
  name: "MonProjet"
  language: "c"  # Langage principal
  root: "."

indexing:
  # Extensions à indexer
  extensions:
    c: [".c", ".h"]
    cpp: [".cpp", ".hpp", ".cc", ".hh"]
    python: [".py"]
    javascript: [".js", ".jsx", ".ts", ".tsx"]
  
  # Patterns à exclure
  exclude:
    - "build/**"
    - "dist/**"
    - "vendor/**"
    - "node_modules/**"
    - "**/*.min.js"
    - "**/*.generated.*"
    - ".git/**"
    - ".claude/agentdb/**"

  # Outils d'indexation par langage
  tools:
    c: "ctags"      # ou "clang", "tree-sitter"
    cpp: "ctags"
    python: "ast"
    javascript: "tree-sitter"

criticality:
  # Patterns de chemins critiques
  critical_paths:
    - "src/security/**"
    - "src/auth/**"
    - "src/crypto/**"
    - "**/password*"
    - "**/secret*"
  
  # Patterns haute importance
  high_importance_paths:
    - "src/core/**"
    - "src/api/**"
    - "src/main.*"

metrics:
  # Seuils de complexité
  complexity:
    warning: 10
    error: 20
  
  # Seuils de documentation
  documentation:
    minimum_score: 50

git:
  # Périodes d'analyse d'activité
  activity_periods:
    - 30
    - 90
    - 365
```

---

# PARTIE 9 : LA MAINTENANCE

## 9.1 Mise à Jour Incrémentale

Après le bootstrap initial, AgentDB doit être mis à jour à chaque changement.

```
ÉVÉNEMENT : Nouveau commit

1. IDENTIFIER les fichiers modifiés :
   git diff --name-only HEAD~1

2. POUR CHAQUE fichier modifié :
   a. Supprimer les anciens symboles et relations
   b. Réindexer le fichier
   c. Recalculer les métriques

3. METTRE À JOUR les métriques d'activité :
   - Incrémenter commits_30d, etc.
   - Mettre à jour last_modified

DURÉE CIBLE : < 5 secondes pour un commit typique
```

## 9.2 Nettoyage et Optimisation

```
TÂCHES PÉRIODIQUES (quotidien ou hebdomadaire) :

1. VACUUM :
   - Récupérer l'espace des lignes supprimées
   - sqlite3 db.sqlite "VACUUM;"

2. ANALYZE :
   - Mettre à jour les statistiques pour l'optimiseur
   - sqlite3 db.sqlite "ANALYZE;"

3. INTÉGRITÉ :
   - Vérifier les FK
   - sqlite3 db.sqlite "PRAGMA integrity_check;"

4. ROTATION des snapshots :
   - Supprimer les snapshots > 30 jours
   - DELETE FROM snapshot_symbols WHERE created_at < date('now', '-30 days');

5. ARCHIVAGE des vieux runs :
   - Garder les 100 derniers runs détaillés
   - Archiver le reste (garder juste les métriques)
```

## 9.3 Sauvegarde et Restauration

```
SAUVEGARDE :

1. Copier le fichier SQLite :
   cp .claude/agentdb/db.sqlite .claude/agentdb/db.sqlite.backup

2. Ou export SQL :
   sqlite3 db.sqlite ".dump" > backup.sql

RESTAURATION :

1. Depuis fichier :
   cp db.sqlite.backup db.sqlite

2. Depuis SQL :
   sqlite3 db.sqlite < backup.sql
```

---

# PARTIE 10 : INSTRUCTIONS D'IMPLÉMENTATION

## 10.1 Ordre d'Implémentation Recommandé

```
PHASE 1 : FONDATIONS (Jour 1-2)
───────────────────────────────
□ Créer la structure de dossiers
□ Créer schema.sql complet
□ Créer le module Python de base (connexion DB, init)
□ Implémenter les CRUD basiques (files, symbols, relations)
□ Tests unitaires des CRUD

PHASE 2 : INDEXATION (Jour 3-4)
───────────────────────────────
□ Implémenter le parser de fichiers (ctags ou autre)
□ Extraire les symboles
□ Extraire les relations
□ Calculer les métriques basiques
□ Tests d'indexation sur fichiers exemples

PHASE 3 : REQUÊTES GRAPHE (Jour 5-6)
────────────────────────────────────
□ Implémenter get_symbol_callers (récursif)
□ Implémenter get_symbol_callees (récursif)
□ Implémenter get_file_impact
□ Optimiser avec les bons index
□ Tests de performance

PHASE 4 : SERVEUR MCP (Jour 7-8)
────────────────────────────────
□ Structure du serveur MCP
□ Implémenter chaque outil (10 outils)
□ Tests de chaque outil
□ Configuration dans settings.json
□ Test d'intégration avec Claude

PHASE 5 : BOOTSTRAP (Jour 9-10)
───────────────────────────────
□ Script de bootstrap complet
□ Analyse Git (activité)
□ Détection criticité
□ Import patterns initiaux
□ Vérification d'intégrité
□ Tests sur projet réel

PHASE 6 : POLISH (Jour 11-12)
─────────────────────────────
□ Logging complet
□ Gestion d'erreurs robuste
□ Documentation
□ Command /bootstrap
□ Hook post-commit pour mise à jour
```

## 10.2 Points d'Attention

```
PERFORMANCE
───────────
• Index SQLite bien placés (voir schéma)
• Requêtes récursives avec limite de profondeur
• Cache des résultats fréquents
• WAL mode pour concurrence

ROBUSTESSE
──────────
• Transactions pour les opérations groupées
• Gestion des fichiers binaires (skip)
• Gestion des encodages (UTF-8 principalement)
• Timeout sur les opérations longues

MAINTENABILITÉ
──────────────
• Code modulaire et testé
• Logs structurés
• Configuration externalisée
• Versioning du schéma

SÉCURITÉ
────────
• Pas de secrets dans la DB
• Validation des inputs
• Requêtes paramétrées (pas de SQL injection)
```

## 10.3 Fichiers à Créer

```
.claude/
├── agentdb/
│   ├── __init__.py
│   ├── schema.sql                 # Schéma complet
│   ├── db.py                      # Connexion et helpers DB
│   ├── models.py                  # Dataclasses/Types
│   ├── crud.py                    # Opérations CRUD
│   ├── queries.py                 # Requêtes complexes (graphe)
│   └── indexer.py                 # Indexation du code
│
├── mcp/
│   └── agentdb/
│       ├── __init__.py
│       ├── server.py              # Serveur MCP principal
│       └── tools.py               # Implémentation des 10 outils
│
├── scripts/
│   ├── bootstrap.py               # Script de bootstrap
│   ├── update.py                  # Mise à jour incrémentale
│   └── maintenance.py             # Tâches de maintenance
│
├── config/
│   └── agentdb.yaml               # Configuration
│
└── tests/
    ├── test_crud.py
    ├── test_queries.py
    ├── test_indexer.py
    └── test_mcp_tools.py
```

---

# RÉSUMÉ EXÉCUTIF

AgentDB est composé de :

**4 PILIERS DE DONNÉES :**
1. **Graphe** : fichiers, symboles, relations (qui appelle qui)
2. **Mémoire** : historique des erreurs, runs du pipeline
3. **Connaissance** : patterns, conventions, décisions architecturales
4. **Métriques** : complexité, activité, qualité

**10 OUTILS MCP :**
1. get_file_context
2. get_symbol_callers
3. get_symbol_callees
4. get_file_impact
5. get_error_history
6. get_patterns
7. get_architecture_decisions
8. search_symbols
9. get_file_metrics
10. get_module_summary

**BÉNÉFICES POUR LES AGENTS :**
- **Analyzer** : Calcule l'impact via le graphe
- **Security** : Détecte les régressions via l'historique
- **Reviewer** : Vérifie les patterns via la connaissance
- **Risk** : Évalue le risque via les métriques + historique

---

**C'EST LE FONDEMENT DU SYSTÈME. IMPLÉMENTE-LE AVEC SOIN.**