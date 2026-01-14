---
tags:
  - reference
---

# Référence

Ressources de référence rapide pour R-Type.

<div class="grid-cards">
  <div class="card">
    <div class="card-icon">📖</div>
    <h3><a href="glossaire/">Glossaire</a></h3>
    <p>Définitions des termes techniques.</p>
  </div>

  <div class="card">
    <div class="card-icon">❓</div>
    <h3><a href="faq/">FAQ</a></h3>
    <p>Questions fréquemment posées.</p>
  </div>

  <div class="card">
    <div class="card-icon">📋</div>
    <h3><a href="changelog/">Changelog</a></h3>
    <p>Historique des versions.</p>
  </div>

  <div class="card">
    <div class="card-icon">🏷️</div>
    <h3><a href="tags/">Tags</a></h3>
    <p>Navigation par tags.</p>
  </div>
</div>

---

## Liens Rapides

### Commandes CLI

```bash
# Serveur
./r-type_server [OPTIONS]
  -p, --port <PORT>     Port d'écoute (default: 4242)
  -c, --config <FILE>   Fichier de configuration
  -v, --verbose         Mode verbeux

# Client
./r-type_client [OPTIONS]
  -h, --host <HOST>     Adresse du serveur (default: 127.0.0.1)
  -p, --port <PORT>     Port du serveur (default: 4242)
  --backend <BACKEND>   Backend graphique (sdl2|sfml)
  -f, --fullscreen      Mode plein écran
```

### Raccourcis Clavier

| Touche | Action |
|--------|--------|
| ++arrow-up++ / ++w++ | Haut |
| ++arrow-down++ / ++s++ | Bas |
| ++arrow-left++ / ++a++ | Gauche |
| ++arrow-right++ / ++d++ | Droite |
| ++space++ | Tir |
| ++escape++ | Pause |
| ++f11++ | Plein écran |
| ++ctrl+r++ | Recharger config |

### Ports par Défaut

| Service | Port | Protocol |
|---------|------|----------|
| Game Server | 4242 | UDP |
| Debug Console | 4243 | TCP |

### Variables d'Environnement

| Variable | Description |
|----------|-------------|
| `RTYPE_CONFIG_DIR` | Dossier des configurations |
| `RTYPE_LOG_LEVEL` | Niveau de log |
| `RTYPE_BACKEND` | Backend graphique |
| `VCPKG_ROOT` | Chemin vers vcpkg |
