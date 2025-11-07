# Guide de Contribution

Merci de votre intérêt pour contribuer au projet R-Type ! Ce document vous guidera à travers le processus de contribution.

## Premiers Pas

### 1. Fork et Clone

```bash
# Fork le repository sur GitHub
# Puis clonez votre fork
git clone https://github.com/VOTRE-USERNAME/rtype.git
cd rtype

# Ajoutez le remote upstream
git remote add upstream https://github.com/Pluenet-Killian/rtype.git
```

### 2. Configuration de l'environnement

```bash
# Installez les dépendances et configurez
./scripts/build.sh

# Compilez
./scripts/compile.sh

# Vérifiez que les tests passent
./artifacts/server/linux/server_tests
```

## Workflow de Contribution

### 1. Créer une branche

Créez une branche pour votre fonctionnalité ou correction :

```bash
# Pour une nouvelle fonctionnalité
git checkout -b feature/nom-de-la-fonctionnalite

# Pour une correction de bug
git checkout -b fix/description-du-bug

# Pour de la documentation
git checkout -b docs/sujet-documente

# Pour du refactoring
git checkout -b refactor/description
```

**Convention de nommage des branches :**

- `feature/` - Nouvelles fonctionnalités
- `fix/` - Corrections de bugs
- `docs/` - Documentation
- `refactor/` - Refactoring sans changement fonctionnel
- `test/` - Ajout ou modification de tests
- `ci/` - Modifications CI/CD

### 2. Développer

#### Avant de coder

1. Assurez-vous de comprendre le problème
2. Consultez l'[architecture](../guides/architecture.md) du projet
3. Lisez le code existant dans la zone que vous modifiez
4. Si vous ajoutez une feature importante, discutez-en d'abord dans une issue

#### Pendant le développement

- Écrivez du code propre et lisible
- Commentez les parties complexes
- Suivez les conventions C++23 modernes
- Testez régulièrement vos modifications

```bash
# Compilation incrémentale
./scripts/compile.sh

# Tests
./artifacts/server/linux/server_tests
```

#### Bonnes pratiques

**C++ Moderne :**

```cpp
// ✅ Bon : utiliser auto et range-based for
auto players = getPlayers();
for (const auto& player : players) {
    processPlayer(player);
}

// ❌ Éviter : itérateurs manuels sans raison
std::vector<Player> players = getPlayers();
for (std::vector<Player>::iterator it = players.begin();
     it != players.end(); ++it) {
    processPlayer(*it);
}
```

**RAII et gestion mémoire :**

```cpp
// ✅ Bon : smart pointers
auto connection = std::make_unique<Connection>();

// ❌ Éviter : raw pointers et new/delete
Connection* connection = new Connection();
// ...
delete connection;
```

**Const-correctness :**

```cpp
// ✅ Bon : const pour les méthodes qui ne modifient pas
class Player {
public:
    int getScore() const { return score_; }
    void addScore(int points) { score_ += points; }
private:
    int score_;
};
```

### 3. Ajouter des Tests

Tous les nouveaux code doivent être testés.

**Exemple de test dans `tests/server/main.cpp` :**

```cpp
#include <gtest/gtest.h>
#include "votre_classe.hpp"

TEST(VotreClasse, VotreMethode) {
    // Arrange
    VotreClasse obj;

    // Act
    auto result = obj.votreMethode();

    // Assert
    EXPECT_EQ(result, valeurAttendue);
}

TEST(VotreClasse, GestionErreurs) {
    VotreClasse obj;
    EXPECT_THROW(obj.methodeQuiLanceException(), std::exception);
}
```

**Lancer les tests :**

```bash
./scripts/compile.sh
./artifacts/server/linux/server_tests
```

**Tests spécifiques :**

```bash
# Lancer un test spécifique
./artifacts/server/linux/server_tests --gtest_filter=VotreClasse.VotreMethode

# Lancer une suite
./artifacts/server/linux/server_tests --gtest_filter=VotreClasse.*
```

### 4. Vérifier le Code

Avant de commit, vérifiez :

#### Compilation sans warnings

```bash
./scripts/compile.sh
# Aucun warning ne doit apparaître
```

#### Tests passent

```bash
./artifacts/server/linux/server_tests
# Tous les tests doivent passer
```

#### Pas de fuites mémoire (Debug mode)

Le mode Debug active automatiquement AddressSanitizer :

```bash
# Si votre code fuit, vous verrez des erreurs
./artifacts/server/linux/rtype_server
./artifacts/server/linux/server_tests
```

#### Optionnel : Valgrind

```bash
valgrind --leak-check=full ./artifacts/server/linux/server_tests
```

### 5. Commit

Utilisez des messages de commit clairs et descriptifs.

#### Format des commits

```
<type>(<scope>): <sujet>

<corps optionnel>

<footer optionnel>
```

**Types :**

- `feat` - Nouvelle fonctionnalité
- `fix` - Correction de bug
- `docs` - Documentation
- `style` - Formatage, point-virgules manquants, etc.
- `refactor` - Refactoring sans changement fonctionnel
- `test` - Ajout ou modification de tests
- `chore` - Tâches de maintenance (build, CI, etc.)
- `perf` - Amélioration de performance

**Exemples :**

```bash
# Feature
git commit -m "feat(server): add player connection handling"

# Fix
git commit -m "fix(network): resolve socket timeout issue"

# Documentation
git commit -m "docs(readme): update installation instructions"

# Multi-lignes
git commit -m "feat(game): implement game loop

- Add main game loop with fixed timestep
- Implement entity update system
- Add basic collision detection

Closes #42"
```

#### Commits atomiques

Faites des commits petits et logiques :

```bash
# ✅ Bon : commits séparés pour chaque changement logique
git add src/server/player.cpp
git commit -m "feat(server): add Player class"

git add tests/server/player_test.cpp
git commit -m "test(server): add Player tests"

# ❌ Éviter : un gros commit avec tout
git add .
git commit -m "add stuff"
```

### 6. Push et Pull Request

#### Push vers votre fork

```bash
git push origin feature/votre-branche
```

#### Créer une Pull Request

1. Allez sur GitHub
2. Cliquez sur "New Pull Request"
3. Sélectionnez votre branche
4. Remplissez le template de PR :

```markdown
## Description
Brève description de vos changements

## Type de changement
- [ ] Bug fix
- [ ] Nouvelle fonctionnalité
- [ ] Breaking change
- [ ] Documentation

## Checklist
- [ ] Les tests passent
- [ ] J'ai ajouté des tests
- [ ] J'ai mis à jour la documentation
- [ ] Mon code suit les conventions du projet
- [ ] Pas de warnings de compilation

## Tests
Comment avez-vous testé vos changements ?

## Screenshots (si applicable)
```

#### Revue de code

- Répondez aux commentaires de review
- Faites les modifications demandées
- Poussez les changements (ils s'ajouteront automatiquement à la PR)

```bash
# Faire des modifications
git add .
git commit -m "fix: address review comments"
git push origin feature/votre-branche
```

## Standards de Code

### Style C++

#### Nommage

```cpp
// Classes, structs, enums : PascalCase
class PlayerManager {};
struct Position {};
enum class GameState {};

// Fonctions, variables : camelCase
void updatePlayer();
int playerScore;

// Constantes : UPPER_SNAKE_CASE
const int MAX_PLAYERS = 4;
constexpr int BUFFER_SIZE = 1024;

// Membres privés : snake_case avec underscore final
class Player {
private:
    int score_;
    std::string name_;
};
```

#### Indentation et Formatage

- **Indentation :** 4 espaces (pas de tabs)
- **Accolades :** Style K&R (accolade ouvrante sur la même ligne)
- **Ligne max :** 100-120 caractères
- **Espaces :** autour des opérateurs

```cpp
// ✅ Bon
class Player {
public:
    void updateScore(int points) {
        if (points > 0) {
            score_ += points;
        }
    }

private:
    int score_ = 0;
};

// ❌ Éviter
class Player
{
  public:
    void updateScore(int points)
    {
      if(points>0){score_+=points;}
    }
  private:
    int score_=0;
};
```

#### Headers

```cpp
// player.hpp
#pragma once  // Préféré à #ifndef

#include <string>
#include <vector>

namespace rtype {

class Player {
public:
    Player(std::string name);
    ~Player() = default;

    // Rule of 5 ou = default
    Player(const Player&) = default;
    Player& operator=(const Player&) = default;
    Player(Player&&) = default;
    Player& operator=(Player&&) = default;

    void update(float deltaTime);
    int getScore() const;

private:
    std::string name_;
    int score_ = 0;
};

}  // namespace rtype
```

### Documentation

#### Commentaires de code

```cpp
// ✅ Bon : explique le POURQUOI
// Use binary search because the list is always sorted
// and can contain millions of entries
auto it = std::lower_bound(players.begin(), players.end(), target);

// ❌ Éviter : explique le QUOI (évident dans le code)
// Increment i by 1
i++;
```

#### Documentation Doxygen (optionnel)

```cpp
/**
 * @brief Manages player connections and states
 *
 * This class handles all player-related operations including
 * connection establishment, state synchronization, and cleanup.
 */
class PlayerManager {
public:
    /**
     * @brief Adds a new player to the game
     * @param name Player's display name
     * @return Player ID or -1 on error
     */
    int addPlayer(const std::string& name);
};
```

## Structure d'un PR

### Taille

- **Idéal :** < 300 lignes modifiées
- **Maximum :** < 1000 lignes
- Si plus grand, décomposez en plusieurs PR

### Contenu

Une PR doit :

1. Résoudre UN problème ou ajouter UNE fonctionnalité
2. Inclure les tests correspondants
3. Mettre à jour la documentation si nécessaire
4. Passer tous les checks CI/CD

### Description

```markdown
## Résumé
Description concise des changements (2-3 phrases)

## Motivation
Pourquoi ces changements sont nécessaires ?

## Changements
- Liste des modifications principales
- Point par point

## Tests
- Quels tests ont été ajoutés ?
- Comment tester manuellement ?

## Screenshots/Logs
Si pertinent, ajoutez des captures ou logs
```

## Revue de Code

### En tant que contributeur

- Soyez ouvert aux retours
- Posez des questions si quelque chose n'est pas clair
- Ne prenez pas les critiques personnellement
- Remerciez les reviewers

### En tant que reviewer

- Soyez constructif et bienveillant
- Expliquez le "pourquoi" de vos suggestions
- Distinguez les "must fix" des "nice to have"
- Approuvez quand c'est prêt

## CI/CD

Chaque PR déclenche le pipeline Jenkins qui :

1. Checkout du code
2. Installation des dépendances
3. Compilation
4. Exécution des tests
5. Checks de qualité

**Votre PR doit passer tous les checks avant merge.**

Si les checks échouent :

```bash
# Reproduire localement
./scripts/build.sh
./scripts/compile.sh
./artifacts/server/linux/server_tests

# Corriger les erreurs
# Commit et push
```

## Questions et Support

### Communication

- **Issues GitHub :** Pour bugs et features
- **Discussions GitHub :** Pour questions générales
- **PR Comments :** Pour discussions sur le code spécifique

### Ressources

- [Guide de démarrage rapide](../getting-started/quickstart.md)
- [Architecture du projet](../guides/architecture.md)
- [FAQ](../reference/faq.md)

## Checklist du Contributeur

Avant de soumettre votre PR :

- [ ] J'ai créé une branche appropriée
- [ ] Mon code compile sans warnings
- [ ] Tous les tests passent
- [ ] J'ai ajouté des tests pour mon code
- [ ] J'ai mis à jour la documentation si nécessaire
- [ ] Mes commits suivent les conventions
- [ ] Ma PR a une description claire
- [ ] Le pipeline CI passe

## Licence

En contribuant, vous acceptez que vos contributions soient sous la même licence que le projet (projet pédagogique EPITECH).

## Merci !

Merci de contribuer au projet R-Type. Chaque contribution, petite ou grande, est appréciée !
