# :material-file-document-edit: Templates de Rapports

<div class="templates-header" markdown>

!!! abstract "Modèles Standardisés"

    Cette section contient les templates officiels pour la génération de rapports.
    Chaque agent utilise ces modèles pour garantir la cohérence et le professionnalisme.

</div>

---

## :material-shield-star: Template Rapport Général

```markdown
# :material-shield-star: Rapport du Général

<div class="report-header" markdown>

!!! info "Informations du Rapport"

    | Attribut | Valeur |
    |:---------|:-------|
    | **Date** | YYYY-MM-DD |
    | **Type** | [Type d'audit] |
    | **Statut** | :material-[icon]:{ .status-[state] } [Statut] |
    | **Score Global** | **X/10** |

</div>

---

## :material-account-supervisor: État des Forces

[Grille des commandants avec leurs états]

---

## :material-alert-octagon: Alertes Critiques

[Liste des alertes critiques]

---

## :material-chart-bar: Résumé par Axe

[Analyse détaillée par domaine]

---

## :material-clipboard-list: Plan d'Action Prioritaire

### Phase 1 - URGENCE
### Phase 2 - BLOCKERS
### Phase 3 - QUALITÉ
### Phase 4 - GIT

---

## :material-gavel: Recommandation Stratégique

[Verdict et priorisation]

---

<div class="report-footer" markdown>

:material-shield-star: **Rapport généré par le Général - Army2077**

</div>
```

---

## :material-shield-lock: Template Rapport Commandant

```markdown
# :material-[icon]: Rapport du Commandant [Domaine]

<div class="report-header" markdown>

!!! [type] "[Titre]"

    | Attribut | Valeur |
    |:---------|:-------|
    | **Date** | YYYY-MM-DD |
    | **Score** | **X/10** |
    | **Métrique Clé** | [Valeur] |
    | **Statut** | :material-[icon]:{ .status-[state] } [Statut] |

</div>

---

## :material-chart-donut: Vue d'Ensemble

[Graphiques et métriques visuelles]

---

## :material-check-circle: Points Forts

[Liste des aspects positifs]

---

## :material-close-circle: Points Faibles

[Liste des problèmes détectés]

---

## :material-clipboard-check: Plan d'Action

[Actions recommandées par priorité]

---

<div class="report-footer" markdown>

:material-[icon]: **Rapport généré par le Commandant [Domaine] - Army2077**

</div>
```

---

## :material-account-group: Template Rapport Soldat

```markdown
# :material-[icon]: Rapport du Soldat [Spécialité]

<div class="report-header" markdown>

!!! info "[Titre]"

    | Attribut | Valeur |
    |:---------|:-------|
    | **Date** | YYYY-MM-DD |
    | **Éléments Analysés** | [Nombre] |
    | **Issues** | [Nombre] |

</div>

---

## :material-magnify: Analyse

[Contenu de l'analyse]

---

## :material-lightbulb: Recommandations

[Suggestions d'amélioration]

---

<div class="report-footer" markdown>

:material-[icon]: **Rapport généré par le Soldat [Spécialité] - Army2077**

</div>
```

---

## :material-palette: Classes CSS Disponibles

### Statuts

| Classe | Couleur | Usage |
|:-------|:--------|:------|
| `.status-ok` | :material-check-circle:{ style="color: #4caf50" } Vert | Excellent/OK |
| `.status-warning` | :material-alert-circle:{ style="color: #ff9800" } Orange | Attention |
| `.status-critical` | :material-close-circle:{ style="color: #f44336" } Rouge | Critique |
| `.status-info` | :material-information:{ style="color: #2196f3" } Bleu | Information |

### Admonitions

```markdown
!!! success "Titre"     # Vert
!!! warning "Titre"     # Orange
!!! danger "Titre"      # Rouge
!!! info "Titre"        # Bleu
!!! tip "Titre"         # Cyan
!!! abstract "Titre"    # Violet
```

### Icônes Material

```markdown
:material-shield-star:      # Général
:material-shield-lock:      # Sécurité
:material-test-tube:        # Tests
:material-book-open-variant: # Documentation
:material-source-branch:    # Git
:material-clipboard-check:  # Qualité
:material-sitemap:          # Architecture
:material-code-review:      # Code Review
:material-school:           # Mentor
```

---

[:material-arrow-left: Retour au Dashboard](../index.md){ .md-button }
