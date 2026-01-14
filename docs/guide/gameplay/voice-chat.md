---
tags:
  - guide
  - gameplay
  - voice-chat
---

# Voice Chat

Communiquez vocalement avec votre équipe.

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

```json
{
  "audio": {
    "voice": {
      "mode": "ptt",
      "ptt_key": "V",
      "vad_threshold": 0.02,
      "input_device": "default",
      "output_device": "default"
    }
  }
}
```

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
