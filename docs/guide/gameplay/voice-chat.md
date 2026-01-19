---
tags:
  - guide
  - gameplay
  - voice-chat
---

# Voice Chat

Communiquez vocalement avec votre équipe.

!!! warning "Linux : Utilisez le script de lancement"
    Sur Linux avec PipeWire (Ubuntu 22.04+, Fedora, etc.), **utilisez toujours** le script wrapper :
    ```bash
    ./scripts/run-client.sh --server=51.254.137.175
    ```
    Ce script active automatiquement le support audio JACK via `pw-jack`. Sans lui, le voice chat ne fonctionnera pas.

## Technologies

- **Opus** : Codec audio haute qualité
- **PortAudio** : Capture cross-platform
- **UDP** : Transport temps réel (port 4126)

---

## Prérequis Linux (PipeWire)

Sur les distributions Linux modernes utilisant PipeWire (Ubuntu 22.04+, Fedora, etc.), vous devez installer les paquets audio et utiliser le script de lancement dédié.

### Installation des dépendances audio

```bash
sudo apt install pulseaudio pulseaudio-utils libpulse0 pipewire pipewire-pulse pipewire-jack pipewire-bin
```

### Activation de PipeWire

```bash
systemctl --user enable --now pipewire pipewire-pulse wireplumber
```

### Lancement du client

Utilisez le script `run-client.sh` qui gère automatiquement le wrapper JACK pour PipeWire :

```bash
./scripts/run-client.sh
```

Ce script :

- Détecte si PipeWire est actif
- Utilise automatiquement `pw-jack` pour la compatibilité audio
- Passe tous les arguments au client (ex: `./scripts/run-client.sh --graphics=sdl2`)

!!! tip "Pourquoi pw-jack ?"
    PortAudio utilise JACK pour l'audio. Sur les systèmes PipeWire, le wrapper `pw-jack` permet à JACK de fonctionner via PipeWire sans configuration supplémentaire.

---

## Modes

### Push-to-Talk (PTT)

Maintenez ++v++ pour parler.

| ✅ Avantages | ❌ Inconvénients |
|-------------|-----------------|
| Contrôle total | Nécessite une touche |
| Pas de bruit | Moins naturel |

### Voice Activity Detection (VAD)

Parlez librement, détection automatique.

| ✅ Avantages | ❌ Inconvénients |
|-------------|-----------------|
| Mains libres | Peut capter du bruit |
| Plus naturel | Bon micro requis |

---

## Configuration

Les paramètres audio sont configurables via le menu **Paramètres** du jeu (section Audio).

### Paramètres disponibles

| Paramètre | Description | Valeur par défaut |
|-----------|-------------|-------------------|
| **Mode vocal** | PTT ou VAD | Push-to-Talk |
| **Touche PTT** | Touche pour parler | ++v++ |
| **Seuil VAD** | Sensibilité de détection | 0.02 |
| **Périphérique d'entrée** | Microphone | Défaut système |
| **Périphérique de sortie** | Haut-parleurs/Casque | Défaut système |
| **Gain micro** | Volume d'entrée (0.0-2.0) | 1.0 |

!!! info "Persistance des paramètres"
    Les préférences audio (périphériques sélectionnés) sont sauvegardées dans votre compte utilisateur (MongoDB côté serveur).

### Seuil VAD

| Valeur | Sensibilité | Environnement |
|--------|-------------|---------------|
| 0.01 | Très haute | Très silencieux |
| 0.02 | Haute (défaut) | Silencieux |
| 0.05 | Moyenne | Normal |
| 0.1 | Basse | Bruyant |

---

## Dépannage

??? question "Micro non détecté"
    ```bash
    # Linux - lister les périphériques
    arecord -l

    # Tester le micro
    arecord -d 5 test.wav && aplay test.wav
    ```

??? question "Les autres ne m'entendent pas"
    - Vérifiez que vous n'êtes pas mute
    - PTT : maintenez bien la touche
    - VAD : baissez le seuil

??? question "Écho"
    - Utilisez un casque
    - Baissez le volume de sortie
