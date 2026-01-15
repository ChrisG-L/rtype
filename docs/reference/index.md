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
# Serveur (configuration via .env uniquement)
./rtype_server
# Voir docs/configuration/serveur.md pour les options .env

# Client
./rtype_client [OPTIONS]
  --graphics=<name>       Backend graphique (sdl2 ou sfml, défaut: sfml)
  --graphics-path=<path>  Chemin vers un plugin graphique custom
  -h, --help              Afficher l'aide
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
| Authentification (TLS) | 4125 | TCP |
| Game Server | 4124 | UDP |
| Voice Chat | 4126 | UDP |

### Variables d'Environnement

| Variable | Description |
|----------|-------------|
| `MONGODB_URI` | URI de connexion MongoDB |
| `MONGODB_DB` | Nom de la base de données |
| `TLS_CERT_FILE` | Chemin certificat TLS |
| `TLS_KEY_FILE` | Chemin clé privée TLS |
| `VCPKG_ROOT` | Chemin vers vcpkg |
