# Commandant Qualité

## Carte d'Identité

| Attribut | Valeur |
|----------|--------|
| **Rang** | Commandant |
| **Fichier** | `.claude/agents/commander_quality.md` |
| **Invocation** | `@commander_quality` |
| **Domaine** | Tests & Qualité de Code |
| **Autonomie** | Moyenne |

---

## Mission

Le **Commandant Qualité** est l'expert en tests et standards de code. Sa mission est de :

1. **Créer** des tests unitaires complets avec Google Test
2. **Analyser** la couverture de code et identifier les gaps
3. **Valider** le respect des standards C++23 et bonnes pratiques
4. **Coordonner** les soldats Architecte et Review pour les analyses approfondies

---

## Domaines d'Expertise

```mermaid
mindmap
  root((CMD QUALITÉ))
    Tests
      Google Test
      Tests unitaires
      Tests intégration
      Mocking
    Couverture
      Analyse coverage
      Identification gaps
      Métriques
    Standards
      C++23
      RAII
      Modern C++
      Clean Code
    Analyse
      Code smells
      Complexité
      Performance
```

---

## Standards de Qualité

### Métriques Cibles

| Métrique | Objectif | Critique |
|----------|----------|----------|
| **Couverture tests** | > 80% | < 60% |
| **Complexité cyclomatique** | < 10 | > 15 |
| **Duplication** | < 3% | > 5% |
| **Warnings compilation** | 0 | > 5 |
| **Code smells** | 0 critiques | > 0 |

### Standards C++23

```cpp
// Bonnes pratiques attendues

// RAII et smart pointers
auto entity = std::make_unique<Entity>();

// Auto et range-based for
for (const auto& component : components) {
    process(component);
}

// Const-correctness
int getValue() const { return value_; }

// Modern C++ features
std::optional<Entity> findEntity(EntityId id);
std::variant<Success, Error> processResult();
```

---

## Structure de Tests

### Format Google Test

```cpp
#include <gtest/gtest.h>

// Test fixture pour setup/teardown commun
class EntityManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<EntityManager>();
    }

    void TearDown() override {
        manager_.reset();
    }

    std::unique_ptr<EntityManager> manager_;
};

// Test basique
TEST_F(EntityManagerTest, CreateEntity_ReturnsValidId) {
    // Arrange
    // (déjà fait dans SetUp)

    // Act
    Entity entity = manager_->create();

    // Assert
    EXPECT_NE(entity, NULL_ENTITY);
}

// Test avec paramètres
TEST_F(EntityManagerTest, CreateMultipleEntities_AllUnique) {
    std::set<Entity> entities;

    for (int i = 0; i < 100; ++i) {
        entities.insert(manager_->create());
    }

    EXPECT_EQ(entities.size(), 100);
}

// Test d'exception
TEST_F(EntityManagerTest, DestroyInvalidEntity_ThrowsException) {
    EXPECT_THROW(
        manager_->destroy(NULL_ENTITY),
        std::invalid_argument
    );
}
```

---

## Commandes Types

=== "Créer des tests"

    ```bash
    @commander_quality "Crée des tests unitaires pour la classe Entity.

                        Couvrir:
                        - Constructeur
                        - Méthodes publiques
                        - Edge cases
                        - Exceptions"
    ```

=== "Analyser la couverture"

    ```bash
    @commander_quality "Analyse la couverture de tests du module ECS.

                        Identifie:
                        - Fonctions non testées
                        - Branches non couvertes
                        - Cas limites manquants"
    ```

=== "Valider la qualité"

    ```bash
    @commander_quality "Valide la qualité du code dans src/network/.

                        Vérifie:
                        - Standards C++23
                        - RAII
                        - Error handling
                        - Performance"
    ```

=== "Analyser un bug"

    ```bash
    @commander_quality "Analyse ce bug:

                        Description: Crash au lancement
                        Fichier: src/core/Engine.cpp
                        Ligne: 45
                        Erreur: Segfault

                        Identifie la root cause et propose un fix."
    ```

---

## Flux de Travail

```mermaid
flowchart TD
    subgraph "ANALYSE"
        A1[Lire le code] --> A2[Identifier les besoins de tests]
        A2 --> A3[Évaluer la couverture actuelle]
    end

    subgraph "DÉLÉGATION (si nécessaire)"
        B1{Besoin review architecture?}
        B1 -->|Oui| B2[Consulter Sol. Architecte]
        B1 -->|Non| B3[Continuer]
        B2 --> B3

        B3 --> B4{Besoin review code?}
        B4 -->|Oui| B5[Consulter Sol. Review]
        B4 -->|Non| B6[Continuer]
        B5 --> B6
    end

    subgraph "PRODUCTION"
        C1[Rédiger tests] --> C2[Valider couverture]
        C2 --> C3[Rapport qualité]
    end

    A3 --> B1
    B6 --> C1

    style A1 fill:#E3F2FD
    style C3 fill:#E8F5E9
```

---

## Format de Rapport

```markdown
# RAPPORT QUALITÉ - [Module]

## Résumé
**Score Global**: [XX/100]
**Couverture**: [XX%]
**Statut**: [VALIDÉ/EN ATTENTE/ÉCHEC]

---

## 1. Couverture de Tests

| Fichier | Couverture | Statut |
|---------|------------|--------|
| Entity.cpp | 92% | OK |
| World.cpp | 78% | Attention |
| ComponentPool.hpp | 100% | OK |

**Moyenne**: 90%

### Gaps Identifiés
- `World::destroyEntity()` : branche else non couverte
- `ComponentPool::remove()` : cas liste vide non testé

---

## 2. Analyse Code

### Code Smells
- [ ] Fonction trop longue: `processPacket()` (120 lignes)
- [ ] Magic numbers: ligne 45, 67, 89

### Complexité
| Fonction | Complexité | Limite |
|----------|------------|--------|
| handleEvent | 12 | 10 |
| processPacket | 18 | 10 |

---

## 3. Recommandations

### Priorité Haute
1. Ajouter tests pour `destroyEntity()` cas edge
2. Refactorer `processPacket()` - trop complexe

### Priorité Normale
3. Extraire constantes magiques
4. Améliorer documentation fonctions publiques

---

## Tests à Créer

```cpp
// Test manquant 1
TEST(World, DestroyEntity_InvalidId_DoesNotCrash) {
    World world;
    EXPECT_NO_THROW(world.destroyEntity(NULL_ENTITY));
}

// Test manquant 2
TEST(ComponentPool, Remove_EmptyPool_DoesNotCrash) {
    ComponentPool<Position> pool;
    EXPECT_NO_THROW(pool.remove(Entity{1}));
}
```
```

---

## Interactions

```mermaid
graph TD
    CMD_QUA((CMD QUALITÉ))

    GEN[Général] -->|délègue| CMD_QUA
    CMD_QUA -->|consulte| SOL_ARCH[Sol. Architecte]
    CMD_QUA -->|consulte| SOL_REV[Sol. Code Review]

    CMD_QUA -->|produit| TESTS[Tests]
    CMD_QUA -->|produit| RAPPORT[Rapport Qualité]
    CMD_QUA -->|valide pour| CMD_GIT[Cmd Git]

    style CMD_QUA fill:#4169E1,color:#fff
    style GEN fill:#FFD700
    style SOL_ARCH fill:#228B22,color:#fff
    style SOL_REV fill:#228B22,color:#fff
```

---

## Checklist Qualité

- [ ] Tous les fichiers modifiés ont des tests
- [ ] Couverture >= 80%
- [ ] Aucun warning de compilation
- [ ] Complexité < 10 par fonction
- [ ] Pas de code dupliqué
- [ ] Standards C++23 respectés
- [ ] RAII utilisé partout
- [ ] Error handling approprié

---

## Voir Aussi

- [Général](general.md)
- [Soldat Architecte](soldier-architecture.md)
- [Soldat Code Review](soldier-code-review.md)
- [Guide des Tests](/development/testing.md)
