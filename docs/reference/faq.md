---
tags:
  - reference
  - faq
---

# FAQ

Questions fréquemment posées sur R-Type.

---

## Installation & Build

??? question "Quelle version de GCC/Clang est requise ?"
    R-Type utilise C++23. Vous avez besoin de :

    - **GCC 11+** (recommandé: GCC 13)
    - **Clang 15+** (recommandé: Clang 17)

    ```bash
    # Vérifier la version
    g++ --version
    clang++ --version
    ```

??? question "Comment installer vcpkg ?"
    ```bash
    git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
    cd ~/vcpkg
    ./bootstrap-vcpkg.sh
    export VCPKG_ROOT="$HOME/vcpkg"
    ```

    Ajoutez `export VCPKG_ROOT="$HOME/vcpkg"` à votre `~/.bashrc` pour la persistance.

??? question "Le build échoue avec 'C++23 features not available'"
    Votre compilateur est trop ancien. Mettez-le à jour :

    ```bash
    # Ubuntu
    sudo apt install gcc-13 g++-13 clang
    ```

    Puis relancez la configuration :

    ```bash
    rm -rf buildLinux
    ./scripts/build.sh
    ```

??? question "vcpkg ne trouve pas les dépendances"
    Le script `build.sh` gère automatiquement vcpkg. Si vous rencontrez des problèmes, nettoyez et recommencez :

    ```bash
    rm -rf third_party/vcpkg buildLinux
    ./scripts/build.sh
    ```

---

## Gameplay

??? question "Comment changer le backend graphique ?"
    Trois méthodes :

    1. **CLI** : `./r-type_client --backend=sfml`
    2. **Config** : Éditez `config/client.json` : `"backend": "sfml"`
    3. **Environnement** : `RTYPE_BACKEND=sfml ./r-type_client`

??? question "Comment jouer en multijoueur ?"
    1. **Hôte** : Lance le serveur avec `./r-type_server`
    2. **Joueurs** : Connectent avec `./r-type_client -h <IP_HOTE>`

    L'hôte doit partager son IP publique et ouvrir le port 4242/UDP.

??? question "Le jeu lag beaucoup"
    Vérifiez :

    1. **Latence réseau** : Utilisez `ping <server_ip>` (< 50ms idéal)
    2. **VSync** : Activez dans `config/client.json`
    3. **Backend** : SDL2 est généralement plus performant que SFML
    4. **Mode Release** : Compilez en Release, pas Debug

??? question "Les contrôles ne répondent pas"
    Vérifiez que la fenêtre a le focus. Sur certains window managers Linux, cliquez dans la fenêtre.

    Pour les manettes, assurez-vous qu'elles sont détectées :
    ```bash
    ls /dev/input/js*
    ```

---

## Réseau

??? question "Quel port utilise R-Type ?"
    - **Port par défaut** : 4242 (UDP)
    - **Console debug** : 4243 (TCP)

    Modifiable via `-p` ou dans la configuration.

??? question "Comment ouvrir les ports sur mon routeur ?"
    1. Accédez à l'interface admin de votre routeur (souvent `192.168.1.1`)
    2. Trouvez "Port Forwarding" ou "NAT"
    3. Ajoutez une règle : Port 4242, Protocol UDP, vers votre IP locale

    Consultez la documentation de votre routeur pour les détails.

??? question "Pourquoi UDP et pas TCP ?"
    UDP offre une **latence minimale** car :

    - Pas d'attente d'ACK
    - Pas de retransmission automatique
    - Pas de congestion control agressif

    Pour un jeu action, perdre un paquet est préférable à attendre sa retransmission. L'état suivant corrige naturellement.

---

## Développement

??? question "Comment ajouter un nouveau composant ECS ?"
    ```cpp
    // 1. Définir le composant (struct de données)
    struct MyComponent {
        int value;
        float timer;
    };

    // 2. L'utiliser
    auto entity = registry.create();
    registry.emplace<MyComponent>(entity, 42, 0.f);

    // 3. Y accéder dans un système
    for (auto [e, comp] : registry.view<MyComponent>()) {
        comp.timer += dt;
    }
    ```

??? question "Comment débugger le réseau ?"
    Activez les logs verbeux :
    ```bash
    RTYPE_LOG_LEVEL=debug ./r-type_server -v
    ```

    Ou utilisez Wireshark pour capturer le trafic UDP sur le port 4242.

??? question "Comment créer un nouveau backend graphique ?"
    1. Implémentez l'interface `IGraphicsBackend`
    2. Créez les classes `IWindow`, `ITexture`, etc.
    3. Utilisez la macro d'enregistrement :
    ```cpp
    REGISTER_GRAPHICS_BACKEND("mybackend", MyBackend);
    ```

---

## Problèmes Courants

??? question "Erreur : 'SDL2/SDL.h' not found"
    SDL2 n'est pas installé. Via vcpkg :
    ```bash
    vcpkg install sdl2 sdl2-image sdl2-ttf
    ```

    Ou via apt :
    ```bash
    sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
    ```

??? question "Erreur : 'undefined reference to boost::asio'"
    Boost n'est pas linkée. Vérifiez `CMakeLists.txt` :
    ```cmake
    find_package(Boost REQUIRED COMPONENTS system)
    target_link_libraries(${PROJECT_NAME} Boost::system)
    ```

??? question "Le client crashe au démarrage"
    1. Vérifiez que les assets existent dans `assets/`
    2. Lancez en mode debug pour voir l'erreur
    3. Vérifiez les logs dans `logs/client.log`

??? question "Segfault dans le système ECS"
    Causes fréquentes :

    1. Accès à une entité détruite
    2. Modification du registry pendant l'itération
    3. Composant non initialisé

    Utilisez AddressSanitizer pour débugger. Le script `build.sh` configure automatiquement les sanitizers en mode Debug.
