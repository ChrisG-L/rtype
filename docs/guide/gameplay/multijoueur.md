---
tags:
  - guide
  - gameplay
  - multijoueur
---

# Mode Multijoueur

Jouez jusqu'à 4 joueurs en coopération !

## Architecture

```mermaid
flowchart TB
    subgraph Serveur
        S[R-Type Server]
        R1[Room 1]
        R2[Room 2]
    end

    P1[Joueur 1] & P2[Joueur 2] --> R1
    P3[Joueur 3] & P4[Joueur 4] --> R2
    R1 & R2 --> S

    style S fill:#7c3aed,color:#fff
```

---

## Rejoindre une Partie

```bash
./r-type_client -h <IP_SERVEUR> -p 4242 --name "MonPseudo"
```

Ou via le menu : **Multijoueur → Rejoindre**

---

## Héberger une Partie

1. Lancez le serveur :
```bash
./r-type_server -p 4242
```

2. Partagez votre IP :
```bash
curl ifconfig.me  # IP publique
```

3. Les joueurs se connectent avec votre IP

---

## Ports à Ouvrir

| Port | Protocole | Usage |
|------|-----------|-------|
| 4242 | TCP | Auth, rooms, chat |
| 4242 | UDP | Game sync |
| 4243 | UDP | Voice chat |

---

## Indicateurs Réseau

| Icône | Ping | État |
|-------|------|------|
| 🟢 | < 50ms | Excellent |
| 🟡 | 50-100ms | Bon |
| 🟠 | 100-200ms | Moyen |
| 🔴 | > 200ms | Mauvais |

---

## Dépannage

??? question "Impossible de se connecter"
    - Vérifiez l'IP et le port
    - Testez : `nc -vz <IP> 4242`
    - Vérifiez le pare-feu

??? question "Les autres ne peuvent pas me rejoindre"
    - Configurez le port forwarding sur votre routeur
    - Utilisez votre IP publique (pas locale)
