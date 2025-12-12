# Rapport d'Analyse - Exemple Golden

> Ce fichier est un exemple de référence ("golden") montrant le format attendu
> pour un rapport complet généré par `/analyze`. Utilisez-le pour valider
> que vos rapports sont conformes.

---

**Date** : 2025-12-11
**Commit** : `abc1234` - "feat: Add UDP packet retry mechanism"
**Branche** : `feature/udp-retry` → `main`
**Fichiers analysés** : 3

---

## Verdict : 🟠 CAREFUL

**Score global** : 62/100

**Résumé** : Modification du serveur UDP avec une vulnérabilité HIGH détectée.
1 fichier critique impacté, tests manquants pour les nouvelles fonctionnalités.
Temps de correction estimé : ~45 minutes.

---

## Données AgentDB Utilisées

| Agent | file_context | symbol_callers | error_history | patterns | file_metrics |
|-------|--------------|----------------|---------------|----------|--------------|
| Analyzer | ✅ | ✅ | - | - | ✅ |
| Security | ✅ | ✅ | ✅ | ✅ | - |
| Reviewer | ✅ | - | - | ✅ | ✅ |
| Risk | ✅ | - | ✅ | - | ✅ |

**Légende** : ✅ = utilisé avec données, ⚠️ = utilisé mais vide, ❌ = non utilisé, - = non applicable

---

## Résumé par Agent

| Agent | Score | Issues | Bloquants | Status |
|-------|-------|--------|-----------|--------|
| 🔒 Security | 55/100 | 3 | 1 | 🟠 |
| 📋 Reviewer | 72/100 | 4 | 1 | 🟡 |
| ⚠️ Risk | 58/100 | 2 | 0 | 🟠 |
| 🔍 Analyzer | 65/100 | 2 | 1 | 🟡 |
| **📊 Global** | **62/100** | **11** | **3** | **🟠** |

### Calcul du Score Global

```
Security  : 55 × 0.35 = 19.25
Risk      : 58 × 0.25 = 14.50
Reviewer  : 72 × 0.25 = 18.00
Analyzer  : 65 × 0.15 =  9.75
                       ──────
Sous-total            = 61.50
Pénalité (bloquants)  = -10
Pénalité (1 fichier critique) = -5
                       ──────
SCORE BRUT            = 46.50

Ajustement : +15 (pas de régression, pas de vuln CRITICAL)
                       ──────
SCORE FINAL           = 62/100
```

---

## Contradictions Détectées

| # | Type | Agents | Détail | Résolution |
|---|------|--------|--------|------------|
| 1 | Score divergent | Security (55) vs Reviewer (72) | Écart de 17 points | Prioriser Security |
| 2 | Sévérité | Security=HIGH, Risk=MEDIUM | Désaccord sur criticité | Appliquer HIGH |

---

## Issues Critiques

### 🔴 BLOQUANTES (3)

#### 1. [HIGH] SEC-001 - Buffer Overflow potentiel (CWE-120)

- **Source** : 🔒 Security
- **Fichier** : `src/server/UDPServer.cpp:67`
- **Fonction** : `processRequest()`
- **Temps** : ~5 min

**Code actuel** :
```cpp
void processRequest(const char* user_data) {
    char response_buffer[256];
    strcpy(response_buffer, user_data);  // DANGER: No bounds check
}
```

**Correction suggérée** :
```cpp
void processRequest(const char* user_data) {
    char response_buffer[256];
    strncpy(response_buffer, user_data, sizeof(response_buffer) - 1);
    response_buffer[sizeof(response_buffer) - 1] = '\0';
}
```

---

#### 2. [HIGH] ANA-001 - Changement de signature à fort impact

- **Source** : 🔍 Analyzer
- **Fichier** : `src/server/UDPServer.cpp:42`
- **Symbole** : `sendPacket()`
- **Temps** : ~30 min

**Impact** : 8 appelants doivent être mis à jour

```
sendPacket (src/server/UDPServer.cpp:42) [MODIFIED]
├── [L1] handleConnection (src/server/TCPServer.cpp:120)
│   └── [L2] main (src/main.cpp:45)
├── [L1] processRequest (src/handler/RequestHandler.cpp:89)
│   └── [L2] APIServer::handle (src/api/Server.cpp:156) ⚠️ CRITICAL
└── [L1] NetworkManager::broadcast (src/net/Manager.cpp:234)
```

---

#### 3. [ERROR] REV-001 - Fonction trop complexe

- **Source** : 📋 Reviewer
- **Fichier** : `src/server/UDPServer.cpp:145`
- **Fonction** : `processMultipleRequests()`
- **Complexité** : 25 (seuil=20)
- **Temps** : ~20 min

**Action** : Refactorer en sous-fonctions

---

### 🟠 IMPORTANTES (4)

#### 4. [WARNING] REV-002 - Magic number

- **Source** : 📋 Reviewer
- **Fichier** : `src/server/UDPServer.cpp:78`
- **Temps** : ~2 min

```cpp
// Avant
if (buffer.size() > 65535) { ... }

// Après
constexpr size_t MAX_UDP_PAYLOAD = 65535;
if (buffer.size() > MAX_UDP_PAYLOAD) { ... }
```

---

#### 5. [WARNING] REV-003 - ADR-007 violé

- **Source** : 📋 Reviewer
- **Fichier** : `src/server/UDPServer.cpp:92`
- **ADR** : "Use error codes over exceptions"
- **Temps** : ~10 min

---

#### 6. [MEDIUM] RISK-001 - Fichier critique sans tests

- **Source** : ⚠️ Risk
- **Fichier** : `src/server/UDPServer.cpp`
- **Temps** : ~120 min

---

#### 7. [MEDIUM] SEC-002 - Retour non vérifié (CWE-252)

- **Source** : 🔒 Security
- **Fichier** : `src/server/UDPServer.cpp:89`
- **Temps** : ~5 min

---

### 🟡 MINEURES (4)

| # | ID | Source | Fichier | Description |
|---|-----|--------|---------|-------------|
| 8 | REV-004 | Reviewer | UDPServer.cpp:120 | Fonction non documentée |
| 9 | REV-005 | Reviewer | Config.hpp:15 | Naming convention |
| 10 | ANA-002 | Analyzer | Server.cpp:156 | Fichier critique impacté |
| 11 | RISK-002 | Risk | UDPServer.cpp | Complexité élevée |

---

## Actions Requises

### Avant merge (BLOQUANT) :

- [ ] **SEC-001** : Corriger buffer overflow dans UDPServer.cpp:67
- [ ] **ANA-001** : Mettre à jour les 8 appelants de sendPacket
- [ ] **REV-001** : Refactorer processMultipleRequests

### Recommandé :

- [ ] **RISK-001** : Ajouter tests pour UDPServer.cpp
- [ ] **REV-003** : Respecter ADR-007 (error codes)
- [ ] **SEC-002** : Vérifier retour de send()

### Optionnel :

- [ ] **REV-002** : Extraire magic number en constante
- [ ] **REV-004** : Ajouter documentation Doxygen

---

## Temps Estimés

| Catégorie | Temps |
|-----------|-------|
| Bloquants | ~45 min |
| Recommandé | ~2h30 |
| Optionnel | ~15 min |
| **Total** | **~3h30** |

---

## Fichiers Analysés

| Fichier | +/- | Issues | Critique | Tests |
|---------|-----|--------|----------|-------|
| src/server/UDPServer.cpp | +145 -23 | 6 | ✅ Oui | ❌ Non |
| src/utils/Shell.cpp | +12 -3 | 1 | ❌ Non | ✅ Oui |
| src/core/Config.hpp | +3 -1 | 0 | ❌ Non | N/A |

---

## Métriques Comparatives

| Métrique | Ce commit | Moyenne projet | Delta |
|----------|-----------|----------------|-------|
| Score global | 62 | 75 | -13 ⚠️ |
| Issues bloquantes | 3 | 0.5 | +2.5 ⚠️ |
| Fichiers critiques touchés | 1 | 0.3 | +0.7 ⚠️ |
| Temps correction estimé | 45 min | 15 min | ×3 ⚠️ |

---

## Recommandation Finale

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  🟠 NE PAS MERGER EN L'ÉTAT                                     │
│                                                                 │
│  Actions requises avant merge :                                 │
│  1. Corriger les 3 issues bloquantes (~45 min)                  │
│  2. Faire review par senior (fichier critique touché)           │
│  3. Relancer /analyze après corrections                         │
│                                                                 │
│  Prochain reviewer suggéré : @senior-dev (expertise sécurité)   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## JSON Output (CI/CD Integration)

```json
{
  "synthesis": {
    "verdict": "CAREFUL",
    "global_score": 62,
    "timestamp": "2025-12-11T14:32:00Z",
    "commit": "abc1234",
    "branch": "feature/udp-retry"
  },
  "scores": {
    "security": 55,
    "reviewer": 72,
    "risk": 58,
    "analyzer": 65,
    "global": 62
  },
  "weights": {
    "security": 0.35,
    "risk": 0.25,
    "reviewer": 0.25,
    "analyzer": 0.15
  },
  "issues": {
    "total": 11,
    "blocking": 3,
    "by_severity": {
      "HIGH": 2,
      "ERROR": 1,
      "MEDIUM": 2,
      "WARNING": 2,
      "INFO": 4
    }
  },
  "contradictions": [
    {
      "type": "score_divergence",
      "agents": ["security", "reviewer"],
      "delta": 17
    }
  ],
  "time_estimates": {
    "blocking_fixes_min": 45,
    "recommended_fixes_min": 150,
    "total_min": 210
  },
  "files_analyzed": 3,
  "critical_files_touched": 1,
  "regressions_detected": 0,
  "merge_ready": false,
  "actions_required": [
    {
      "id": "SEC-001",
      "priority": 1,
      "blocking": true,
      "description": "Fix buffer overflow"
    },
    {
      "id": "ANA-001",
      "priority": 2,
      "blocking": true,
      "description": "Update 8 callers of sendPacket"
    },
    {
      "id": "REV-001",
      "priority": 3,
      "blocking": true,
      "description": "Refactor complex function"
    }
  ]
}
```

---

## Rapports Individuels

Les rapports détaillés de chaque agent sont disponibles dans ce dossier :

- `analyzer.md` - Analyse d'impact
- `security.md` - Audit de sécurité
- `reviewer.md` - Code review
- `risk.md` - Évaluation des risques

---

*Rapport généré par la suite d'agents Claude Code*
*Configuration : .claude/config/agentdb.yaml*
