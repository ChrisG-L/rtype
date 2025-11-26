# Architecture Army2077 - Conception Détaillée

## Vue d'Ensemble de l'Architecture

Army2077 utilise une architecture en couches inspirée de l'organisation militaire, avec des canaux de communication bien définis et des responsabilités clairement séparées.

---

## Diagramme d'Architecture Complet

```mermaid
graph TB
    subgraph "COUCHE INTERFACE"
        USER([Développeur])
        CLAUDE[Claude Code CLI]
    end

    subgraph "COUCHE ORCHESTRATION"
        GEN["GÉNÉRAL<br/>━━━━━━━━━━━━━━━<br/>• Analyse stratégique<br/>• Coordination globale<br/>• Décisions finales"]
    end

    subgraph "COUCHE COMMANDEMENT"
        direction LR
        CMD_DOC["COMMANDANT DOCS<br/>━━━━━━━━━━━━━━━<br/>• MkDocs Material<br/>• Diagrammes<br/>• Guides"]
        CMD_GIT["COMMANDANT GIT<br/>━━━━━━━━━━━━━━━<br/>• Commits atomiques<br/>• Conventions<br/>• PRs"]
        CMD_QUA["COMMANDANT QUALITÉ<br/>━━━━━━━━━━━━━━━<br/>• Google Test<br/>• Code review<br/>• Standards"]
        CMD_SEC["COMMANDANT SÉCURITÉ<br/>━━━━━━━━━━━━━━━<br/>• Audit vulnérabilités<br/>• Jenkins CI/CD<br/>• SonarCloud"]
    end

    subgraph "COUCHE EXÉCUTION"
        direction LR
        SOL_ARCH["SOLDAT ARCHITECTE<br/>━━━━━━━━━━━━━━━<br/>• Design patterns<br/>• SOLID<br/>• Clean Architecture"]
        SOL_REV["SOLDAT REVIEW<br/>━━━━━━━━━━━━━━━<br/>• Code smells<br/>• Refactoring<br/>• Best practices"]
        SOL_LEARN["SOLDAT MENTOR<br/>━━━━━━━━━━━━━━━<br/>• Pédagogie<br/>• Explications<br/>• Ressources"]
    end

    subgraph "COUCHE RESSOURCES"
        CODEBASE[(Codebase)]
        DOCS_FILES[(Documentation)]
        CONFIG[(Configuration)]
        TESTS[(Tests)]
    end

    USER --> CLAUDE
    CLAUDE --> GEN

    GEN --> CMD_DOC
    GEN --> CMD_GIT
    GEN --> CMD_QUA
    GEN --> CMD_SEC

    CMD_DOC --> SOL_LEARN
    CMD_QUA --> SOL_ARCH
    CMD_QUA --> SOL_REV

    CMD_DOC --> DOCS_FILES
    CMD_GIT --> CODEBASE
    CMD_QUA --> TESTS
    CMD_SEC --> CONFIG

    SOL_ARCH --> CODEBASE
    SOL_REV --> CODEBASE
    SOL_LEARN --> DOCS_FILES

    classDef user fill:#E1F5FE,stroke:#0288D1
    classDef general fill:#FFD700,stroke:#B8860B,stroke-width:3px
    classDef commander fill:#4169E1,stroke:#00008B,color:#fff
    classDef soldier fill:#228B22,stroke:#006400,color:#fff
    classDef resource fill:#F5F5F5,stroke:#9E9E9E

    class USER,CLAUDE user
    class GEN general
    class CMD_DOC,CMD_GIT,CMD_QUA,CMD_SEC commander
    class SOL_ARCH,SOL_REV,SOL_LEARN soldier
    class CODEBASE,DOCS_FILES,CONFIG,TESTS resource
```

---

## Communication Inter-Agents

### Modèle de Communication

```mermaid
sequenceDiagram
    autonumber
    participant U as Utilisateur
    participant G as Général
    participant CQ as Cmd Qualité
    participant SA as Soldat Architecte
    participant CG as Cmd Git

    U->>G: "Finalise ma feature ECS"

    Note over G: Analyse stratégique

    G->>CQ: Demande validation qualité
    activate CQ

    CQ->>SA: Demande review architecture
    activate SA
    SA-->>CQ: Rapport architecture OK
    deactivate SA

    CQ-->>G: Qualité validée
    deactivate CQ

    G->>CG: Demande commits atomiques
    activate CG
    CG-->>G: 3 commits proposés
    deactivate CG

    Note over G: Synthèse finale

    G-->>U: Rapport complet + actions proposées
```

---

## Matrice de Responsabilités (RACI)

```mermaid
graph LR
    subgraph "Légende RACI"
        R[R = Responsable]
        A[A = Approbateur]
        C[C = Consulté]
        I[I = Informé]
    end

    style R fill:#4CAF50,color:#fff
    style A fill:#2196F3,color:#fff
    style C fill:#FF9800,color:#fff
    style I fill:#9E9E9E,color:#fff
```

| Tâche | Général | Cmd Docs | Cmd Git | Cmd Qualité | Cmd Sécurité | Sol. Arch | Sol. Review | Sol. Mentor |
|-------|:-------:|:--------:|:-------:|:-----------:|:------------:|:---------:|:-----------:|:-----------:|
| **Stratégie globale** | R/A | I | I | I | I | - | - | - |
| **Documentation** | A | R | I | C | I | - | - | C |
| **Commits** | A | - | R | C | I | - | - | - |
| **Tests unitaires** | A | I | I | R | C | C | C | - |
| **Code review** | A | - | I | R | C | C | R | - |
| **Sécurité** | A | I | I | C | R | C | C | - |
| **Architecture** | A | I | I | R | C | R | C | I |
| **Apprentissage** | I | C | - | I | - | C | C | R |

---

## Zones de Responsabilité

```mermaid
pie showData
    title Répartition des Domaines d'Expertise
    "Code & Architecture" : 30
    "Tests & Qualité" : 25
    "Documentation" : 20
    "Sécurité & CI/CD" : 15
    "Versioning Git" : 10
```

---

## États et Transitions d'un Agent

```mermaid
stateDiagram-v2
    [*] --> Idle: Agent inactif
    Idle --> Analyzing: Réception demande
    Analyzing --> Planning: Analyse terminée
    Planning --> Executing: Plan validé
    Planning --> NeedsDelegate: Délégation requise
    NeedsDelegate --> WaitingSubAgent: Délégation envoyée
    WaitingSubAgent --> Merging: Résultat reçu
    Merging --> Executing: Synthèse OK
    Executing --> Validating: Exécution terminée
    Validating --> Reporting: Validation OK
    Validating --> Executing: Correction nécessaire
    Reporting --> [*]: Rapport envoyé

    note right of Analyzing
        Analyse de la demande
        et du contexte
    end note

    note right of NeedsDelegate
        Tâche trop complexe
        ou hors domaine
    end note

    note right of Merging
        Fusion des résultats
        des sous-agents
    end note
```

---

## Flux de Données

### Flux d'une Feature Complète

```mermaid
flowchart LR
    subgraph "ENTRÉE"
        REQ[Demande Feature]
        CODE[Code Écrit]
    end

    subgraph "ANALYSE"
        GEN_A[Général<br/>Analyse]
    end

    subgraph "PARALLÉLISATION"
        direction TB
        QUAL[Cmd Qualité<br/>Tests]
        SEC[Cmd Sécurité<br/>Audit]
        DOC[Cmd Documentation<br/>Docs]
    end

    subgraph "FINALISATION"
        GIT[Cmd Git<br/>Commits]
    end

    subgraph "SORTIE"
        OUTPUT[Feature Complète<br/>& Validée]
    end

    REQ --> GEN_A
    CODE --> GEN_A
    GEN_A --> QUAL
    GEN_A --> SEC
    GEN_A --> DOC
    QUAL --> GIT
    SEC --> GIT
    DOC --> GIT
    GIT --> OUTPUT

    style REQ fill:#E3F2FD
    style CODE fill:#E3F2FD
    style OUTPUT fill:#E8F5E9
    style GEN_A fill:#FFD700
    style QUAL fill:#4169E1,color:#fff
    style SEC fill:#4169E1,color:#fff
    style DOC fill:#4169E1,color:#fff
    style GIT fill:#4169E1,color:#fff
```

---

## Dépendances Entre Agents

```mermaid
graph TD
    subgraph "Dépendances Obligatoires"
        GEN2[Général] -->|coordonne| CMD_ALL[Tous Commandants]
        CMD_QUA2[Cmd Qualité] -->|utilise| SOL_REV2[Sol. Review]
        CMD_QUA2 -->|utilise| SOL_ARCH2[Sol. Architecte]
    end

    subgraph "Dépendances Optionnelles"
        CMD_DOC2[Cmd Docs] -.->|peut consulter| SOL_LEARN2[Sol. Mentor]
        CMD_SEC2[Cmd Sécurité] -.->|peut consulter| SOL_ARCH2
    end

    subgraph "Indépendants"
        CMD_GIT2[Cmd Git]
    end

    style GEN2 fill:#FFD700,stroke:#B8860B
    style CMD_ALL fill:#4169E1,stroke:#00008B,color:#fff
    style CMD_QUA2 fill:#4169E1,stroke:#00008B,color:#fff
    style CMD_DOC2 fill:#4169E1,stroke:#00008B,color:#fff
    style CMD_SEC2 fill:#4169E1,stroke:#00008B,color:#fff
    style CMD_GIT2 fill:#4169E1,stroke:#00008B,color:#fff
    style SOL_REV2 fill:#228B22,stroke:#006400,color:#fff
    style SOL_ARCH2 fill:#228B22,stroke:#006400,color:#fff
    style SOL_LEARN2 fill:#228B22,stroke:#006400,color:#fff
```

---

## Niveaux de Confiance et Autonomie

```mermaid
quadrantChart
    title Autonomie vs Spécialisation des Agents
    x-axis Généraliste --> Spécialiste
    y-axis Faible Autonomie --> Haute Autonomie
    quadrant-1 "Experts Autonomes"
    quadrant-2 "Stratèges"
    quadrant-3 "Assistants"
    quadrant-4 "Spécialistes Guidés"
    "Général": [0.3, 0.9]
    "Cmd Documentation": [0.7, 0.75]
    "Cmd Git": [0.8, 0.8]
    "Cmd Qualité": [0.6, 0.7]
    "Cmd Sécurité": [0.75, 0.65]
    "Sol. Architecte": [0.85, 0.5]
    "Sol. Review": [0.9, 0.55]
    "Sol. Mentor": [0.6, 0.4]
```

---

## Architecture des Fichiers Agents

```
.claude/agents/
├── README.md                      # Vue d'ensemble du système
│
├── general_agent.md               # État-Major
│   └── Orchestrateur suprême
│
├── commander_documentation.md     # Commandant
│   └── Expert Documentation
│
├── commander_git.md               # Commandant
│   └── Expert Versioning
│
├── commander_quality.md           # Commandant
│   └── Expert Qualité
│
├── commander_security.md          # Commandant
│   └── Expert Sécurité
│
├── soldier_architecture.md        # Soldat
│   └── Spécialiste Architecture
│
├── soldier_code_review.md         # Soldat
│   └── Spécialiste Review
│
└── soldier_learning_advisor.md    # Soldat
    └── Conseiller Apprentissage
```

---

## Conventions de Nommage

| Type | Pattern | Exemple |
|------|---------|---------|
| **Général** | `general_*.md` | `general_agent.md` |
| **Commandant** | `commander_*.md` | `commander_documentation.md` |
| **Soldat** | `soldier_*.md` | `soldier_architecture.md` |

---

## Extensibilité

### Ajouter un Nouveau Commandant

```mermaid
flowchart TD
    A[Identifier le domaine] --> B[Créer commander_domaine.md]
    B --> C[Définir les responsabilités]
    C --> D[Définir les standards]
    D --> E[Créer les templates de sortie]
    E --> F[Intégrer au Général]
    F --> G[Documenter dans docs/hive/agents/]

    style A fill:#E3F2FD
    style G fill:#E8F5E9
```

### Ajouter un Nouveau Soldat

```mermaid
flowchart TD
    A2[Identifier la spécialisation] --> B2[Créer soldier_specialisation.md]
    B2 --> C2[Définir le scope précis]
    C2 --> D2[Lier au Commandant parent]
    D2 --> E2[Définir les formats de rapport]
    E2 --> F2[Documenter dans docs/hive/agents/]

    style A2 fill:#E3F2FD
    style F2 fill:#E8F5E9
```

---

## Métriques et Monitoring

### KPIs par Agent

| Agent | Métrique Clé | Objectif |
|-------|--------------|----------|
| **Général** | Taux de succès coordination | > 95% |
| **Cmd Documentation** | Couverture documentation | > 90% |
| **Cmd Git** | Commits conventionnels | 100% |
| **Cmd Qualité** | Couverture tests | > 80% |
| **Cmd Sécurité** | Vulnérabilités critiques | 0 |
| **Sol. Architecte** | Respect SOLID | > 90% |
| **Sol. Review** | Issues détectées avant merge | > 85% |
| **Sol. Mentor** | Satisfaction pédagogique | > 4/5 |

---

## Voir Aussi

- [Introduction à la Ruche](introduction.md)
- [Workflows Détaillés](workflows.md)
- [Référence Rapide](quick-reference.md)
- [Agents Individuels](agents/)
