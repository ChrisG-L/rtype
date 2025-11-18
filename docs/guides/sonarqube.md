# SonarCloud - Qualité de Code

**Dernière mise à jour:** 18 janvier 2025

---

## 📊 Qu'est-ce que SonarCloud ?

**SonarCloud** est une plateforme d'analyse de qualité de code en ligne qui analyse automatiquement la branche `main` du projet R-Type.

### Ce que SonarCloud détecte

- **🐛 Bugs** : Erreurs de code qui peuvent causer des problèmes en production
- **🔒 Vulnérabilités** : Failles de sécurité (buffer overflows, injections, etc.)
- **💩 Code Smells** : Mauvaises pratiques qui rendent le code difficile à maintenir
- **📋 Duplication** : Code répété qui devrait être refactorisé
- **🧪 Couverture de tests** : Pourcentage de code couvert par les tests
- **🔢 Complexité** : Complexité cyclomatique du code

---

## 🌐 Accès à SonarCloud

Le projet R-Type est analysé automatiquement via l'intégration GitHub/GitLab.

### Accéder aux résultats

1. **Allez sur** : [https://sonarcloud.io](https://sonarcloud.io)
2. **Connectez-vous** avec votre compte GitHub/GitLab/Bitbucket
3. **Cherchez** le projet R-Type dans votre organisation
4. **Consultez** le dashboard pour voir les métriques

!!! tip "Analyses automatiques"
    SonarCloud analyse automatiquement :

    - ✅ Chaque push sur la branche **main**

    **Rien à configurer** - tout est géré via l'intégration SCM !

    !!! note "Branches et Pull Requests"
        Seule la branche `main` est analysée. Les autres branches et pull requests ne sont pas analysées automatiquement.

---

## 📈 Interpréter le Dashboard

### Métriques Principales

#### 🐛 Bugs
**Définition** : Erreurs de code qui peuvent causer des problèmes en production

**Exemples en C++** :
- Déréférencement de pointeur null
- Division par zéro
- Accès hors limites de tableau
- Fuite mémoire

**Priorité** : 🔴 **CRITIQUE** - À corriger immédiatement

#### 🔒 Vulnérabilités
**Définition** : Failles de sécurité exploitables

**Exemples en C++** :
- Buffer overflow
- Injection de commandes
- Utilisation de fonctions non sécurisées (`strcpy`, `sprintf`)
- Mauvaise gestion des ressources

**Priorité** : 🔴 **CRITIQUE** - À corriger immédiatement

#### 💩 Code Smells
**Définition** : Mauvaises pratiques qui rendent le code difficile à maintenir

**Exemples en C++** :
- Fonctions trop longues (>100 lignes)
- Complexité cyclomatique élevée (>15)
- Duplication de code
- Mauvais nommage de variables
- Commentaires TODO non résolus

**Priorité** : 🟡 **MOYEN** - À corriger progressivement

#### 🧪 Couverture de Tests
**Définition** : Pourcentage de code couvert par les tests unitaires

**Objectifs** :
- ✅ **>80%** : Très bonne couverture
- 🟡 **60-80%** : Couverture acceptable
- 🔴 **<60%** : Couverture insuffisante

#### 📋 Duplication
**Définition** : Pourcentage de code dupliqué

**Objectifs** :
- ✅ **<3%** : Très bon
- 🟡 **3-5%** : Acceptable
- 🔴 **>5%** : Refactoring nécessaire

### Niveaux de Sévérité

| Sévérité | Icône | Description | Action |
|----------|-------|-------------|--------|
| **Blocker** | 🔴 | Bloque le déploiement | Corriger immédiatement |
| **Critical** | 🟠 | Problème majeur | Corriger avant merge |
| **Major** | 🟡 | Problème important | Corriger rapidement |
| **Minor** | 🔵 | Problème mineur | Corriger progressivement |
| **Info** | ⚪ | Information | Optionnel |

---

## ✅ Quality Gates

Les **Quality Gates** sont des seuils de qualité que le code doit respecter pour être mergé.

### Seuils du Projet R-Type

```yaml
✅ Coverage on New Code >= 80%
✅ Duplicated Lines on New Code <= 3%
✅ Maintainability Rating on New Code = A
✅ Reliability Rating on New Code = A
✅ Security Rating on New Code = A
✅ Security Hotspots Reviewed = 100%
```

!!! warning "Quality Gate Failed"
    Si le Quality Gate échoue sur la branche `main` :

    1. **Consultez** les détails sur SonarCloud
    2. **Identifiez** les problèmes détectés
    3. **Créez** une branche de correction
    4. **Corrigez** les problèmes
    5. **Mergez** dans `main`
    6. **Attendez** 2-5 minutes que SonarCloud ré-analyse automatiquement

---

## 🔧 Workflow de Développement

### Consulter les Résultats

1. **Allez sur** [sonarcloud.io](https://sonarcloud.io)
2. **Connectez-vous** avec votre compte GitHub/GitLab
3. **Cherchez** le projet R-Type
4. **Consultez** les métriques de la branche **main**

!!! info "Analyses de la branche main uniquement"
    SonarCloud analyse uniquement la branche `main`. Les résultats reflètent l'état du code mergé dans la branche principale.

### Corriger les Problèmes Détectés

1. **Identifiez** les bugs, vulnérabilités ou code smells sur le dashboard
2. **Créez** une branche pour corriger le problème
3. **Corrigez** le code
4. **Créez** une Pull Request vers `main`
5. **Mergez** dans `main`
6. **Attendez** 2-5 minutes que SonarCloud ré-analyse automatiquement
7. **Vérifiez** que le problème a disparu sur SonarCloud

---

## 🛠️ Corriger les Problèmes

### Bugs Courants en C++

#### Déréférencement de Pointeur Null

```cpp
❌ Problème:
Player* player = nullptr;
player->move(10, 5, 0);  // Crash!

✅ Solution:
Player* player = nullptr;
if (player != nullptr) {
    player->move(10, 5, 0);
}
```

#### Accès Hors Limites

```cpp
❌ Problème:
std::vector<int> vec = {1, 2, 3};
int value = vec[10];  // Hors limites!

✅ Solution:
std::vector<int> vec = {1, 2, 3};
if (index < vec.size()) {
    int value = vec[index];
}
// Ou utiliser .at() qui lance une exception
int value = vec.at(index);
```

#### Fuite Mémoire

```cpp
❌ Problème:
Player* player = new Player();
// Jamais de delete = fuite mémoire

✅ Solution:
// Utiliser smart pointers
std::unique_ptr<Player> player = std::make_unique<Player>();
// Nettoyage automatique
```

### Vulnérabilités Courantes

#### Buffer Overflow

```cpp
❌ Problème:
char buffer[10];
strcpy(buffer, "Une très longue chaîne");  // Overflow!

✅ Solution:
std::string buffer = "Une très longue chaîne";
// Ou utiliser strncpy avec limite
char buffer[10];
strncpy(buffer, "longue chaîne", sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';
```

#### Utilisation de Fonctions Non Sécurisées

```cpp
❌ Problème:
char output[100];
sprintf(output, "Player: %s", playerName.c_str());

✅ Solution:
char output[100];
snprintf(output, sizeof(output), "Player: %s", playerName.c_str());
// Ou mieux encore, utiliser std::string
std::string output = "Player: " + playerName;
```

### Code Smells Courants

#### Fonction Trop Longue

```cpp
❌ Problème:
void processGame() {
    // 200 lignes de code...
    // Trop complexe à maintenir
}

✅ Solution:
void processGame() {
    updatePlayers();
    updateEnemies();
    checkCollisions();
    updateUI();
}

void updatePlayers() { /* logique spécifique */ }
void updateEnemies() { /* logique spécifique */ }
// ...
```

#### Duplication de Code

```cpp
❌ Problème:
void movePlayerLeft() {
    player.x -= speed;
    validatePosition(player);
    notifyObservers(player);
}

void movePlayerRight() {
    player.x += speed;
    validatePosition(player);
    notifyObservers(player);
}

✅ Solution:
void movePlayer(float dx, float dy) {
    player.x += dx;
    player.y += dy;
    validatePosition(player);
    notifyObservers(player);
}

void movePlayerLeft() { movePlayer(-speed, 0); }
void movePlayerRight() { movePlayer(speed, 0); }
```

---

## 📊 Métriques de Qualité du Projet

### Objectifs R-Type

| Métrique | Objectif | Priorité |
|----------|----------|----------|
| **Bugs** | 0 | 🔴 Critique |
| **Vulnérabilités** | 0 | 🔴 Critique |
| **Code Smells** | < 50 | 🟡 Important |
| **Couverture de tests** | > 80% | 🟠 Élevé |
| **Duplication** | < 3% | 🟡 Important |
| **Maintainability Rating** | A | 🟢 Bon |
| **Reliability Rating** | A | 🔴 Critique |
| **Security Rating** | A | 🔴 Critique |

---

## 🔗 Ressources

### Documentation

- **[SonarCloud](https://sonarcloud.io)** - Plateforme d'analyse
- **[Documentation SonarCloud](https://docs.sonarcloud.io/)** - Guide officiel
- **[Règles C++ SonarQube](https://rules.sonarsource.com/cpp/)** - Règles de qualité C++
- **[Quality Gates](https://docs.sonarcloud.io/improving/quality-gates/)** - Configuration des seuils

### Documentation Projet

- **[Architecture](architecture.md)** - Architecture globale du projet
- **[Bonnes Pratiques](best-practices.md)** - Standards de code C++23
- **[Tests](../development/testing.md)** - Guide des tests unitaires
- **[Contribution](../development/contributing.md)** - Guide de contribution

---

## ❓ Questions Fréquentes

### Comment voir les résultats de mon code ?

1. Mergez votre code dans la branche `main`
2. Attendez 2-5 minutes que SonarCloud analyse
3. Allez sur [sonarcloud.io](https://sonarcloud.io)
4. Consultez les métriques de la branche **main**

!!! note "Branches de développement"
    Seule la branche `main` est analysée. Votre code ne sera analysé qu'après avoir été mergé dans `main`.

### Comment suivre la qualité de mon code avant le merge ?

SonarCloud n'analyse que la branche `main`. Pour vérifier votre code avant le merge :

1. **Respectez les bonnes pratiques** C++23 (voir [Bonnes Pratiques](best-practices.md))
2. **Écrivez des tests unitaires** pour votre code
3. **Faites une revue de code** avec l'équipe
4. **Après le merge**, vérifiez sur SonarCloud qu'aucun problème n'a été introduit

### Comment augmenter la couverture de tests ?

1. Consultez le rapport de couverture sur SonarCloud
2. Identifiez les fichiers/fonctions non couverts
3. Ajoutez des tests unitaires avec Google Test
4. Voir le guide [Tests](../development/testing.md)

### Dois-je installer SonarCloud localement ?

**Non !** SonarCloud analyse automatiquement votre code à chaque push via l'intégration GitHub/GitLab. Aucune installation locale n'est nécessaire.

### Comment corriger un problème spécifique ?

1. Cliquez sur le problème dans SonarCloud
2. Lisez la description et l'explication
3. Consultez l'exemple de code fourni
4. Appliquez la correction recommandée
5. Pushez et attendez la ré-analyse

---

**Note** : SonarCloud est un outil puissant pour maintenir la qualité du code. Consultez régulièrement les rapports pour détecter les problèmes tôt et maintenir un code propre et maintenable !
