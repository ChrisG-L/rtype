# Workflows Army2077 - Processus et Procédures

Ce guide détaille les workflows optimaux pour chaque type de tâche avec Army2077.

---

## Workflow 1 : Développement d'une Feature

### Vue d'Ensemble

```mermaid
flowchart TB
    subgraph "Phase 1: Conception"
        A1[Analyser le besoin] --> A2[Concevoir l'architecture]
        A2 --> A3[Valider avec Sol. Architecte]
    end

    subgraph "Phase 2: Implémentation"
        B1[Développer le code] --> B2[Écrire les tests]
        B2 --> B3[Review avec Sol. Review]
    end

    subgraph "Phase 3: Documentation"
        C1[Documenter l'API] --> C2[Créer les guides]
        C2 --> C3[Valider avec Cmd Docs]
    end

    subgraph "Phase 4: Finalisation"
        D1[Audit sécurité] --> D2[Commits atomiques]
        D2 --> D3[Validation Général]
    end

    A3 --> B1
    B3 --> C1
    C3 --> D1

    style A1 fill:#E3F2FD
    style D3 fill:#E8F5E9
```

### Étapes Détaillées

=== "Phase 1: Conception"

    **Objectif** : Définir une architecture solide avant de coder.

    ```bash
    # Demander une analyse architecturale
    @soldier_architecture "J'ai besoin d'implémenter [FEATURE].
                           Voici les requirements: [REQUIREMENTS].
                           Propose une architecture."
    ```

    **Output attendu** :
    - Diagramme d'architecture proposé
    - Patterns recommandés
    - Points d'attention

=== "Phase 2: Implémentation"

    **Objectif** : Code de qualité avec tests complets.

    ```bash
    # Après avoir codé, demander une review
    @soldier_code_review "Review mon implémentation de [FEATURE]
                          dans les fichiers: [FICHIERS]"
    ```

    **Output attendu** :
    - Code smells identifiés
    - Suggestions d'amélioration
    - Points positifs

=== "Phase 3: Documentation"

    **Objectif** : Documentation complète et professionnelle.

    ```bash
    # Demander la documentation
    @commander_documentation "Documente la feature [FEATURE].
                              Inclus: API, exemples, diagrammes."
    ```

    **Output attendu** :
    - Page de documentation MkDocs
    - Exemples de code
    - Diagrammes Mermaid

=== "Phase 4: Finalisation"

    **Objectif** : Commits propres et validation finale.

    ```bash
    # Audit sécurité
    @commander_security "Audite le code de [FEATURE]"

    # Puis commits
    @commander_git "Propose des commits atomiques pour [FEATURE]"
    ```

    **Output attendu** :
    - Rapport de sécurité
    - Commits proposés au format Conventional

---

## Workflow 2 : Bug Fix

### Vue d'Ensemble

```mermaid
flowchart LR
    A[Bug reporté] --> B{Gravité?}
    B -->|Critique| C[Fix immédiat]
    B -->|Normal| D[Analyse root cause]

    C --> E[Test de régression]
    D --> F[Fix + Test régression]

    E --> G[Review rapide]
    F --> G

    G --> H[Commit fix]
    H --> I[Validation]

    style A fill:#FFCDD2
    style I fill:#E8F5E9
```

### Procédure Standard

```bash
# 1. Analyser le bug avec le Commandant Qualité
@commander_quality "Analyse ce bug: [DESCRIPTION]
                    Erreur: [ERROR_MESSAGE]
                    Fichier: [FICHIER]"

# 2. Après le fix, créer le test de régression
@commander_quality "Crée un test de régression pour le bug [BUG_ID]"

# 3. Review du fix
@soldier_code_review "Review mon fix pour [BUG_ID] dans [FICHIER]"

# 4. Commit
@commander_git "Commit le fix pour [BUG_ID]"
```

### Template de Commit Bug Fix

```
FIX(<scope>): correction [description courte]

Problème:
- [Description du bug]
- [Impact sur les utilisateurs]

Solution:
- [Description de la solution]
- [Fichiers modifiés]

Tests:
- [Tests ajoutés/modifiés]

Closes #[BUG_ID]
```

---

## Workflow 3 : Code Review Complet

### Diagramme de Processus

```mermaid
sequenceDiagram
    autonumber
    participant DEV as Développeur
    participant SR as Sol. Review
    participant SA as Sol. Architecte
    participant CQ as Cmd Qualité
    participant CS as Cmd Sécurité

    DEV->>SR: Demande review code
    activate SR

    SR->>SR: Analyse statique
    SR->>SA: Demande validation architecture
    activate SA
    SA-->>SR: Rapport architecture
    deactivate SA

    SR->>CQ: Demande validation tests
    activate CQ
    CQ-->>SR: Rapport couverture
    deactivate CQ

    SR->>CS: Demande audit sécurité
    activate CS
    CS-->>SR: Rapport sécurité
    deactivate CS

    SR-->>DEV: Rapport complet consolidé
    deactivate SR
```

### Checklist de Review

```bash
# Review complète avec le Soldat Review
@soldier_code_review "Review complète de [MODULE/FEATURE].
                       Fichiers: [LISTE_FICHIERS]

                       Vérifie:
                       - [ ] Clean Code
                       - [ ] SOLID principles
                       - [ ] Performance
                       - [ ] Error handling
                       - [ ] Edge cases"
```

### Critères d'Évaluation

| Critère | Poids | Description |
|---------|-------|-------------|
| **Lisibilité** | 20% | Code clair, nommage explicite |
| **Architecture** | 20% | Respect des patterns, découplage |
| **Tests** | 20% | Couverture, qualité des tests |
| **Performance** | 15% | Complexité algorithmique, ressources |
| **Sécurité** | 15% | Vulnérabilités, validation inputs |
| **Documentation** | 10% | Commentaires, docstrings |

---

## Workflow 4 : Apprentissage Guidé

### Diagramme Pédagogique

```mermaid
flowchart TB
    subgraph "CYCLE D'APPRENTISSAGE"
        direction TB
        A[Identifier le sujet] --> B[Expliquer le concept]
        B --> C[Montrer un exemple]
        C --> D[Guider la pratique]
        D --> E{Compris?}
        E -->|Non| F[Reformuler]
        F --> B
        E -->|Oui| G[Exercice avancé]
        G --> H[Validation]
    end

    style A fill:#E3F2FD
    style H fill:#E8F5E9
```

### Utilisation du Soldat Mentor

```bash
# Demander une explication pédagogique
@soldier_learning_advisor "Explique-moi [CONCEPT] dans le contexte de notre projet.

                           Mon niveau: [débutant/intermédiaire/avancé]

                           Ce que je veux comprendre:
                           - [Point 1]
                           - [Point 2]"
```

### Format de Réponse Pédagogique

Le Soldat Mentor structure ses réponses ainsi :

1. **Concept** : Explication théorique simple
2. **Pourquoi** : Justification dans le contexte
3. **Exemple** : Code illustratif (dans la doc, pas dans les sources)
4. **Exercice** : Suggestion de pratique
5. **Ressources** : Liens pour approfondir

!!! warning "Important"
    Le Soldat Mentor **n'écrit JAMAIS de code** dans les fichiers sources. Il guide, explique, et documente uniquement.

---

## Workflow 5 : Sécurité et Audit

### Pipeline d'Audit

```mermaid
flowchart TD
    subgraph "PHASE 1: Analyse Statique"
        A1[Scan dépendances] --> A2[Analyse code]
        A2 --> A3[Détection vulnérabilités]
    end

    subgraph "PHASE 2: Audit Manuel"
        B1[Review inputs validation] --> B2[Review auth/authz]
        B2 --> B3[Review crypto]
        B3 --> B4[Review network]
    end

    subgraph "PHASE 3: Rapport"
        C1[Synthèse findings] --> C2[Priorisation]
        C2 --> C3[Recommandations]
    end

    A3 --> B1
    B4 --> C1

    style A1 fill:#FFECB3
    style C3 fill:#E8F5E9
```

### Commandes d'Audit

```bash
# Audit complet
@commander_security "Audit de sécurité complet du module [MODULE].

                     Focus:
                     - Injection (SQL, Command, XSS)
                     - Buffer overflow
                     - Auth/Authz
                     - Crypto"

# Audit ciblé réseau
@commander_security "Audit du code réseau dans src/network/.

                     Vérifie:
                     - Validation des paquets entrants
                     - Rate limiting
                     - Timeouts
                     - TLS/SSL"
```

### Classification des Vulnérabilités

| Sévérité | Couleur | Action | Délai |
|----------|---------|--------|-------|
| **Critique** | Rouge | Fix immédiat | < 24h |
| **Haute** | Orange | Fix prioritaire | < 1 semaine |
| **Moyenne** | Jaune | Fix planifié | < 1 mois |
| **Basse** | Vert | Backlog | Quand possible |

---

## Workflow 6 : Documentation Complète

### Structure de Documentation

```mermaid
flowchart TB
    subgraph "TYPES DE DOC"
        API[Documentation API]
        GUIDE[Guides Utilisateur]
        ARCH[Documentation Architecture]
        REF[Référence Technique]
    end

    subgraph "PROCESS"
        P1[Identifier l'audience] --> P2[Structurer le contenu]
        P2 --> P3[Rédiger]
        P3 --> P4[Ajouter diagrammes]
        P4 --> P5[Ajouter exemples]
        P5 --> P6[Review]
    end

    API --> P1
    GUIDE --> P1
    ARCH --> P1
    REF --> P1

    style API fill:#E3F2FD
    style P6 fill:#E8F5E9
```

### Templates de Documentation

=== "API Documentation"

    ```bash
    @commander_documentation "Documente l'API de [MODULE].

                              Structure:
                              1. Vue d'ensemble
                              2. Installation/Setup
                              3. Référence des méthodes
                              4. Exemples d'utilisation
                              5. FAQ"
    ```

=== "Guide Utilisateur"

    ```bash
    @commander_documentation "Crée un guide utilisateur pour [FEATURE].

                              Public cible: [débutant/développeur/admin]

                              Inclure:
                              - Prérequis
                              - Étapes pas à pas
                              - Screenshots/diagrammes
                              - Troubleshooting"
    ```

=== "Documentation Architecture"

    ```bash
    @commander_documentation "Documente l'architecture de [SYSTÈME].

                              Inclure:
                              - Diagramme C4 (Context, Container, Component)
                              - Flux de données
                              - Décisions techniques
                              - ADRs (Architecture Decision Records)"
    ```

---

## Workflow 7 : Git et Versioning

### Flow de Commits

```mermaid
gitgraph
    commit id: "Initial"
    branch feature/ecs
    checkout feature/ecs
    commit id: "FEAT: add Entity class"
    commit id: "FEAT: add ComponentPool"
    commit id: "TEST: add Entity tests"
    checkout main
    merge feature/ecs id: "Merge: ECS System"
    branch fix/collision
    checkout fix/collision
    commit id: "FIX: collision detection"
    commit id: "TEST: regression test"
    checkout main
    merge fix/collision id: "Merge: Fix collision"
```

### Commandes Git avec les Agents

```bash
# Analyser les changements et proposer des commits
@commander_git "Analyse mes changements et propose des commits atomiques.

                Contexte: [CONTEXTE DE LA FEATURE]"

# Créer une PR
@commander_git "Prépare une PR pour [FEATURE].

                Branche source: [BRANCH]
                Branche cible: main

                Inclure:
                - Résumé des changements
                - Tests effectués
                - Screenshots si pertinent"
```

### Convention de Commits

| Type | Description | Exemple |
|------|-------------|---------|
| `FEAT` | Nouvelle fonctionnalité | `FEAT(ecs): add Entity class` |
| `FIX` | Correction de bug | `FIX(network): resolve timeout issue` |
| `DOCS` | Documentation | `DOCS(api): add auth endpoint docs` |
| `REFACTOR` | Refactoring | `REFACTOR(core): extract GameLoop` |
| `TEST` | Tests | `TEST(ecs): add Entity unit tests` |
| `BUILD` | Build/CI | `BUILD(cmake): add SFML linkage` |
| `PERF` | Performance | `PERF(render): optimize sprite batch` |
| `CHORE` | Maintenance | `CHORE: update dependencies` |

---

## Workflow 8 : Orchestration Complète (Via Général)

### Cas d'Usage : Feature End-to-End

```mermaid
flowchart TB
    subgraph "INPUT"
        REQ[/"@general_agent 'Feature ECS complète'"/]
    end

    subgraph "ANALYSE STRATÉGIQUE"
        GEN[Général analyse]
        GEN --> PLAN[Plan d'action]
    end

    subgraph "EXÉCUTION PARALLÈLE"
        direction LR
        ARCH[Sol. Architecte<br/>Design]
        QUAL[Cmd Qualité<br/>Tests]
        SEC[Cmd Sécurité<br/>Audit]
        DOC[Cmd Documentation<br/>Docs]
    end

    subgraph "FINALISATION"
        GIT[Cmd Git<br/>Commits]
        SYNTH[Général<br/>Synthèse]
    end

    subgraph "OUTPUT"
        RESULT[/"Rapport complet + Actions"/]
    end

    REQ --> GEN
    PLAN --> ARCH
    PLAN --> QUAL
    PLAN --> SEC
    PLAN --> DOC

    ARCH --> GIT
    QUAL --> GIT
    SEC --> GIT
    DOC --> GIT

    GIT --> SYNTH
    SYNTH --> RESULT

    style REQ fill:#E3F2FD
    style GEN fill:#FFD700
    style SYNTH fill:#FFD700
    style ARCH fill:#228B22,color:#fff
    style QUAL fill:#4169E1,color:#fff
    style SEC fill:#4169E1,color:#fff
    style DOC fill:#4169E1,color:#fff
    style GIT fill:#4169E1,color:#fff
    style RESULT fill:#E8F5E9
```

### Commande Type

```bash
@general_agent "J'ai terminé la feature [FEATURE].

                Code dans: [FICHIERS]

                Assure-toi que:
                1. L'architecture est solide
                2. Les tests sont complets (>80%)
                3. La sécurité est validée
                4. La documentation est complète
                5. Les commits sont atomiques et conventionnels

                Donne-moi un rapport complet avec les actions à effectuer."
```

### Exemple de Rapport du Général

```markdown
# RAPPORT DE MISSION - Feature ECS

## Résumé Exécutif
**Statut Global**: PRÊT POUR MERGE (avec recommandations)
**Score Qualité**: 87/100

---

## 1. Architecture (Sol. Architecte)
**Statut**: VALIDÉE

- Pattern ECS correctement implémenté
- Découplage respecté
- Suggestion: Extraire ComponentRegistry

## 2. Tests (Cmd Qualité)
**Statut**: VALIDÉS

- Couverture: 84%
- 12 tests unitaires
- 3 tests d'intégration
- Suggestion: Ajouter tests edge cases

## 3. Sécurité (Cmd Sécurité)
**Statut**: VALIDÉE

- Aucune vulnérabilité critique
- 1 warning mineur (bounds check)

## 4. Documentation (Cmd Documentation)
**Statut**: COMPLÈTE

- Guide créé: docs/guides/ecs.md
- API documentée
- 3 diagrammes Mermaid

## 5. Commits Proposés (Cmd Git)
1. `FEAT(ecs): add Entity and EntityManager`
2. `FEAT(ecs): add ComponentPool template`
3. `FEAT(ecs): add World facade`
4. `TEST(ecs): add comprehensive test suite`
5. `DOCS(ecs): add architecture guide`

---

## Actions Recommandées
1. [OPTIONNEL] Extraire ComponentRegistry
2. [OPTIONNEL] Ajouter 2 tests edge cases
3. [REQUIS] Corriger bounds check ligne 45
4. Valider les commits et merge
```

---

## Navigation

- [Retour à l'Introduction](introduction.md)
- [Architecture Détaillée](architecture.md)
- [Référence Rapide](quick-reference.md)
- [Agents Individuels](agents/)
