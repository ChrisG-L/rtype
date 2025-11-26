# Référence Rapide - Commandes et Exemples

Guide de référence rapide pour utiliser efficacement les agents d'Army2077.

---

## Cheat Sheet - Commandes Essentielles

### Par Type de Tâche

| Tâche | Agent | Commande |
|-------|-------|----------|
| Orchestration complexe | Général | `@general_agent "..."` |
| Documentation | Cmd Documentation | `@commander_documentation "..."` |
| Commits Git | Cmd Git | `@commander_git "..."` |
| Tests / Qualité | Cmd Qualité | `@commander_quality "..."` |
| Sécurité / CI | Cmd Sécurité | `@commander_security "..."` |
| Architecture | Sol. Architecte | `@soldier_architecture "..."` |
| Code Review | Sol. Review | `@soldier_code_review "..."` |
| Apprentissage | Sol. Mentor | `@soldier_learning_advisor "..."` |

---

## Commandes par Scénario

### Feature Development

```bash
# 1. Conception architecture
@soldier_architecture "Propose une architecture pour [FEATURE]"

# 2. Review du code
@soldier_code_review "Review mon code dans [FICHIERS]"

# 3. Documentation
@commander_documentation "Documente [FEATURE]"

# 4. Commits
@commander_git "Propose des commits atomiques"

# OU - Tout en un via le Général
@general_agent "Feature complète: [DESCRIPTION]"
```

### Bug Fix

```bash
# Analyse et fix
@commander_quality "Bug: [DESCRIPTION], Fichier: [FICHIER]"

# Test de régression
@commander_quality "Crée test de régression pour [BUG]"

# Commit du fix
@commander_git "Commit fix pour [BUG]"
```

### Code Review

```bash
# Review rapide
@soldier_code_review "Review [FICHIER]"

# Review complète avec architecture
@soldier_code_review "Review complète [MODULE] avec validation SOLID"
```

### Documentation

```bash
# Documentation API
@commander_documentation "Doc API pour [MODULE]"

# Guide utilisateur
@commander_documentation "Guide pour [FEATURE]"

# Documentation architecture
@commander_documentation "Doc architecture [SYSTÈME] avec diagrammes"
```

### Sécurité

```bash
# Audit complet
@commander_security "Audit sécurité [MODULE]"

# Audit réseau
@commander_security "Audit code réseau"

# Vérification CI/CD
@commander_security "Vérifie pipeline Jenkins"
```

### Git

```bash
# Analyser changements
@commander_git "Analyse mes changements"

# Proposer commits
@commander_git "Propose commits atomiques pour [CONTEXT]"

# Préparer PR
@commander_git "Prépare PR pour [FEATURE]"
```

### Apprentissage

```bash
# Explication concept
@soldier_learning_advisor "Explique [CONCEPT]"

# Analyse pédagogique code
@soldier_learning_advisor "Analyse mon code [FICHIER] et guide-moi"

# Recommandations d'amélioration
@soldier_learning_advisor "Comment améliorer [ASPECT] dans mon code?"
```

---

## Templates de Commandes

### Template Feature Complète

```bash
@general_agent "Feature: [NOM_FEATURE]

Contexte:
- [Description du besoin]
- [Contraintes techniques]

Code dans:
- [Liste des fichiers]

Actions demandées:
1. Validation architecture
2. Vérification tests (>80%)
3. Audit sécurité
4. Documentation complète
5. Commits atomiques

Attentes:
- Rapport détaillé par domaine
- Actions concrètes recommandées"
```

### Template Bug Fix

```bash
@commander_quality "Bug Report:

Description: [Description du bug]
Reproduction: [Étapes pour reproduire]
Comportement attendu: [Ce qui devrait se passer]
Comportement actuel: [Ce qui se passe]

Fichier concerné: [Chemin]
Ligne approximative: [Numéro]

Logs/Erreur:
```
[Coller les logs ici]
```

Actions demandées:
1. Identifier la root cause
2. Proposer un fix
3. Créer test de régression"
```

### Template Code Review

```bash
@soldier_code_review "Code Review Request:

Module: [Nom du module]
Fichiers: [Liste des fichiers]

Contexte:
- [Ce que fait le code]
- [Patterns utilisés]

Points d'attention:
- [Points spécifiques à vérifier]

Critères de review:
- [ ] Clean Code
- [ ] SOLID
- [ ] Performance
- [ ] Error handling
- [ ] Tests
- [ ] Documentation"
```

### Template Documentation

```bash
@commander_documentation "Documentation Request:

Sujet: [Nom du module/feature]
Type: [API / Guide / Architecture / Reference]
Audience: [Débutant / Développeur / Architecte]

Contenu requis:
1. [Section 1]
2. [Section 2]
3. [Section 3]

Inclure:
- [ ] Diagrammes Mermaid
- [ ] Exemples de code
- [ ] Screenshots
- [ ] FAQ"
```

---

## Raccourcis et Patterns

### Patterns Courants

| Pattern | Description | Exemple |
|---------|-------------|---------|
| `[FICHIER]` | Chemin vers un fichier | `src/ecs/World.cpp` |
| `[MODULE]` | Nom d'un module | `ECS`, `Network`, `Graphics` |
| `[FEATURE]` | Nom d'une feature | `Système de particules` |
| `[CONCEPT]` | Concept technique | `SOLID`, `ECS`, `RAII` |

### Modificateurs

| Modificateur | Effet |
|--------------|-------|
| `avec détails` | Réponse plus détaillée |
| `rapidement` | Réponse concise |
| `pour débutant` | Explication simplifiée |
| `avec exemples` | Inclure des exemples de code |
| `avec diagrammes` | Inclure des diagrammes Mermaid |

### Exemples avec Modificateurs

```bash
# Explication simplifiée
@soldier_learning_advisor "Explique le pattern Observer pour débutant"

# Review avec focus performance
@soldier_code_review "Review [FICHIER] avec focus performance"

# Documentation avec diagrammes
@commander_documentation "Doc [MODULE] avec diagrammes détaillés"
```

---

## Flux de Décision Rapide

```mermaid
flowchart TD
    START([Quelle est ma tâche?])

    START --> Q1{Code à écrire?}
    Q1 -->|Non| Q2{Question?}
    Q1 -->|Oui| Q3{Complexité?}

    Q2 -->|Apprentissage| LEARN[@soldier_learning_advisor]
    Q2 -->|Architecture| ARCH[@soldier_architecture]

    Q3 -->|Simple| DIRECT[Agent direct approprié]
    Q3 -->|Complexe| GENERAL[@general_agent]

    DIRECT --> Q4{Quel domaine?}

    Q4 -->|Tests| QUAL[@commander_quality]
    Q4 -->|Sécurité| SEC[@commander_security]
    Q4 -->|Documentation| DOC[@commander_documentation]
    Q4 -->|Git| GIT[@commander_git]
    Q4 -->|Review| REV[@soldier_code_review]

    style START fill:#E3F2FD
    style GENERAL fill:#FFD700
    style LEARN fill:#228B22,color:#fff
    style ARCH fill:#228B22,color:#fff
    style REV fill:#228B22,color:#fff
    style QUAL fill:#4169E1,color:#fff
    style SEC fill:#4169E1,color:#fff
    style DOC fill:#4169E1,color:#fff
    style GIT fill:#4169E1,color:#fff
```

---

## Matrice Agent x Tâche

| Tâche | GEN | CMD_DOC | CMD_GIT | CMD_QUA | CMD_SEC | SOL_ARCH | SOL_REV | SOL_LEARN |
|-------|:---:|:-------:|:-------:|:-------:|:-------:|:--------:|:-------:|:---------:|
| Feature end-to-end | **X** | | | | | | | |
| Documentation | | **X** | | | | | | |
| Commits | | | **X** | | | | | |
| Tests unitaires | | | | **X** | | | | |
| Audit sécurité | | | | | **X** | | | |
| Design patterns | | | | | | **X** | | |
| Code review | | | | | | | **X** | |
| Explication concept | | | | | | | | **X** |
| Bug fix | | | | **X** | | | | |
| Refactoring | | | | **X** | | **X** | | |
| Architecture | | | | | | **X** | | |
| CI/CD | | | | | **X** | | | |

---

## Codes de Couleur (Standards)

| Couleur | Signification |
|---------|---------------|
| Or/Jaune | Général (État-Major) |
| Bleu | Commandants |
| Vert | Soldats |
| Rouge | Critique / Bloquant |
| Orange | Important / Warning |
| Gris | Informationnel |

---

## FAQ Rapide

??? question "Quel agent utiliser pour une tâche simple?"
    Utilisez directement l'agent spécialisé correspondant au domaine.
    Exemple: `@commander_git` pour les commits.

??? question "Quand utiliser le Général?"
    Pour les tâches complexes impliquant plusieurs domaines ou nécessitant une coordination.

??? question "Un agent peut-il en appeler un autre?"
    Oui, les Commandants peuvent faire appel aux Soldats de leur domaine.
    Le Général peut coordonner tous les agents.

??? question "Comment obtenir plus de détails?"
    Ajoutez `avec détails` ou `détaillé` à votre demande.

??? question "Les agents modifient-ils directement le code?"
    Ils proposent des modifications. Vous validez avant application.
    Exception: Le Soldat Mentor ne modifie JAMAIS le code.

---

## Liens Utiles

- [Introduction à la Ruche](introduction.md)
- [Architecture Détaillée](architecture.md)
- [Workflows Complets](workflows.md)
- [Documentation des Agents](agents/)
