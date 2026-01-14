---
tags:
  - configuration
  - client
---

# Configuration Client

## Arguments CLI

```bash
./r-type_client [OPTIONS]
  -h, --host <HOST>     Serveur (127.0.0.1)
  -p, --port <PORT>     Port (4242)
  --name <NAME>         Pseudo
  --backend <BACKEND>   sdl2 | sfml
  -f, --fullscreen      Plein écran
  -v, --verbose         Mode debug
```

---

## Fichier config/client.json

```json
{
  "network": {
    "default_host": "127.0.0.1",
    "default_port": 4242
  },
  "graphics": {
    "backend": "sdl2",
    "resolution": { "width": 1920, "height": 1080 },
    "fullscreen": false,
    "vsync": true,
    "fps_limit": 144
  },
  "audio": {
    "master_volume": 80,
    "music_volume": 60,
    "sfx_volume": 100,
    "voice_volume": 100
  }
}
```

---

## Backends Graphiques

| Backend | Description |
|---------|-------------|
| `sdl2` | Défaut, performant |
| `sfml` | Alternative, haut niveau |

```bash
./r-type_client --backend sfml
# ou
RTYPE_BACKEND=sfml ./r-type_client
```

---

## Résolutions

| Résolution | Ratio |
|------------|-------|
| 1920x1080 | 16:9 (recommandé) |
| 2560x1440 | 16:9 |
| 1280x720 | 16:9 |

---

## Variables d'Environnement

| Variable | Description |
|----------|-------------|
| `RTYPE_BACKEND` | Backend graphique |
| `RTYPE_LOG_LEVEL` | Niveau de log |
| `RTYPE_CONFIG_DIR` | Dossier config |
