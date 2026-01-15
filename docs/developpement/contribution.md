---
tags:
  - developpement
  - contribution
---

# Guide de Contribution

Comment contribuer au projet R-Type.

## Code de Conduite

Nous nous engageons à fournir un environnement accueillant et respectueux pour tous les contributeurs.

- Soyez respectueux et inclusif
- Acceptez la critique constructive
- Concentrez-vous sur ce qui est le mieux pour la communauté
- Montrez de l'empathie envers les autres membres

---

## Comment Contribuer

### 1. Fork et Clone

```bash
# Fork sur GitHub, puis clone
git clone https://github.com/VOTRE_USERNAME/rtype.git
cd rtype

# Ajouter le remote upstream
git remote add upstream https://github.com/original-org/rtype.git
```

### 2. Créer une Branche

```bash
# Mettre à jour develop
git checkout develop
git pull upstream develop

# Créer une branche
git checkout -b feature/ma-feature
# ou
git checkout -b fix/mon-bugfix
```

### 3. Développer

```bash
# Configurer (installe vcpkg + dépendances)
./scripts/build.sh

# Compiler
./scripts/compile.sh

# Tester
./scripts/test.sh
```

### 4. Commit

```bash
# Format du commit (conventional commits)
git commit -m "type(scope): description"

# Exemples
git commit -m "feat(server): add wave manager"
git commit -m "fix(client): resolve rendering issue"
git commit -m "docs(api): update GameWorld documentation"
```

### 5. Push et PR

```bash
git push origin feature/ma-feature
```

Puis créer une Pull Request sur GitHub.

---

## Conventional Commits

### Format

```
type(scope): description

[optional body]

[optional footer]
```

### Types

| Type | Description |
|------|-------------|
| `feat` | Nouvelle fonctionnalité |
| `fix` | Correction de bug |
| `docs` | Documentation |
| `style` | Formatage, pas de changement de code |
| `refactor` | Refactoring sans nouvelle feature ni fix |
| `perf` | Amélioration de performance |
| `test` | Ajout/modification de tests |
| `chore` | Maintenance (build, CI, etc.) |

### Scopes

| Scope | Description |
|-------|-------------|
| `server` | Code serveur |
| `client` | Code client |
| `network` | Protocole réseau |
| `audio` | Système audio |
| `graphics` | Rendu graphique |
| `docs` | Documentation |
| `ci` | CI/CD |

### Exemples

```bash
feat(server): implement enemy wave system

Add WaveManager class to handle enemy spawning:
- 5 waves of increasing difficulty
- Boss at wave 5
- Configurable spawn patterns

Closes #42

---

fix(client): resolve memory leak in audio manager

VoiceChat component was not properly releasing
PortAudio resources on disconnect.

Fixes #123

---

docs(api): add GameWorld class documentation

- Add UML diagram
- Document all public methods
- Add usage examples
```

---

## Pull Request

### Template

```markdown
## Description

[Description claire des changements]

## Type de changement

- [ ] Bug fix
- [ ] Nouvelle feature
- [ ] Breaking change
- [ ] Documentation

## Checklist

- [ ] Mon code suit les conventions du projet
- [ ] J'ai testé mes changements localement
- [ ] J'ai ajouté des tests si nécessaire
- [ ] J'ai mis à jour la documentation si nécessaire
- [ ] Tous les tests passent

## Screenshots (si applicable)

## Issues liées

Closes #XXX
```

### Review Process

1. **CI doit passer** - Jenkins build et tests
2. **Au moins 1 approval** - Review par un mainteneur
3. **Pas de conflicts** - Branche à jour avec develop
4. **Squash and merge** - Pour un historique propre

---

## Issues

### Bug Report

```markdown
## Description

[Description du bug]

## Reproduction

1. Étape 1
2. Étape 2
3. ...

## Comportement attendu

[Ce qui devrait se passer]

## Comportement actuel

[Ce qui se passe]

## Environnement

- OS: [ex: Ubuntu 24.04]
- Version: [ex: v1.0.0]
- Compilateur: [ex: Clang 16]

## Logs/Screenshots

[Si applicable]
```

### Feature Request

```markdown
## Description

[Description de la feature]

## Motivation

[Pourquoi cette feature serait utile]

## Solution proposée

[Comment vous imaginez l'implémentation]

## Alternatives considérées

[Autres approches possibles]
```

---

## Architecture Décisions

Pour tout changement architectural significatif, créer un ADR :

```markdown
# ADR-XXX: Titre

## Status

Proposed | Accepted | Deprecated | Superseded

## Context

[Pourquoi cette décision est nécessaire]

## Decision

[La décision prise]

## Consequences

### Positive
- ...

### Negative
- ...
```

---

## Communication

### Canaux

| Canal | Usage |
|-------|-------|
| GitHub Issues | Bugs, features |
| GitHub Discussions | Questions, idées |
| Pull Requests | Code review |

### Labels

| Label | Description |
|-------|-------------|
| `bug` | Bug confirmé |
| `enhancement` | Nouvelle feature |
| `documentation` | Doc uniquement |
| `good first issue` | Bon pour débutants |
| `help wanted` | Besoin d'aide |
| `priority: high` | Priorité haute |
| `priority: low` | Priorité basse |

---

## Setup Développement

### Prérequis

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake ninja-build git clang

# vcpkg est installé automatiquement par build.sh
```

### Build Rapide

```bash
# 1. Clone
git clone https://github.com/org/rtype.git
cd rtype

# 2. Configure + Build
./scripts/build.sh
./scripts/compile.sh

# 3. Test
./scripts/test.sh

# 4. Run
./artifacts/server/linux/rtype_server
./artifacts/client/linux/rtype_client
```

### Cross-Compile Windows

```bash
# Installer MinGW
sudo apt install mingw-w64

# Build pour Windows
./scripts/build.sh --platform=windows
./scripts/compile.sh
```

---

## Reconnaissance

Tous les contributeurs sont listés dans le fichier `CONTRIBUTORS.md` et mentionnés dans les release notes.

Merci de contribuer au projet R-Type !
