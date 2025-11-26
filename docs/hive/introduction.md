# Army2077 - Système d'Assistance Militaire IA

## Vue d'Ensemble

**Army2077** est un système d'assistance au développement de niveau professionnel, basé sur **Claude Code**, utilisant une **hiérarchie militaire** pour maximiser l'efficacité, la qualité et la cohérence du projet R-Type.

!!! info "Philosophie"
    Inspirée des structures militaires d'élite, Army2077 utilise une chaîne de commandement claire où chaque agent a un rôle précis, des responsabilités définies et des standards stricts à respecter.

---

## Architecture Hiérarchique

```mermaid
graph TB
    subgraph "ÉTAT-MAJOR"
        GEN["<b>GÉNÉRAL</b><br/>Orchestrateur Suprême<br/><i>general_agent.md</i>"]
    end

    subgraph "COMMANDANTS"
        CMD_DOC["<b>COMMANDANT DOCS</b><br/>Documentation<br/><i>commander_documentation.md</i>"]
        CMD_GIT["<b>COMMANDANT GIT</b><br/>Versioning & Commits<br/><i>commander_git.md</i>"]
        CMD_QUA["<b>COMMANDANT QUALITÉ</b><br/>Tests & Standards<br/><i>commander_quality.md</i>"]
        CMD_SEC["<b>COMMANDANT SÉCURITÉ</b><br/>Sécurité & CI/CD<br/><i>commander_security.md</i>"]
    end

    subgraph "SOLDATS SPÉCIALISÉS"
        SOL_ARCH["<b>SOLDAT ARCHITECTE</b><br/>Conception & Design<br/><i>soldier_architecture.md</i>"]
        SOL_REV["<b>SOLDAT REVIEW</b><br/>Code Review<br/><i>soldier_code_review.md</i>"]
        SOL_LEARN["<b>SOLDAT MENTOR</b><br/>Apprentissage<br/><i>soldier_learning_advisor.md</i>"]
    end

    GEN --> CMD_DOC
    GEN --> CMD_GIT
    GEN --> CMD_QUA
    GEN --> CMD_SEC

    CMD_QUA --> SOL_ARCH
    CMD_QUA --> SOL_REV
    CMD_DOC --> SOL_LEARN

    style GEN fill:#FFD700,stroke:#B8860B,stroke-width:3px,color:#000
    style CMD_DOC fill:#4169E1,stroke:#00008B,stroke-width:2px,color:#fff
    style CMD_GIT fill:#4169E1,stroke:#00008B,stroke-width:2px,color:#fff
    style CMD_QUA fill:#4169E1,stroke:#00008B,stroke-width:2px,color:#fff
    style CMD_SEC fill:#4169E1,stroke:#00008B,stroke-width:2px,color:#fff
    style SOL_ARCH fill:#228B22,stroke:#006400,stroke-width:2px,color:#fff
    style SOL_REV fill:#228B22,stroke:#006400,stroke-width:2px,color:#fff
    style SOL_LEARN fill:#228B22,stroke:#006400,stroke-width:2px,color:#fff
```

---

## Les Grades et Leurs Rôles

### Général - Orchestrateur Suprême

| Attribut | Valeur |
|----------|--------|
| **Fichier** | `.claude/agents/general_agent.md` |
| **Rang** | État-Major |
| **Mission** | Coordination globale et prise de décision stratégique |
| **Autorité** | Peut invoquer et coordonner tous les commandants et soldats |

**Responsabilités clés :**

- Analyser la demande utilisateur et déterminer la stratégie optimale
- Déléguer aux agents appropriés selon la complexité
- Assurer la cohérence globale du travail produit
- Prendre les décisions finales en cas de conflit

---

### Commandants - Experts de Domaine

Les commandants sont des **experts spécialisés** dans leur domaine respectif.

<div class="grid cards" markdown>

-   :material-file-document:{ .lg .middle } **Commandant Documentation**

    ---

    Expert en documentation technique MkDocs Material.

    - Documentation professionnelle et complète
    - Diagrammes Mermaid
    - Exemples de code fonctionnels

    [:octicons-arrow-right-24: Détails](agents/commander-documentation.md)

-   :material-source-commit:{ .lg .middle } **Commandant Git**

    ---

    Expert en versioning et gestion de commits.

    - Commits atomiques et conventionnels
    - Historique Git propre
    - Pull Requests structurées

    [:octicons-arrow-right-24: Détails](agents/commander-git.md)

-   :material-test-tube:{ .lg .middle } **Commandant Qualité**

    ---

    Expert en tests et qualité de code.

    - Tests unitaires Google Test
    - Couverture de code
    - Standards C++23

    [:octicons-arrow-right-24: Détails](agents/commander-quality.md)

-   :material-shield-lock:{ .lg .middle } **Commandant Sécurité**

    ---

    Expert en sécurité et CI/CD.

    - Audit de vulnérabilités
    - Pipeline Jenkins
    - SonarCloud

    [:octicons-arrow-right-24: Détails](agents/commander-security.md)

</div>

---

### Soldats - Spécialistes Techniques

Les soldats sont des **spécialistes pointus** dans des domaines techniques précis.

<div class="grid cards" markdown>

-   :material-sitemap:{ .lg .middle } **Soldat Architecte**

    ---

    Spécialiste en conception logicielle.

    - Patterns de conception
    - Architecture hexagonale
    - SOLID et Clean Code

    [:octicons-arrow-right-24: Détails](agents/soldier-architecture.md)

-   :material-magnify:{ .lg .middle } **Soldat Code Review**

    ---

    Spécialiste en revue de code.

    - Analyse critique constructive
    - Détection de code smells
    - Suggestions d'amélioration

    [:octicons-arrow-right-24: Détails](agents/soldier-code-review.md)

-   :material-school:{ .lg .middle } **Soldat Mentor**

    ---

    Conseiller d'apprentissage technique.

    - Explication pédagogique
    - Guide sans modifier le code
    - Ressources d'apprentissage

    [:octicons-arrow-right-24: Détails](agents/soldier-learning-advisor.md)

</div>

---

## Comment Invoquer les Agents

### Méthode Directe

Utilisez la syntaxe `@nom_du_fichier` dans Claude Code :

```bash
# Invoquer le Général pour une tâche complexe
@general_agent "Analyse mon projet et propose un plan d'amélioration"

# Invoquer un Commandant spécifique
@commander_git "Propose des commits atomiques pour mes changements"

# Invoquer un Soldat pour une tâche spécialisée
@soldier_learning_advisor "Explique-moi le pattern Observer"
```

### Via le Général (Recommandé)

Pour les tâches complexes, passez par le Général qui coordonnera les agents :

```bash
@general_agent "J'ai terminé le système de particules,
                assure-toi que tout est parfait (code, tests, docs, commits)"
```

Le Général va :

1. Analyser la demande
2. Invoquer le **Commandant Qualité** pour les tests
3. Invoquer le **Commandant Sécurité** pour l'audit
4. Invoquer le **Commandant Documentation** pour la doc
5. Invoquer le **Commandant Git** pour les commits
6. Synthétiser et valider le tout

---

## Flux de Décision

```mermaid
flowchart TD
    START([Demande Utilisateur]) --> ANALYZE{Type de demande?}

    ANALYZE -->|Simple & Unique| DIRECT[Invocation Directe]
    ANALYZE -->|Complexe / Multi-domaine| GENERAL[Via le Général]

    DIRECT --> EXEC_DIRECT[Agent exécute]

    GENERAL --> STRAT[Analyse stratégique]
    STRAT --> DELEGATE[Délégation aux Commandants]
    DELEGATE --> CMD1[Commandant 1]
    DELEGATE --> CMD2[Commandant 2]
    DELEGATE --> CMDN[Commandant N]

    CMD1 --> SOL1{Besoin soldat?}
    SOL1 -->|Oui| SOLDIER1[Soldat spécialisé]
    SOL1 -->|Non| EXEC1[Exécution directe]

    CMD2 --> EXEC2[Exécution directe]
    CMDN --> EXECN[Exécution directe]

    SOLDIER1 --> EXEC1

    EXEC1 --> MERGE[Synthèse Général]
    EXEC2 --> MERGE
    EXECN --> MERGE
    EXEC_DIRECT --> RESULT

    MERGE --> VALIDATE{Validation OK?}
    VALIDATE -->|Oui| RESULT([Résultat Final])
    VALIDATE -->|Non| ITERATE[Itération / Correction]
    ITERATE --> DELEGATE

    style START fill:#E8F5E9,stroke:#4CAF50
    style RESULT fill:#E8F5E9,stroke:#4CAF50
    style GENERAL fill:#FFD700,stroke:#B8860B
    style MERGE fill:#FFD700,stroke:#B8860B
    style CMD1 fill:#4169E1,stroke:#00008B,color:#fff
    style CMD2 fill:#4169E1,stroke:#00008B,color:#fff
    style CMDN fill:#4169E1,stroke:#00008B,color:#fff
    style SOLDIER1 fill:#228B22,stroke:#006400,color:#fff
```

---

## Principes Fondamentaux

### Standards Communs à Tous les Agents

| Critère | Standard |
|---------|----------|
| **Langue** | Français (code en anglais) |
| **Format commits** | Conventional Commits (FEAT, FIX, DOCS...) |
| **Tests** | Couverture >= 80% |
| **Documentation** | Complète avec exemples |
| **Code** | C++23, RAII, Modern C++ |
| **Sécurité** | 0 vulnérabilité critique |

### Règles d'Engagement

1. **Clarté** : Chaque agent explique ses actions et décisions
2. **Transparence** : Les agents signalent ce qu'ils ne peuvent pas faire
3. **Qualité** : Aucun compromis sur les standards
4. **Respect** : Les agents ne modifient jamais sans accord explicite
5. **Apprentissage** : Les agents fournissent des explications pédagogiques

---

## Navigation

<div class="grid cards" markdown>

-   :material-map:{ .lg .middle } **Architecture Détaillée**

    ---

    Diagrammes complets et interactions entre agents.

    [:octicons-arrow-right-24: architecture.md](architecture.md)

-   :material-cog:{ .lg .middle } **Workflows**

    ---

    Processus et workflows pour chaque type de tâche.

    [:octicons-arrow-right-24: workflows.md](workflows.md)

-   :material-lightning-bolt:{ .lg .middle } **Référence Rapide**

    ---

    Commandes et exemples d'utilisation.

    [:octicons-arrow-right-24: quick-reference.md](quick-reference.md)

-   :material-account-group:{ .lg .middle } **Agents Détaillés**

    ---

    Documentation complète de chaque agent.

    [:octicons-arrow-right-24: agents/](agents/)

</div>

---

## Prochaines Étapes

1. **Découvrez l'architecture** : [architecture.md](architecture.md)
2. **Apprenez les workflows** : [workflows.md](workflows.md)
3. **Consultez la référence rapide** : [quick-reference.md](quick-reference.md)
4. **Explorez chaque agent** : [agents/](agents/)

---

!!! success "Bienvenue dans Army2077"
    Le système d'agents IA est conçu pour vous assister, pas vous remplacer. Vous restez le décideur final. Les agents sont là pour augmenter votre productivité et garantir la qualité du projet.
