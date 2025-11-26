# Documentation des Agents

Cette section contient la documentation détaillée de chaque agent d'Army2077.

---

## Hiérarchie des Agents

```mermaid
graph TB
    subgraph "ÉTAT-MAJOR"
        GEN["GÉNÉRAL<br/>general_agent.md"]
    end

    subgraph "COMMANDEMENT"
        DOC["Cmd Documentation"]
        GIT["Cmd Git"]
        QUA["Cmd Qualité"]
        SEC["Cmd Sécurité"]
        TEST["Cmd Tests"]
    end

    subgraph "EXÉCUTION"
        ARCH["Sol. Architecte"]
        REV["Sol. Code Review"]
        LEARN["Sol. Mentor"]
    end

    GEN --> DOC
    GEN --> GIT
    GEN --> QUA
    GEN --> SEC
    GEN --> TEST

    DOC --> LEARN
    QUA --> ARCH
    QUA --> REV
    TEST --> QUA

    click GEN "general.md"
    click DOC "commander-documentation.md"
    click GIT "commander-git.md"
    click QUA "commander-quality.md"
    click SEC "commander-security.md"
    click TEST "commander-tests.md"
    click ARCH "soldier-architecture.md"
    click REV "soldier-code-review.md"
    click LEARN "soldier-learning-advisor.md"

    style GEN fill:#FFD700,stroke:#B8860B
    style DOC fill:#4169E1,color:#fff
    style GIT fill:#4169E1,color:#fff
    style QUA fill:#4169E1,color:#fff
    style SEC fill:#4169E1,color:#fff
    style TEST fill:#4169E1,color:#fff
    style ARCH fill:#228B22,color:#fff
    style REV fill:#228B22,color:#fff
    style LEARN fill:#228B22,color:#fff
```

---

## État-Major

<div class="grid cards" markdown>

-   :material-crown:{ .lg .middle } **Général - Orchestrateur**

    ---

    Agent suprême coordonnant l'ensemble de la Ruche.

    [:octicons-arrow-right-24: Documentation](general.md)

</div>

---

## Commandants

<div class="grid cards" markdown>

-   :material-file-document-multiple:{ .lg .middle } **Commandant Documentation**

    ---

    Expert en documentation technique MkDocs Material.

    [:octicons-arrow-right-24: Documentation](commander-documentation.md)

-   :material-source-branch:{ .lg .middle } **Commandant Git**

    ---

    Expert en versioning et gestion des commits.

    [:octicons-arrow-right-24: Documentation](commander-git.md)

-   :material-test-tube:{ .lg .middle } **Commandant Qualité**

    ---

    Expert en tests et standards de qualité.

    [:octicons-arrow-right-24: Documentation](commander-quality.md)

-   :material-shield-check:{ .lg .middle } **Commandant Sécurité**

    ---

    Expert en sécurité et CI/CD.

    [:octicons-arrow-right-24: Documentation](commander-security.md)

-   :material-test-tube-empty:{ .lg .middle } **Commandant Tests**

    ---

    Expert en tests unitaires et couverture (GTest).

    [:octicons-arrow-right-24: Documentation](commander-tests.md)

</div>

---

## Soldats Spécialisés

<div class="grid cards" markdown>

-   :material-sitemap:{ .lg .middle } **Soldat Architecte**

    ---

    Spécialiste en conception et design patterns.

    [:octicons-arrow-right-24: Documentation](soldier-architecture.md)

-   :material-magnify-scan:{ .lg .middle } **Soldat Code Review**

    ---

    Spécialiste en revue de code critique.

    [:octicons-arrow-right-24: Documentation](soldier-code-review.md)

-   :material-school:{ .lg .middle } **Soldat Mentor**

    ---

    Conseiller d'apprentissage technique.

    [:octicons-arrow-right-24: Documentation](soldier-learning-advisor.md)

</div>

---

## Tableau Comparatif

| Agent | Fichier | Domaine | Autonomie |
|-------|---------|---------|-----------|
| **Général** | `general_agent.md` | Coordination | Haute |
| **Cmd Documentation** | `commander_documentation.md` | Documentation | Haute |
| **Cmd Git** | `commander_git.md` | Versioning | Haute |
| **Cmd Qualité** | `commander_quality.md` | Tests/Qualité | Moyenne |
| **Cmd Sécurité** | `commander_security.md` | Sécurité/CI | Moyenne |
| **Cmd Tests** | `commander_tests.md` | Tests/Couverture | Haute |
| **Sol. Architecte** | `soldier_architecture.md` | Architecture | Moyenne |
| **Sol. Code Review** | `soldier_code_review.md` | Review | Moyenne |
| **Sol. Mentor** | `soldier_learning_advisor.md` | Apprentissage | Moyenne |

---

## Navigation

- [Introduction à la Ruche](../introduction.md)
- [Architecture](../architecture.md)
- [Workflows](../workflows.md)
- [Référence Rapide](../quick-reference.md)
