---
tags:
  - reference
  - glossaire
---

# Glossaire

Définitions des termes techniques utilisés dans R-Type.

---

## A

### ASIO
**Asynchronous I/O** - Bibliothèque Boost pour les opérations asynchrones, notamment réseau. Utilisée pour la communication UDP.

### Autoritatif (Serveur)
Modèle où le serveur est la seule source de vérité pour l'état du jeu. Les clients envoient des inputs, le serveur valide et broadcast l'état.

---

## B

### Backend
Implémentation concrète d'une interface. R-Type supporte deux backends graphiques : SDL2 et SFML.

### Batch Rendering
Technique d'optimisation qui regroupe plusieurs opérations de dessin en une seule pour réduire les appels GPU.

---

## C

### Client-Side Prediction
Technique où le client simule localement l'effet de ses inputs sans attendre la confirmation du serveur, pour une meilleure réactivité.

### Composant (ECS)
Structure de données pure associée à une entité. Ex: Position, Velocity, Sprite.

---

## D

### Delta State
État différentiel envoyé par le serveur contenant uniquement les changements depuis le dernier état, pour économiser la bande passante.

### Draw Call
Appel à l'API graphique pour dessiner un élément. Minimiser les draw calls améliore les performances.

---

## E

### ECS
**Entity Component System** - Pattern architectural où :
- **Entity** = identifiant unique
- **Component** = données pures
- **System** = logique opérant sur les composants

### Entité
Identifiant unique dans le système ECS. N'a pas de données propres, juste un ID auquel sont attachés des composants.

---

## F

### Frame
Image complète rendue à l'écran. 60 FPS = 60 frames par seconde.

### Fullscreen
Mode d'affichage où le jeu occupe tout l'écran, sans bordures de fenêtre.

---

## G

### Game Loop
Boucle principale du jeu : Input → Update → Render → Repeat.

### Game State
État complet du monde de jeu à un instant donné : positions, scores, états des entités.

---

## H

### Hitbox
Zone de collision invisible autour d'une entité, utilisée pour la détection des collisions.

### Hot Reload
Rechargement de configuration ou ressources sans redémarrer l'application.

---

## I

### Interpolation
Technique de lissage entre deux états connus pour créer un mouvement fluide. Le client interpole entre les états reçus du serveur.

### Input Lag
Délai entre l'action du joueur et sa répercussion à l'écran.

---

## L

### Latence
Temps de trajet d'un paquet entre client et serveur. Mesurée en millisecondes.

### Lerp
**Linear Interpolation** - Interpolation linéaire entre deux valeurs : `lerp(a, b, t) = a + (b - a) * t`

---

## M

### Manifest (vcpkg)
Fichier `vcpkg.json` déclarant les dépendances du projet. vcpkg les installe automatiquement.

### Multi-backend
Architecture supportant plusieurs implémentations d'une même fonctionnalité (ex: SDL2 et SFML pour le rendu).

---

## P

### Packet
Unité de données envoyée sur le réseau. Contient un header et un payload.

### Plugin
Module chargé dynamiquement à l'exécution. Les backends graphiques sont des plugins.

### Port
Numéro identifiant un service réseau. R-Type utilise le port 4242 par défaut.

---

## R

### RAII
**Resource Acquisition Is Initialization** - Pattern C++ où les ressources sont acquises dans le constructeur et libérées dans le destructeur.

### Reconciliation
Processus de correction de la prédiction client quand l'état serveur arrive et diffère.

### Registry
Conteneur central gérant toutes les entités et leurs composants dans un ECS.

---

## S

### Serialization
Conversion de structures en flux d'octets pour transmission réseau ou stockage.

### Sprite
Image 2D représentant un élément graphique du jeu.

### System (ECS)
Logique opérant sur un ensemble de composants. Ex: MovementSystem met à jour les positions selon les vélocités.

---

## T

### Tick
Mise à jour du serveur. Le serveur tourne à 60 ticks/seconde = 16.67ms par tick.

### Tick Rate
Fréquence des mises à jour du serveur en Hertz.

---

## U

### UDP
**User Datagram Protocol** - Protocole réseau sans connexion, sans garantie de livraison, mais avec latence minimale. Idéal pour les jeux temps réel.

---

## V

### vcpkg
Gestionnaire de paquets C++ de Microsoft. Gère les dépendances du projet.

### View (ECS)
Itérateur sur les entités possédant un ensemble spécifique de composants.

### VSync
**Vertical Synchronization** - Synchronise le rendu avec le rafraîchissement de l'écran pour éviter le tearing.

---

## W

### Wrapper
Classe encapsulant une API externe pour l'adapter à notre code.
