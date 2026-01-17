# Serveur VPS R-Type

Documentation du serveur de production R-Type heberge sur VPS.

## Infrastructure

| Element | Valeur |
|---------|--------|
| **OS** | Ubuntu 24.04 LTS |
| **Localisation** | France (OVH) |
| **Base de donnees** | MongoDB 7.0 |
| **Securite** | TLS 1.2+, SSH key-only |

## Ports reseau

| Port | Protocole | Description |
|------|-----------|-------------|
| 4125 | TCP | Authentification (TLS) |
| 4124 | UDP | Gameplay temps reel |
| 4126 | UDP | Voice chat (Opus) |

---

## Guide Developpeur

### Connexion SSH

```bash
ssh <username>@51.254.137.175
```

> L'authentification par mot de passe est desactivee. Contactez l'administrateur pour ajouter votre cle SSH publique.

### Gestion du service

Les membres de l'equipe peuvent controler le serveur sans privileges sudo :

```bash
# Voir le status
systemctl status rtype-server

# Demarrer / Arreter / Redemarrer
systemctl start rtype-server
systemctl stop rtype-server
systemctl restart rtype-server
```

### Consultation des logs

```bash
# Logs en temps reel
tail -f /opt/rtype/logs/server.log

# Dernieres erreurs
tail -100 /opt/rtype/logs/server-error.log

# Journal systemd
journalctl -u rtype-server -n 50 --no-pager
```

### Structure du projet

```
/opt/rtype/
├── server/           # Binaire serveur
│   └── certs/        # Certificats TLS
├── logs/             # Fichiers de log
│   ├── server.log
│   └── server-error.log
└── repo/             # Clone du repository (auto-deploy)
```

---

## Deploiement

### Deploiement automatique

Le serveur se redéploie automatiquement a chaque push sur la branche `main` :

1. Detection du nouveau commit (check toutes les minutes)
2. Pull des modifications
3. Build avec CMake + Ninja
4. Redemarrage du service
5. Notification Discord

### Deploiement manuel

```bash
# Depuis votre machine locale
scp artifacts/server/linux/rtype_server <user>@51.254.137.175:/opt/rtype/server/

# Sur le VPS
chmod +x /opt/rtype/server/rtype_server
systemctl restart rtype-server
```

---

## Guide Joueur

### Connexion au serveur

#### Via ligne de commande

```bash
# Serveur de production (France)
./scripts/run-client.sh --server=51.254.137.175

# Serveur local (developpement)
./scripts/run-client.sh --server=127.0.0.1
```

#### Via l'interface du jeu

1. Sur l'ecran de connexion, appuyez sur **S** ou cliquez sur **SERVER**
2. Selectionnez un preset :
   - **FRANCE** : Serveur de production (`51.254.137.175`)
   - **LOCAL** : Serveur local (`127.0.0.1`)
3. Ou entrez manuellement l'adresse et les ports

### Configuration persistante

La configuration du serveur est sauvegardee automatiquement :

- **Linux** : `~/.config/rtype/rtype_client.json`
- **Windows** : `%APPDATA%/RType/rtype_client.json`

---

## Securite

- **SSH** : Authentification par cle uniquement (mot de passe desactive)
- **TLS** : Chiffrement des communications d'authentification (port 4125)
- **Firewall** : UFW configure avec uniquement les ports necessaires ouverts
- **Service** : Execution avec privileges restreints (systemd hardening)

---

## Maintenance

### Redemarrage planifie

Le serveur redémarre automatiquement chaque jour a 6h00 pour garantir la stabilite.

### Backups

Sauvegarde automatique quotidienne a 3h00.

### Monitoring

Notifications Discord automatiques pour :
- Demarrage du serveur
- Arret du serveur
- Crash et redemarrage automatique
- Deploiements (succes/echec)

---

## Depannage

### Le serveur ne repond pas

```bash
# Verifier le status
systemctl status rtype-server

# Voir les dernieres erreurs
journalctl -u rtype-server -n 100 --no-pager

# Redemarrer
systemctl restart rtype-server
```

### Impossible de se connecter en SSH

1. Verifiez que votre cle SSH est ajoutee sur le serveur
2. Testez avec : `ssh -v <username>@51.254.137.175`
3. Contactez l'administrateur si le probleme persiste

### Le client ne se connecte pas

1. Verifiez que le serveur est en ligne : `systemctl status rtype-server`
2. Verifiez les ports dans la configuration client
3. Consultez les logs serveur pour les erreurs de connexion
