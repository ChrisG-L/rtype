# Général - Orchestrateur Suprême

## Carte d'Identité

| Attribut | Valeur |
|----------|--------|
| **Rang** | État-Major |
| **Fichier** | `.claude/agents/general_agent.md` |
| **Invocation** | `@general_agent` |
| **Domaine** | Coordination globale |
| **Autonomie** | Haute |

---

## Mission

Le **Général** est l'agent orchestrateur suprême d'Army2077. Sa mission est de :

1. **Analyser** les demandes complexes et déterminer la stratégie optimale
2. **Coordonner** les agents spécialisés d'Army2077 pour accomplir les tâches
3. **Synthétiser** les résultats en un rapport cohérent
4. **Valider** la qualité globale du travail produit

---

## Diagramme de Responsabilités

```mermaid
mindmap
  root((GÉNÉRAL))
    Analyse
      Comprendre la demande
      Évaluer la complexité
      Identifier les domaines
    Coordination
      Sélectionner les agents
      Définir l'ordre d'exécution
      Gérer les dépendances
    Synthèse
      Consolider les rapports
      Identifier les conflits
      Produire le rapport final
    Validation
      Vérifier la cohérence
      Valider les standards
      Approuver ou demander corrections
```

---

## Flux de Travail

```mermaid
sequenceDiagram
    autonumber
    participant U as Utilisateur
    participant G as Général
    participant C1 as Commandant 1
    participant C2 as Commandant 2
    participant S as Soldat

    U->>G: Demande complexe
    activate G

    Note over G: Phase Analyse

    G->>G: Analyse stratégique
    G->>G: Plan d'action

    Note over G: Phase Coordination

    G->>C1: Délégation tâche 1
    activate C1
    G->>C2: Délégation tâche 2
    activate C2

    C1->>S: Besoin expertise
    activate S
    S-->>C1: Rapport expert
    deactivate S

    C1-->>G: Résultat tâche 1
    deactivate C1
    C2-->>G: Résultat tâche 2
    deactivate C2

    Note over G: Phase Synthèse

    G->>G: Consolidation
    G->>G: Validation

    G-->>U: Rapport complet
    deactivate G
```

---

## Cas d'Utilisation

### Quand invoquer le Général ?

| Situation | Recommandé |
|-----------|:----------:|
| Feature complexe multi-domaines | Oui |
| Validation finale avant merge | Oui |
| Coordination tests + docs + commits | Oui |
| Simple bug fix | Non |
| Documentation seule | Non |
| Commits uniquement | Non |

### Exemples de Commandes

=== "Feature Complète"

    ```bash
    @general_agent "J'ai terminé le système ECS.

                    Code dans: src/ecs/

                    Assure-toi que:
                    - Architecture solide
                    - Tests complets (>80%)
                    - Sécurité validée
                    - Documentation complète
                    - Commits atomiques"
    ```

=== "Validation Avant Merge"

    ```bash
    @general_agent "Valide ma branche feature/particles avant merge.

                    Vérifie tous les aspects:
                    - Qualité du code
                    - Tests passent
                    - Documentation à jour
                    - Pas de vulnérabilités"
    ```

=== "Audit Global"

    ```bash
    @general_agent "Audit complet du module Network.

                    Focus sur:
                    - Architecture
                    - Performance
                    - Sécurité
                    - Maintenabilité"
    ```

---

## Format de Rapport

Le Général produit des rapports structurés :

```markdown
# RAPPORT DE MISSION - [Titre]

## Résumé Exécutif
**Statut Global**: [PRÊT/EN COURS/BLOQUÉ]
**Score Qualité**: [XX/100]

---

## 1. [Domaine 1] (Agent X)
**Statut**: [VALIDÉ/EN ATTENTE/ÉCHEC]

[Détails...]

## 2. [Domaine 2] (Agent Y)
**Statut**: [VALIDÉ/EN ATTENTE/ÉCHEC]

[Détails...]

---

## Actions Recommandées
1. [REQUIS] Action obligatoire
2. [OPTIONNEL] Suggestion d'amélioration

---

## Prochaines Étapes
- [ ] Étape 1
- [ ] Étape 2
```

---

## Interactions avec les Autres Agents

```mermaid
graph LR
    GEN((GÉNÉRAL))

    GEN -->|coordonne| DOC[Cmd Documentation]
    GEN -->|coordonne| GIT[Cmd Git]
    GEN -->|coordonne| QUA[Cmd Qualité]
    GEN -->|coordonne| SEC[Cmd Sécurité]

    GEN -.->|peut consulter| ARCH[Sol. Architecte]
    GEN -.->|peut consulter| REV[Sol. Review]

    style GEN fill:#FFD700,stroke:#B8860B,stroke-width:3px
    style DOC fill:#4169E1,color:#fff
    style GIT fill:#4169E1,color:#fff
    style QUA fill:#4169E1,color:#fff
    style SEC fill:#4169E1,color:#fff
    style ARCH fill:#228B22,color:#fff
    style REV fill:#228B22,color:#fff
```

---

## Principes Fondamentaux

1. **Vision Globale** : Le Général voit l'ensemble, pas juste les détails
2. **Délégation Intelligente** : Sait quand déléguer et à qui
3. **Qualité Sans Compromis** : N'approuve pas un travail insuffisant
4. **Communication Claire** : Rapports structurés et actionnables
5. **Efficacité** : Parallélise quand possible, séquentialise quand nécessaire

---

## Limitations

!!! warning "Ce que le Général ne fait PAS"
    - Écriture directe de code (délègue)
    - Modifications Git directes (délègue au Cmd Git)
    - Tests unitaires (délègue au Cmd Qualité)
    - Documentation technique (délègue au Cmd Documentation)

---

## Voir Aussi

- [Architecture de la Ruche](../architecture.md)
- [Workflows](../workflows.md)
- [Commandant Documentation](commander-documentation.md)
- [Commandant Qualité](commander-quality.md)
