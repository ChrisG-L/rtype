# :material-account-tie: Rapports des Commandants

<div class="commanders-header" markdown>

!!! abstract "Officiers Supérieurs - Army2077"

    Les **Commandants** sont les officiers supérieurs responsables de domaines spécifiques.
    Chaque commandant produit des rapports détaillés sur son domaine d'expertise.

</div>

---

## :material-view-grid: Vue d'Ensemble

<div class="grid cards" markdown>

-   :material-book-open-variant:{ .lg .middle } **CMD Documentation**

    ---

    Supervise toute la documentation technique.

    État: :material-check-circle:{ .status-ok } **7.5/10**

    [:octicons-arrow-right-24: Rapport](documentation/latest.md)

-   :material-clipboard-check:{ .lg .middle } **CMD Qualité**

    ---

    Qualité du code et best practices.

    État: :material-alert-circle:{ .status-warning } **7/10**

    [:octicons-arrow-right-24: Rapport](quality/latest.md)

-   :material-shield-lock:{ .lg .middle } **CMD Sécurité**

    ---

    Sécurité et pipelines CI/CD.

    État: :material-close-circle:{ .status-critical } **3/10**

    [:octicons-arrow-right-24: Rapport](security/latest.md)

-   :material-test-tube:{ .lg .middle } **CMD Tests**

    ---

    Stratégie de tests et couverture.

    État: :material-alert-circle:{ .status-warning } **70%**

    [:octicons-arrow-right-24: Rapport](tests/latest.md)

-   :material-source-branch:{ .lg .middle } **CMD Git**

    ---

    Qualité de l'historique Git.

    État: :material-check-circle:{ .status-ok } **9/10**

    [:octicons-arrow-right-24: Rapport](git/latest.md)

</div>

---

## :material-chart-timeline: Synthèse des États

```mermaid
%%{init: {'theme': 'dark'}}%%
xychart-beta
    title "Scores des Commandants"
    x-axis [Documentation, Qualité, Sécurité, Tests, Git]
    y-axis "Score (/10)" 0 --> 10
    bar [7.5, 7, 3, 7, 9]
```

---

## :material-format-list-checks: Récapitulatif des Actions

| Commandant | Actions Critiques | Priorité |
|:-----------|:------------------|:--------:|
| :material-shield-lock: Sécurité | 5 vulnérabilités à corriger | :material-alert-decagram:{ .status-critical } P0 |
| :material-test-tube: Tests | Couvrir Application/Client | :material-alert:{ .status-warning } P1 |
| :material-book-open-variant: Documentation | Ajouter Docstrings | :material-information:{ .status-info } P2 |
| :material-clipboard-check: Qualité | Implémenter standards | :material-information:{ .status-info } P2 |
| :material-source-branch: Git | Maintenir qualité | :material-check:{ .status-ok } P3 |

---

[:material-arrow-left: Retour au Dashboard](../index.md){ .md-button }
