---
tags:
  - guide
  - gameplay
  - friends
  - social
  - messaging
---

# Amis & Messages Prives

Gerez vos amis et communiquez par messages prives.

---

## Acceder au Systeme Social

Depuis le menu principal, cliquez sur **FRIENDS** pour ouvrir la scene d'amis.

---

## Gestion des Amis

### Envoyer une Demande d'Ami

1. Dans l'onglet **Ajouter**, entrez l'email du joueur
2. Cliquez sur **SEND REQUEST**
3. Le joueur recevra une notification en temps reel

### Accepter/Refuser une Demande

Dans l'onglet **Requests** :

- Cliquez sur **ACCEPT** pour accepter la demande
- Cliquez sur **REJECT** pour la refuser

### Supprimer un Ami

Dans l'onglet **Friends** :

- Cliquez sur le bouton **X** a cote de l'ami

---

## Statut en Ligne

| Indicateur | Signification |
|------------|---------------|
| Vert | En ligne |
| Gris | Hors ligne |

Les changements de statut sont notifies en temps reel.

---

## Messages Prives

### Envoyer un Message

1. Cliquez sur un ami dans la liste
2. Tapez votre message dans le champ de texte
3. Appuyez sur ++enter++ ou cliquez sur **SEND**

### Historique des Conversations

- L'historique est sauvegarde automatiquement
- Scrollez pour voir les anciens messages
- Les messages non lus sont marques

---

## Blocage

### Bloquer un Utilisateur

1. Allez dans l'onglet **Blocked**
2. Entrez l'email a bloquer
3. Cliquez sur **BLOCK**

Un utilisateur bloque ne peut pas :

- Vous envoyer de demande d'ami
- Vous envoyer de message prive
- Voir votre statut en ligne

### Debloquer

Dans l'onglet **Blocked**, cliquez sur **UNBLOCK** a cote de l'utilisateur.

---

## Notifications

Les notifications temps reel incluent :

| Notification | Description |
|--------------|-------------|
| Demande recue | Quelqu'un vous envoie une demande |
| Demande acceptee | Votre demande est acceptee |
| Nouveau message | Message prive recu |
| Ami en ligne | Un ami se connecte |

!!! info "Sans polling"
    Les notifications sont poussees via TCP sans necessiter de requetes periodiques.

---

## Documentation Technique

Pour les details d'implementation, voir [Friends System](../../friends-system.md).
