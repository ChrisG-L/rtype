# Politiques de Commit

**Dernière mise à jour:** 25 novembre 2025

Ce guide définit les conventions et bonnes pratiques pour les commits dans le projet R-Type.

---

## Format des Messages de Commit

### Structure Standard

```
TYPE: Description courte [TICKET-ID]

Description détaillée optionnelle expliquant le "pourquoi" du changement.
```

### Types de Commit

| Type | Description | Exemple |
|------|-------------|---------|
| `ADD` | Ajout d'une nouvelle fonctionnalité | `ADD: Système de logging client [EPI-36]` |
| `FIX` | Correction de bug | `FIX: Race condition dans TCPClient [EPI-37]` |
| `UPDATE` | Amélioration d'une fonctionnalité existante | `UPDATE: Optimisation AssetManager` |
| `REMOVE` | Suppression de code/fonctionnalité | `REMOVE: Logger obsolète` |
| `REFACTOR` | Restructuration sans changement de comportement | `REFACTOR: Séparation .hpp/.cpp` |
| `DOCS` | Documentation uniquement | `DOCS: Mise à jour README` |
| `TEST` | Ajout ou modification de tests | `TEST: Tests unitaires Health` |
| `CI` | Changements CI/CD | `CI: Pipeline Jenkins` |
| `MERGE` | Fusion de branches | `MERGE: feature/auth into main` |

### Exemples de Bons Commits

```bash
# Fonctionnalité
ADD: Implémentation LoginScene avec assets [EPI-42]

Ajout de la scène de connexion avec:
- Chargement du background et input field
- Gestion des événements clavier
- Transition vers GameScene

# Correction
FIX: Dangling reference dans asyncWrite [EPI-37]

Copie du message avant unlock du mutex pour éviter
la référence invalide après async_write.

# Refactoring
REFACTOR: Migration std::thread vers std::jthread

Utilisation de jthread pour auto-join RAII.
Améliore la gestion du lifecycle du thread I/O.
```

---

## Règles Générales

### À Faire

- **Commits atomiques** : Un commit = Un changement logique
- **Messages clairs** : Le "quoi" dans le titre, le "pourquoi" dans le corps
- **Référencer les tickets** : `[EPI-XX]` ou `[RT-XX]` quand applicable
- **Tester avant commit** : `cmake --build build && ctest --test-dir build`
- **Commiter souvent** : Petits commits fréquents > gros commits rares

### À Éviter

- Commits "WIP" ou "fix" sans contexte
- Plusieurs changements non liés dans un commit
- Messages en français et anglais mélangés (choisir une langue)
- Commits cassant le build

---

## Workflow Git

### Branches

```
main                    # Production stable
├── feature/xxx         # Nouvelles fonctionnalités
├── fix/xxx             # Corrections de bugs
├── refactor/xxx        # Refactoring
├── docs/xxx            # Documentation
└── test/xxx            # Tests
```

### Processus Standard

1. **Créer une branche** depuis `main`
   ```bash
   git checkout main
   git pull origin main
   git checkout -b feature/ma-feature
   ```

2. **Développer** avec commits atomiques
   ```bash
   git add -p  # Staging interactif
   git commit -m "ADD: Ma feature [EPI-XX]"
   ```

3. **Rebaser** avant merge
   ```bash
   git fetch origin
   git rebase origin/main
   ```

4. **Créer une Pull Request** via GitHub

5. **Review et Merge** après approbation

---

## Commits et CI/CD

### Vérifications Automatiques

Le pipeline Jenkins vérifie automatiquement :

- Compilation Debug et Release
- Tests unitaires
- Sanitizers (ASan, UBSan, LSan)
- Analyse SonarCloud (qualité code)

### Commits qui Cassent le Build

Si un commit casse le build :

1. **Ne pas paniquer**
2. Corriger rapidement avec `FIX: Correction build [RT-XX]`
3. Ne **jamais** force push sur `main`

---

## Convention AREA

Le projet utilise la convention AREA pour les commits :

- **A** - Add (Ajout)
- **R** - Remove (Suppression)
- **E** - Edit/Update (Modification)
- **A** - Adjust/Fix (Ajustement/Correction)

Cette convention peut être utilisée comme préfixe alternatif au format standard.

---

## Outils Utiles

### Alias Git Recommandés

```bash
# Dans ~/.gitconfig
[alias]
    st = status -sb
    ci = commit
    co = checkout
    br = branch
    lg = log --oneline --graph --decorate
    last = log -1 HEAD --stat
```

### Pre-commit Hooks

Le projet peut configurer des hooks pour valider les commits :

```bash
# .git/hooks/commit-msg
#!/bin/bash
# Vérifie le format du message de commit

msg=$(cat "$1")
if ! echo "$msg" | grep -qE "^(ADD|FIX|UPDATE|REMOVE|REFACTOR|DOCS|TEST|CI|MERGE):"; then
    echo "Format de commit invalide. Utilisez: TYPE: Description"
    exit 1
fi
```

---

## Ressources

- [Conventional Commits](https://www.conventionalcommits.org/)
- [Git Best Practices](https://git-scm.com/book/en/v2)
- [Guide de Contribution](contributing.md)

---

**Dernière révision:** 25/11/2025
