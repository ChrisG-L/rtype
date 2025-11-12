# Introduction à la Ruche d'Agents IA - R-Type

## Vue d'Ensemble

La **Ruche d'Agents IA** est un système d'assistance au développement basé sur **Claude Code** qui utilise des agents spécialisés pour améliorer la qualité et la productivité du projet R-Type. Chaque agent est un **prompt spécialisé** que vous pouvez invoquer pour obtenir une expertise ciblée.

!!! info "Qu'est-ce qu'un Agent ?"
    Un agent est un fichier markdown (`.md`) dans `.claude/agents/` qui définit un rôle, des responsabilités et des standards spécifiques. Lorsque vous invoquez un agent, Claude adopte cette personnalité et cette expertise pour vous assister.

## 🎯 Philosophie

**Objectif** : Améliorer la qualité et la cohérence du code grâce à des assistants IA spécialisés.

- ✅ **Expertise ciblée** : Chaque agent maîtrise un domaine spécifique
- ✅ **Standards cohérents** : Tous les agents respectent les mêmes conventions
- ✅ **Qualité professionnelle** : Guidance experte pour chaque tâche
- ✅ **Flexibilité** : Invoquez les agents selon vos besoins

## 👑 Architecture de la Ruche

```
          👑 Queen Bee (Orchestratrice)
                      |
       ┌──────────────┼──────────────┬─────────────┐
       |              |              |             |
   🐝 Tests       🐝 Security    🐝 Git       🐝 Docs
                      |
                  🐝 Linear
```

### Hiérarchie des Agents

- **Queen Bee** : Agent orchestrateur qui coordonne les autres agents
- **Agents Spécialisés** : Experts dans leur domaine respectif

## 📂 Structure du Projet

```
rtype/
├── .claude/                  # Configuration Claude Code
│   ├── agents/               # Agents IA (prompts spécialisés)
│   │   ├── queen_bee_agent.md          # 👑 Orchestratrice
│   │   ├── docs_agent.md               # 📚 Documentation
│   │   ├── git_commit_agent.md         # 📝 Commits Git
│   │   ├── tests_quality_agent.md      # 🧪 Tests & Qualité
│   │   ├── security_ci_agent.md        # 🔒 Sécurité & CI
│   │   └── linear_manager_agent.md     # 📋 Gestion Linear
│   └── settings.local.json   # Configuration locale
│
└── docs/                     # Documentation du projet
    └── hive/                 # Documentation de la Ruche
        └── introduction.md   # Ce fichier
```

## 🐝 Les Agents Disponibles

### 👑 Queen Bee Agent - L'Orchestratrice

**Fichier** : `.claude/agents/queen_bee_agent.md`

**Rôle** : Agent suprême qui coordonne tous les autres agents et assure la cohérence globale du projet.

**Responsabilités** :
- Orchestrer le travail des agents spécialisés
- Superviser la qualité globale du projet
- Garantir la cohérence entre code, documentation et tickets
- Valider que les standards sont respectés

**Quand l'invoquer** :
```
@queen_bee_agent "J'ai terminé une feature majeure, vérifie que tout est en ordre"
```

---

### 📚 Documentation Agent

**Fichier** : `.claude/agents/docs_agent.md`

**Rôle** : Expert en documentation technique avec MkDocs Material.

**Responsabilités** :
- Créer une documentation technique de qualité professionnelle
- Maintenir la cohérence et la clarté de la documentation
- Éviter la documentation superficielle ou inutile
- Utiliser des diagrammes et exemples concrets

**Standards** :
- Documentation en français
- Markdown bien formaté
- Exemples fonctionnels
- Diagrammes Mermaid quand pertinent
- Pas de pages "TODO" ou placeholder

**Quand l'invoquer** :
```
@docs_agent "Crée la documentation pour le système ECS"
@docs_agent "Vérifie que la documentation réseau est complète"
```

---

### 📝 Git Commit Agent

**Fichier** : `.claude/agents/git_commit_agent.md`

**Rôle** : Expert en gestion de versions Git et création de commits atomiques.

**Responsabilités** :
- Analyser les changements staged et unstaged
- Proposer des commits atomiques et cohérents
- Respecter la convention Conventional Commits
- Créer des messages de commit clairs en français

**Convention de Commit** :
```
<type>(<scope>): <description courte> [ID-TICKET]

<description détaillée>

<footer>
```

**Types de commits** :
- `FEAT` : Nouvelle fonctionnalité
- `FIX` : Correction de bug
- `REFACTOR` : Refactorisation
- `DOCS` : Documentation
- `BUILD` : Build et dépendances
- `TEST` : Tests
- `CI` : CI/CD
- `PERF` : Performance

**Quand l'invoquer** :
```
@git_commit_agent "Analyse mes changements et propose des commits atomiques"
@git_commit_agent "Crée un commit pour le système ECS"
```

---

### 🧪 Tests & Quality Agent

**Fichier** : `.claude/agents/tests_quality_agent.md`

**Rôle** : Expert en tests unitaires et qualité de code C++23.

**Responsabilités** :
- Créer des tests unitaires avec Google Test
- Vérifier la couverture de tests
- Analyser la qualité du code
- Proposer des améliorations

**Standards** :
- Couverture de tests >= 80%
- Tests atomiques et indépendants
- Nommage clair des tests
- Assertions appropriées

**Quand l'invoquer** :
```
@tests_quality_agent "Crée des tests pour la classe Entity"
@tests_quality_agent "Analyse la couverture de tests du module ECS"
```

---

### 🔒 Security & CI Agent

**Fichier** : `.claude/agents/security_ci_agent.md`

**Rôle** : Expert en sécurité et intégration continue.

**Responsabilités** :
- Détecter les vulnérabilités de sécurité
- Configurer et optimiser les pipelines CI/CD
- Analyser les dépendances
- Recommander les meilleures pratiques

**Domaines** :
- Sécurité du code C++
- Configuration Jenkins
- Analyse SonarQube
- Docker et conteneurisation

**Quand l'invoquer** :
```
@security_ci_agent "Vérifie les vulnérabilités dans le code réseau"
@security_ci_agent "Optimise le pipeline Jenkins"
```

---

### 📋 Linear Manager Agent

**Fichier** : `.claude/agents/linear_manager_agent.md`

**Rôle** : Chef de projet technique spécialisé dans Linear.

**Responsabilités** :
- Créer et organiser les tickets Linear
- Maintenir une structure claire et cohérente
- Documenter le travail accompli
- Lier le code aux tickets

**Structure Linear** :
```
Epic (FR) → Features (FR) → Tasks (FR) → Bugs (FR)
```

**Nomenclature** :
- Épics : `[EPIC] Système ECS`
- Features : `[FEAT] Component Manager`
- Tasks : `[TASK] Implémentation de la classe Entity`
- Bugs : `[BUG] Fuite mémoire dans EntityManager`

**Quand l'invoquer** :
```
@linear_manager_agent "Crée les tickets pour le système ECS"
@linear_manager_agent "Organise les tickets de la feature réseau"
```

---

## 💻 Comment Utiliser les Agents

### Méthode 1 : Invocation Directe

Dans Claude Code, utilisez la syntaxe `@nom_agent` :

```
@docs_agent "Crée la documentation du système de particules"
```

### Méthode 2 : Demande à la Reine

Demandez à Queen Bee de coordonner plusieurs agents :

```
@queen_bee_agent "J'ai terminé le système ECS, assure-toi que tout est parfait (code, tests, docs, tickets)"
```

La Reine va :
1. Analyser le code avec l'agent Tests
2. Vérifier la sécurité avec l'agent Security
3. Proposer des commits avec l'agent Git
4. Valider la documentation avec l'agent Docs
5. Créer les tickets avec l'agent Linear

### Méthode 3 : Conversation Naturelle

Vous pouvez aussi simplement décrire votre besoin :

```
"Aide-moi à créer des tests unitaires pour la classe NetworkManager"
```

Claude comprendra automatiquement qu'il doit adopter le rôle de l'agent Tests & Quality.

## 🔄 Workflows Typiques

### Workflow 1 : Développement d'une Feature

```
1. Vous codez la feature
2. @tests_quality_agent "Crée les tests unitaires"
3. @docs_agent "Documente cette feature"
4. @git_commit_agent "Propose des commits atomiques"
5. @linear_manager_agent "Crée les tickets correspondants"
```

### Workflow 2 : Review de Code

```
1. @security_ci_agent "Analyse les vulnérabilités du code réseau"
2. @tests_quality_agent "Vérifie la couverture de tests"
3. @queen_bee_agent "Valide que tout respecte les standards"
```

### Workflow 3 : Bugfix

```
1. @tests_quality_agent "Crée un test de régression pour le bug"
2. Vous corrigez le bug
3. @git_commit_agent "Crée un commit de fix"
4. @linear_manager_agent "Marque le ticket comme résolu"
```

### Workflow 4 : Documentation

```
1. @docs_agent "Crée un guide complet sur l'architecture ECS"
2. @docs_agent "Ajoute des diagrammes Mermaid"
3. @git_commit_agent "Commit la documentation"
```

## 📊 Standards et Conventions

### Standards de Qualité

Tous les agents respectent ces standards :

| Critère | Standard |
|---------|----------|
| **Couverture de tests** | >= 80% |
| **Commits** | Atomiques, Conventional Commits |
| **Documentation** | Complète, avec exemples |
| **Code** | C++23, RAII, Modern C++ |
| **Sécurité** | 0 vulnérabilité critique |

### Conventions de Nommage

- **Commits** : Français, Conventional Commits
- **Tickets Linear** : Français, [TYPE] Description
- **Documentation** : Français, Markdown bien formaté
- **Code** : Anglais, camelCase/PascalCase selon contexte

## 🎓 Exemples Concrets

### Exemple 1 : Créer une Feature Complète

```
Vous: @queen_bee_agent "Je viens de terminer le système de particules.
     Assure-toi que le code, les tests, la documentation et les tickets
     sont parfaits avant que je commit."

Queen Bee: 🔍 Analyse en cours...

          ✅ Tests & Qualité
             - Couverture: 87%
             - Tous les tests passent
             - Suggestion: Ajouter test de charge

          ✅ Sécurité
             - Aucune vulnérabilité détectée
             - Bonnes pratiques RAII respectées

          📝 Commits Proposés
             1. FEAT(rendering): ajout système de particules [RT-45]
             2. TEST(rendering): ajout tests unitaires particules
             3. DOCS(rendering): documentation système particules

          📚 Documentation
             - Guide créé: docs/guides/particle-system.md
             - Diagrammes ajoutés
             - Exemples fonctionnels inclus

          📋 Linear
             - Ticket [FEAT] Système de Particules créé
             - Lié à Epic "Rendering Engine"
             - Statut: Completed

          ✅ VALIDATION GLOBALE: Prêt pour commit et merge !
```

### Exemple 2 : Review de Sécurité

```
Vous: @security_ci_agent "Analyse le code du NetworkManager pour détecter
     les vulnérabilités potentielles"

Security Agent: 🔒 Analyse de sécurité du NetworkManager

                Fichiers analysés:
                - src/network/NetworkManager.cpp
                - src/network/Socket.cpp
                - src/network/Protocol.cpp

                ✅ Points positifs:
                - Utilisation de smart pointers (pas de fuites)
                - Validation des entrées réseau
                - Timeouts configurés

                ⚠️  Points à améliorer:
                1. Buffer overflow potentiel ligne 234
                   Recommandation: Utiliser std::span<> ou vérifier size

                2. Pas de rate limiting sur les connexions
                   Recommandation: Implémenter TokenBucket

                📊 Score de sécurité: 8.5/10
```

### Exemple 3 : Documentation Technique

```
Vous: @docs_agent "Crée une documentation complète pour l'architecture ECS
     avec des diagrammes et des exemples"

Docs Agent: 📚 Documentation de l'Architecture ECS

            Fichier créé: docs/guides/ecs-architecture.md

            Contenu:
            ✅ Introduction au pattern ECS
            ✅ Diagramme d'architecture (Mermaid)
            ✅ Explication des Components
            ✅ Explication des Systems
            ✅ Explication des Entities
            ✅ Exemple complet d'utilisation
            ✅ Bonnes pratiques
            ✅ Anti-patterns à éviter
            ✅ Performance et optimisations

            Sections créées: 8
            Diagrammes: 3
            Exemples de code: 5
            Temps de lecture estimé: 15 minutes

            La documentation est prête à être consultée !
```

## 🛠️ Configuration

### Fichier de Configuration

**Emplacement** : `.claude/settings.local.json`

```json
{
  "permissions": {
    "allow": [
      "Bash(chmod:*)"
    ],
    "deny": [],
    "ask": []
  }
}
```

### Ajouter un Nouvel Agent

Pour créer un nouvel agent :

1. Créez un fichier `.md` dans `.claude/agents/`
2. Définissez le rôle, les responsabilités et les standards
3. Documentez les exemples d'utilisation
4. Testez l'agent avec `@nom_agent`

**Structure recommandée d'un agent** :

```markdown
# Nom de l'Agent

## Identité et Mission

Description du rôle et de la mission

## Principes Fondamentaux

Liste des principes clés

## Responsabilités

- Responsabilité 1
- Responsabilité 2

## Standards

Liste des standards à respecter

## Exemples d'Utilisation

Exemples concrets
```

## 🆘 FAQ

### Comment savoir quel agent utiliser ?

- **Code & Tests** → Tests & Quality Agent
- **Documentation** → Documentation Agent
- **Commits Git** → Git Commit Agent
- **Sécurité & CI/CD** → Security & CI Agent
- **Tickets Linear** → Linear Manager Agent
- **Coordination globale** → Queen Bee Agent

### Puis-je utiliser plusieurs agents en même temps ?

Oui ! Utilisez Queen Bee qui va orchestrer automatiquement les agents nécessaires.

### Les agents peuvent-ils modifier mon code ?

Les agents proposent des modifications, mais c'est **vous qui décidez** d'accepter ou non. Vous gardez toujours le contrôle.

### Comment personnaliser un agent ?

Éditez le fichier `.md` de l'agent dans `.claude/agents/` pour modifier son comportement.

### Où sont stockés les résultats des agents ?

Les résultats sont affichés directement dans la conversation Claude Code. Il n'y a pas de fichiers de rapports automatiques.

## 📚 Ressources

- [Documentation Claude Code](https://docs.claude.com/claude-code)
- [Agents du projet](./.claude/agents/)
- [Guide de contribution](../development/contributing.md)

## 🎯 Prochaines Étapes

1. **Explorez les agents** : Lisez les fichiers dans `.claude/agents/`
2. **Testez un agent** : Essayez `@docs_agent "Aide-moi à documenter ce code"`
3. **Utilisez Queen Bee** : `@queen_bee_agent "Analyse mon feature complète"`
4. **Personnalisez** : Adaptez les agents à vos besoins

---

**🐝 Bienvenue dans la Ruche R-Type ! Les agents IA sont là pour vous assister, vous restez aux commandes.**
