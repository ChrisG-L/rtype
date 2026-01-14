# Combat System - R-Type

Documentation complète du systeme de combat, incluant les degats, HP, hitboxes, et statistiques de toutes les entites du jeu.

## Table des Matieres

- [Vue d'ensemble](#vue-densemble)
- [Joueur](#joueur)
- [Armes du Joueur](#armes-du-joueur)
- [Wave Cannon (Charge)](#wave-cannon-charge)
- [Force Pod](#force-pod)
- [Power-Ups](#power-ups)
- [Ennemis](#ennemis)
- [Boss](#boss)
- [Systeme de Score](#systeme-de-score)
- [Systeme de Vagues](#systeme-de-vagues)
- [Formules et Calculs](#formules-et-calculs)
- [Hitboxes](#hitboxes)
- [References Code](#references-code)

---

## Vue d'ensemble

### Philosophie d'equilibrage

Le systeme de combat est equilibre autour de ces principes :

1. **DPS normalise** : Toutes les armes ont un DPS similaire (~65-70 dps)
2. **Choix tactique** : Chaque arme a un cas d'usage optimal
3. **Progression naturelle** : Les ennemis deviennent plus difficiles avec les vagues
4. **Boss epique** : Combat de ~22 secondes avec 3 phases distinctes

### Constantes globales

| Constante | Valeur | Description |
|-----------|--------|-------------|
| Ecran | 1920 x 1080 px | Resolution de reference |
| Tick rate serveur | 60 Hz | Mise a jour physique |
| Broadcast rate | 20 Hz | Envoi snapshots reseau |

---

## Joueur

### Statistiques de base

| Stat | Valeur | Notes |
|------|--------|-------|
| **HP Maximum** | 100 | Point de vie |
| **HP Depart** | 100 | Full HP au spawn |
| **Vitesse de base** | 200 px/s | Modifie par Speed Power-up |
| **Vitesse max** | ~380 px/s | Avec 3 Speed Power-ups |

### Hitbox

| Dimension | Valeur |
|-----------|--------|
| Largeur | 64 px |
| Hauteur | 30 px |

### Niveaux de vitesse

| Niveau | Multiplicateur | Vitesse effective |
|--------|----------------|-------------------|
| 0 (base) | 1.0x | 200 px/s |
| 1 | 1.3x | 260 px/s |
| 2 | 1.6x | 320 px/s |
| 3 (max) | 1.9x | 380 px/s |

**Note :** Collecter un SpeedUp au niveau max donne +500 points bonus.

### Sources de degats au joueur

| Source | Degats | Survie |
|--------|--------|--------|
| Missile ennemi | 20 HP | 5 hits |
| Contact ennemi | 20 HP | 5 hits |
| Attaque Boss | 20 HP | 5 hits |

---

## Armes du Joueur

Le joueur dispose de 4 types d'armes, selectionnables avec Q/E (par defaut).

### Tableau comparatif complet

| Arme | Degats | Vitesse | Cooldown | DPS | Projectiles | Special |
|------|--------|---------|----------|-----|-------------|---------|
| **Standard** | 20 | 600 px/s | 0.30s | 66.7 | 1 | Fiable |
| **Spread** | 8 | 550 px/s | 0.40s | 60.0 | 3 | Eventail |
| **Laser** | 12 | 900 px/s | 0.18s | 66.7 | 1 | Rapide |
| **Homing** | 50 | 350 px/s | 0.70s | 71.4 | 1 | Auto-guide |

### Hitbox des projectiles

| Type | Largeur | Hauteur |
|------|---------|---------|
| Tous les missiles | 16 px | 8 px |

---

## Systeme de Niveaux d'Armes

Ramasser un **Power-Up WPN (WeaponCrystal)** augmente le niveau de l'arme de +1 (max niveau 3).
Chaque niveau ameliore les stats de TOUTES les armes du joueur.

### Bonus par Niveau

| Niveau | Degats | Cooldown | Vitesse | Visuel HUD |
|--------|--------|----------|---------|------------|
| **Lv.0** | x1.00 | x1.00 | x1.00 | [□][□][□] |
| **Lv.1** | x1.15 (+15%) | x0.95 (-5%) | x1.00 | [■][□][□] Lv.1 |
| **Lv.2** | x1.30 (+30%) | x0.90 (-10%) | x1.00 | [■][■][□] Lv.2 |
| **Lv.3** | x1.50 (+50%) | x0.85 (-15%) | x1.10 (+10%) | [■][■][■] MAX |

### Stats par Arme et Niveau

#### Standard

| Niveau | Degats | Cooldown | Vitesse | DPS |
|--------|--------|----------|---------|-----|
| Lv.0 | 20 | 0.30s | 600 px/s | 66.7 |
| Lv.1 | 23 | 0.285s | 600 px/s | 80.7 |
| Lv.2 | 26 | 0.27s | 600 px/s | 96.3 |
| Lv.3 | 30 | 0.255s | 660 px/s | 117.6 |

#### Spread (par projectile)

| Niveau | Degats | Cooldown | Vitesse | DPS (x3) |
|--------|--------|----------|---------|----------|
| Lv.0 | 8 | 0.40s | 550 px/s | 60.0 |
| Lv.1 | 9 | 0.38s | 550 px/s | 71.1 |
| Lv.2 | 10 | 0.36s | 550 px/s | 83.3 |
| Lv.3 | 12 | 0.34s | 605 px/s | 105.9 |

#### Laser

| Niveau | Degats | Cooldown | Vitesse | DPS |
|--------|--------|----------|---------|-----|
| Lv.0 | 12 | 0.18s | 900 px/s | 66.7 |
| Lv.1 | 13 | 0.171s | 900 px/s | 76.0 |
| Lv.2 | 15 | 0.162s | 900 px/s | 92.6 |
| Lv.3 | 18 | 0.153s | 990 px/s | 117.6 |

#### Homing

| Niveau | Degats | Cooldown | Vitesse | DPS |
|--------|--------|----------|---------|-----|
| Lv.0 | 50 | 0.70s | 350 px/s | 71.4 |
| Lv.1 | 57 | 0.665s | 350 px/s | 85.7 |
| Lv.2 | 65 | 0.63s | 350 px/s | 103.2 |
| Lv.3 | 75 | 0.595s | 385 px/s | 126.1 |

### Comparaison DPS Lv.0 vs Lv.3

| Arme | DPS Lv.0 | DPS Lv.3 | Augmentation |
|------|----------|----------|--------------|
| Standard | 66.7 | 117.6 | +76% |
| Spread | 60.0 | 105.9 | +76% |
| Laser | 66.7 | 117.6 | +76% |
| Homing | 71.4 | 126.1 | +77% |

### Temps pour tuer le Boss (1500 HP)

| Arme | Lv.0 | Lv.3 |
|------|------|------|
| Standard | ~22.5s | ~12.8s |
| Spread | ~25.0s | ~14.2s |
| Laser | ~22.5s | ~12.8s |
| Homing | ~21.0s | ~11.9s |

### Notes importantes

- Le niveau d'arme affecte TOUTES les armes simultanement
- Le bonus de vitesse (+10%) n'est disponible qu'au niveau 3
- Les niveaux sont perdus a la mort du joueur
- Le HUD affiche le niveau avec des barres colorees (jaune → orange → magenta)

---

### Standard

```
Type: Standard (WeaponType::Standard)
Degats: 20
Vitesse: 600 px/s
Cooldown: 0.3s
DPS: 66.7
```

**Caracteristiques :**
- Arme de base, aucun prerequis
- Trajectoire droite horizontale
- Equilibre entre degats et cadence

**Efficacite :**
| Cible | HP | Tirs pour kill |
|-------|-----|----------------|
| Fast | 20 | 1 |
| Zigzag | 25 | 2 |
| Tracker | 35 | 2 |
| Basic | 40 | 2 |
| Bomber | 80 | 4 |
| Boss | 1500 | 75 |

---

### Spread

```
Type: Spread (WeaponType::Spread)
Degats: 8 par projectile
Vitesse: 550 px/s
Cooldown: 0.4s
DPS: 60.0 (si 3 touchent)
Projectiles: 3
Angle: -15°, 0°, +15°
```

**Caracteristiques :**
- 3 projectiles en eventail
- Ideal contre groupes d'ennemis
- DPS reduit sur cible unique (20 dps)

**Efficacite (3 touches) :**
| Cible | HP | Salves pour kill |
|-------|-----|------------------|
| Fast | 20 | 1 |
| Zigzag | 25 | 2 |
| Tracker | 35 | 2 |
| Basic | 40 | 2 |
| Bomber | 80 | 4 |
| Boss | 1500 | 63 |

**Angle de dispersion :**
```
    projectile 1 (+15°)
       /
Player ----> projectile 2 (0°)
       \
    projectile 3 (-15°)
```

---

### Laser

```
Type: Laser (WeaponType::Laser)
Degats: 12
Vitesse: 900 px/s
Cooldown: 0.18s
DPS: 66.7
```

**Caracteristiques :**
- Cadence de tir tres rapide
- Projectiles les plus rapides
- Degats faibles par tir, mais constant
- Ideal pour maintenir la pression

**Efficacite :**
| Cible | HP | Tirs pour kill |
|-------|-----|----------------|
| Fast | 20 | 2 |
| Zigzag | 25 | 3 |
| Tracker | 35 | 3 |
| Basic | 40 | 4 |
| Bomber | 80 | 7 |
| Boss | 1500 | 125 |

---

### Homing (Missile Guide)

```
Type: Homing (WeaponType::Missile)
Degats: 50
Vitesse: 350 px/s
Cooldown: 0.7s
DPS: 71.4
Guidage: Auto vers ennemi le plus proche
```

**Caracteristiques :**
- S'oriente automatiquement vers la cible
- Priorite : Boss > Ennemi le plus proche
- Ideal pour debutants ou situations difficiles
- Plus haut DPS de toutes les armes

**Efficacite :**
| Cible | HP | Missiles pour kill |
|-------|-----|-------------------|
| Fast | 20 | 1 |
| Zigzag | 25 | 1 |
| Tracker | 35 | 1 |
| Basic | 40 | 1 |
| Bomber | 80 | 2 |
| Boss | 1500 | 30 |

**Algorithme de guidage :**
1. Detecte l'ennemi/boss le plus proche
2. Calcule l'angle vers la cible
3. Ajuste la trajectoire a chaque frame
4. Continue jusqu'a impact ou sortie d'ecran

---

## Wave Cannon (Charge)

Le Wave Cannon est une attaque chargee. Maintenez la touche de tir (Space par defaut) pour charger, relachez pour tirer.

### Niveaux de charge

| Niveau | Temps min | Degats | Largeur | Pierce | Couleur |
|--------|-----------|--------|---------|--------|---------|
| **Lv1** | 0.6s | 50 | 20 px | Non | Bleu clair |
| **Lv2** | 1.3s | 100 | 35 px | Oui | Bleu |
| **Lv3** | 2.2s | 200 | 55 px | Oui | Bleu intense |

### Caracteristiques

| Stat | Valeur |
|------|--------|
| Vitesse du beam | 850 px/s |
| Longueur du beam | 60 px |

### Mecanique de Pierce

- **Lv1** : Le beam est detruit au premier contact
- **Lv2/Lv3** : Le beam traverse les ennemis
- **Important** : Chaque ennemi n'est touche qu'**une seule fois** par beam

### Timeline de charge

```
0.0s -------- 0.6s -------- 1.3s -------- 2.2s
  |            |             |             |
Debut      Lv1 pret     Lv2 pret      Lv3 pret
charge     (50 dmg)     (100 dmg)     (200 dmg)
```

### Efficacite

| Cible | HP | Lv1 | Lv2 | Lv3 |
|-------|-----|-----|-----|-----|
| Fast | 20 | 1 | 1 | 1 |
| Zigzag | 25 | 1 | 1 | 1 |
| Tracker | 35 | 1 | 1 | 1 |
| Basic | 40 | 1 | 1 | 1 |
| Bomber | 80 | 2 | 1 | 1 |
| Boss | 1500 | 30 | 15 | 8 |

### DPS theorique

| Niveau | Calcul | DPS |
|--------|--------|-----|
| Lv1 | 50 / 0.6 | 83.3 |
| Lv2 | 100 / 1.3 | 76.9 |
| Lv3 | 200 / 2.2 | 90.9 |

**Note** : Le DPS reel est inferieur car il faut compter le temps de charge + temps de vol du beam.

### Situations optimales

| Situation | Niveau recommande | Raison |
|-----------|-------------------|--------|
| Ennemi isole rapide | Lv1 | Charge rapide, overkill ok |
| Groupe aligne | Lv2/Lv3 | Pierce traverse tout |
| Boss | Lv3 | Maximum burst damage |
| Urgence | Lv1 | Mieux que rien |

---

## Force Pod

Le Force Pod est un compagnon qui peut etre attache au vaisseau ou lance de maniere autonome.

### Statistiques

| Stat | Valeur |
|------|--------|
| Degats contact | 45 |
| Largeur | 32 px |
| Hauteur | 32 px |
| Vitesse (detache) | 400 px/s |
| Offset attache | 50 px devant le vaisseau |

### Modes

| Mode | Touche | Comportement |
|------|--------|--------------|
| **Attache** | F (toggle) | Suit le vaisseau, protege l'avant |
| **Detache** | F (toggle) | Avance continuellement, revient par la gauche |

### Comportement detache

Quand le Force Pod est detache :
- Il avance a 400 px/s en ligne droite
- Une fois sorti de l'ecran a droite, il reapparait a gauche
- Il continue de faire des degats au contact (cooldown 0.5s par ennemi)
- Appuyer sur F le re-attache au vaisseau

### Efficacite

| Ennemi | HP | Contacts pour kill |
|--------|-----|-------------------|
| Fast | 20 | 1 |
| Zigzag | 25 | 1 |
| Tracker | 35 | 1 |
| Basic | 40 | 1 |
| Bomber | 80 | 2 |

### Tir du Force Pod

Quand le joueur tire, le Force Pod attache tire egalement :
- Cooldown: 0.35s (legerement plus lent que le joueur)
- Tire dans la meme direction que le joueur
- Utilise le meme type d'arme que le joueur

### Obtention

- Drop du Power-up "Force Pod" (15% chance sur kill ennemi)
- Garanti sur POW Armor (ennemi special tous les 25s)

---

## Bit Devices

Les Bit Devices sont 2 petits satellites qui orbitent autour du vaisseau et tirent automatiquement.

### Statistiques

| Stat | Valeur |
|------|--------|
| Quantite | 2 par joueur |
| Degats contact | 20 |
| Largeur | 24 px |
| Hauteur | 24 px |
| Rayon orbite | 50 px |
| Vitesse orbite | 3 rad/s (~0.5 tour/s) |

### Comportement

| Caracteristique | Description |
|-----------------|-------------|
| **Orbite** | Tournent continuellement autour du vaisseau |
| **Position** | Commencent a 180° l'un de l'autre (haut/bas) |
| **Tir** | Tirent automatiquement quand le joueur tire |
| **Direction tir** | Toujours vers la droite (horizontal) |

### Tir des Bits

| Stat | Valeur |
|------|--------|
| Cooldown | 0.4s |
| Direction | Horizontale (vers la droite) |
| Arme | Meme type que le joueur |
| Niveau | Meme niveau d'arme que le joueur |

### Degats de contact

Les Bits infligent des degats au contact avec les ennemis :

| Ennemi | HP | Contacts pour kill |
|--------|-----|-------------------|
| Fast | 20 | 1 |
| Zigzag | 25 | 2 |
| Tracker | 35 | 2 |
| Basic | 40 | 2 |
| Bomber | 80 | 4 |

### Comparaison Force Pod vs Bit Devices

| Caracteristique | Force Pod | Bit Devices |
|-----------------|-----------|-------------|
| Quantite | 1 | 2 |
| Taille | 32×32 px | 24×24 px |
| Degats contact | 45 | 20 |
| Mouvement | Attache/detache | Orbite fixe |
| Tir | Oui (0.35s cd) | Oui (0.4s cd) |
| Detachable | Oui (touche F) | Non |
| Couleur | Orange | Violet |

### Obtention

- Drop du Power-up "Bit Device" (15% chance sur kill ennemi)
- Un seul set de Bits par joueur (2 Bits)
- Peut etre combine avec Force Pod

---

## Power-Ups

Les power-ups apparaissent quand un ennemi est tue (15% de chance) ou garantis sur certains ennemis.

### Types de Power-Ups

| Type | Effet | Duree | Chance | Couleur |
|------|-------|-------|--------|---------|
| **Health** | +25 HP | Instantane | 25% | Vert |
| **Speed Up** | +1 niveau vitesse | Permanent | 20% | Bleu |
| **Weapon Crystal** | +1 niveau arme | Permanent | 25% | Rouge |
| **Force Pod** | Donne un Force Pod | Permanent | 15% | Orange |
| **Bit Device** | Donne 2 Bits orbitants | Permanent | 15% | Violet |

### Caracteristiques communes

| Stat | Valeur |
|------|--------|
| Taille | 24 x 24 px |
| Duree de vie | 10 secondes |
| Derive | -30 px/s (vers la gauche) |

### Effets detailles

#### Health
- Restaure 25 HP
- Ne depasse pas 100 HP max
- Effet : `player.health = min(100, health + 25)`

#### Speed Up
- Augmente le niveau de vitesse de 1
- Maximum : niveau 3 (1.3x vitesse)
- Effet : `player.speedLevel = min(3, speedLevel + 1)`

#### Weapon Crystal
- Augmente le niveau d'arme de 1
- Maximum : niveau 3
- Effet : `player.weaponLevel = min(3, weaponLevel + 1)`

#### Force Pod
- Donne un Force Pod au joueur
- Un seul Force Pod par joueur
- Si deja possede : pas d'effet supplementaire

#### Bit Device
- Donne 2 Bit Devices orbitants au joueur
- Un seul set de Bits par joueur
- Si deja possede : pas d'effet supplementaire
- Peut etre combine avec Force Pod

### POW Armor

Ennemi special qui spawn tous les **25 secondes**.

| Stat | Valeur |
|------|--------|
| HP | 50 |
| Vitesse X | -90 px/s |
| Tir Interval | 4.0s |
| Points | 200 |
| Drop garanti | 100% Power-Up |

---

## Ennemis

### Tableau complet des ennemis

| Type | HP | Vitesse X | Tir Interval | Points | Comportement |
|------|-----|-----------|--------------|--------|--------------|
| **Basic** | 40 | -120 px/s | 2.5s | 100 | Sinusoidal vertical |
| **Tracker** | 35 | -100 px/s | 2.0s | 150 | Suit le joueur en Y |
| **Zigzag** | 25 | -140 px/s | 3.0s | 120 | Zigzag brusque |
| **Fast** | 20 | -220 px/s | 1.5s | 180 | Sinusoidal rapide et serre |
| **Bomber** | 80 | -80 px/s | 1.0s | 250 | Sinusoidal lent + derive bas |
| **POW Armor** | 50 | -90 px/s | 4.0s | 200 | Sinusoidal (100% drop) |

### Hitbox commune

| Dimension | Valeur |
|-----------|--------|
| Largeur | 40 px |
| Hauteur | 40 px |

### Constantes de mouvement

| Constante | Valeur | Utilise par |
|-----------|--------|-------------|
| `AMPLITUDE` | 80 px | Basic, POWArmor (base) |
| `FREQUENCY` | 1.8 Hz | Basic, POWArmor (base) |
| `ZIGZAG_SPEED_Y` | 300 px/s | Zigzag |
| `ZIGZAG_INTERVAL` | 0.8s | Zigzag |
| `TRACKER_SPEED_Y` | 150 px/s | Tracker |

### Deblocage par vague

| Vague | Nouveaux types disponibles |
|-------|---------------------------|
| 1 | Basic uniquement |
| 2 | + Tracker |
| 3 | + Zigzag |
| 4 | + Fast |
| 5 | + Bomber |

---

### Basic

```
Type: Basic (EnemyType::Basic)
HP: 40
Vitesse X: -120 px/s
Interval de tir: 2.5s
Points: 100
Vitesse missile: -400 px/s
```

**Pattern de mouvement : Sinusoidal classique**

```cpp
y = baseY + AMPLITUDE * sin(FREQUENCY * aliveTime + phaseOffset)
```

Visualisation :
```
         ~~~~~~~~~~~
        /           \
       /             \    Amplitude: 80px
------/               \------→ X (-120 px/s)
                       \
                        ~~~
```

**Caracteristiques :**
- Mouvement fluide et previsible
- `phaseOffset` aleatoire a la creation (evite la synchronisation)
- Oscillation de ±80 pixels autour de `baseY`
- Frequence de 1.8 Hz (environ 1 cycle toutes les 0.55s)

**Tirs pour tuer :**
| Arme | Tirs |
|------|------|
| Standard | 2 |
| Spread (tous) | 2 salves |
| Laser | 4 |
| Homing | 1 |
| Wave Lv1 | 1 |

**Strategie :** Anticiper le mouvement sinusoidal, viser au centre de l'oscillation.

---

### Tracker

```
Type: Tracker (EnemyType::Tracker)
HP: 35
Vitesse X: -100 px/s
Interval de tir: 2.0s
Points: 150
Vitesse missile: -400 px/s
Vitesse Y: 150 px/s (poursuite)
```

**Pattern de mouvement : Poursuite du joueur**

```cpp
float targetY = getNearestPlayerY();
float diff = targetY - enemy.y;
float maxMove = TRACKER_SPEED_Y * deltaTime;  // 150 px/s

if (abs(diff) > maxMove) {
    y += (diff > 0 ? maxMove : -maxMove);
} else {
    y = targetY;
}
```

Visualisation :
```
    Joueur ←──┐
              │ (suit en Y a 150 px/s)
    Tracker ──┘
        ↓
    [avance vers la gauche a -100 px/s]
```

**Caracteristiques :**
- Cherche le joueur le plus proche
- Se deplace verticalement vers la position Y du joueur
- Vitesse de poursuite limitee a 150 px/s (ne peut pas teleporter)
- Plus lent horizontalement (-100 px/s) pour compenser sa precision

**Strategie :** Bouger verticalement au dernier moment pour eviter ses tirs, car il ajuste constamment sa position.

---

### Zigzag

```
Type: Zigzag (EnemyType::Zigzag)
HP: 25
Vitesse X: -140 px/s
Interval de tir: 3.0s
Points: 120
Vitesse missile: -400 px/s
Vitesse Y: 300 px/s (zigzag)
Intervalle changement: 0.8s
```

**Pattern de mouvement : Dents de scie**

```cpp
zigzagTimer += deltaTime;
if (zigzagTimer >= ZIGZAG_INTERVAL) {  // 0.8s
    zigzagTimer = 0.0f;
    zigzagUp = !zigzagUp;  // Inverse la direction
}

float direction = zigzagUp ? -1.0f : 1.0f;
y += direction * ZIGZAG_SPEED_Y * deltaTime;  // ±300 px/s
```

Visualisation :
```
          /\    /\    /\
         /  \  /  \  /  \     Vitesse Y: 300 px/s
        /    \/    \/    \    Changement: tous les 0.8s
    ───/                  \───→ X (-140 px/s)
```

**Caracteristiques :**
- Mouvement lineaire vertical (pas sinusoidal!)
- Change de direction toutes les 0.8 secondes
- Tres rapide verticalement (300 px/s)
- Direction initiale aleatoire (50% haut, 50% bas)
- Le plus difficile a toucher

**Distance parcourue par segment :** 300 × 0.8 = **240 pixels** avant changement

**Strategie :** Utiliser le Spread ou le Homing. Anticiper le changement de direction toutes les ~0.8s.

---

### Fast

```
Type: Fast (EnemyType::Fast)
HP: 20
Vitesse X: -220 px/s
Interval de tir: 1.5s
Points: 180
Vitesse missile: -550 px/s (plus rapide!)
```

**Pattern de mouvement : Sinusoidal rapide et serre**

```cpp
float fastAmplitude = AMPLITUDE * 0.6f;    // 48 pixels (reduit)
float fastFrequency = FREQUENCY * 2.5f;    // 4.5 Hz (accelere)
y = baseY + fastAmplitude * sin(fastFrequency * aliveTime + phaseOffset);
```

Visualisation :
```
    ~~~~~~~~~~~~~~~~~~~~~~→ X (-220 px/s, tres rapide!)

    Amplitude: 48px (reduite)
    Frequence: 4.5 Hz (2.5x plus rapide que Basic)
```

**Caracteristiques :**
- Traverse l'ecran en ~8.7 secondes (1920 / 220)
- Ondulation serree : ±48 pixels
- Frequence elevee : ~1 cycle toutes les 0.22s
- Missiles plus rapides (-550 px/s vs -400 px/s)
- **Peu de temps pour reagir!**

**Danger :** Combine vitesse horizontale elevee + tirs rapides. Priorite moyenne-haute.

**Strategie :** Utiliser le Homing ou anticiper sa trajectoire. Un seul tir Standard suffit (20 HP).

---

### Bomber

```
Type: Bomber (EnemyType::Bomber)
HP: 80
Vitesse X: -80 px/s
Interval de tir: 1.0s
Points: 250
Vitesse missile: -300 px/s (lent)
Missiles par salve: 2 (decales verticalement)
```

**Pattern de mouvement : Sinusoidal lent avec derive vers le bas**

```cpp
float slowAmplitude = AMPLITUDE * 0.3f;    // 24 pixels (petit)
float slowFrequency = FREQUENCY * 0.5f;    // 0.9 Hz (lent)
y = baseY + slowAmplitude * sin(slowFrequency * aliveTime + phaseOffset);
baseY += 10.0f * deltaTime;  // Derive constante vers le bas!
```

Visualisation :
```
        ~~~
       /   \
      /     \──────→ lente derive vers le bas (+10 px/s)
     /              X: -80 px/s (tres lent)
    /
   ~~~
```

**Caracteristiques :**
- Le plus lent horizontalement (-80 px/s)
- Petite oscillation : ±24 pixels
- **Derive vers le bas** : +10 px/s (descend progressivement)
- **2 missiles simultanement** a des hauteurs differentes
- Tank : 80 HP (4 tirs Standard)

**Positions des missiles :**
```
    Bomber ─┬─ Missile 1 (30% hauteur)
            │
            └─ Missile 2 (70% hauteur)
```

**Strategie :** **Priorite haute!** DPS eleve (2 missiles/seconde). Utiliser le Homing ou concentrer le feu.

---

### POW Armor (Special)

```
Type: POWArmor (EnemyType::POWArmor)
HP: 50
Vitesse X: -90 px/s
Interval de tir: 4.0s
Points: 200
Drop garanti: 100% Power-Up
Spawn automatique: tous les 25 secondes
```

**Pattern de mouvement : Sinusoidal classique (identique a Basic)**

```cpp
y = baseY + AMPLITUDE * sin(FREQUENCY * aliveTime + phaseOffset);
// Identique au Basic
```

**Caracteristiques :**
- Mouvement identique au Basic (sinusoidal)
- Plus lent que Basic (-90 px/s vs -120 px/s)
- Tire rarement (4s entre chaque tir)
- **Garantit un Power-Up a la mort**
- Spawn automatique toutes les 25 secondes

**Strategie :** Cible prioritaire pour obtenir des Power-Ups. Facile a tuer grace a sa lenteur.

---

### Resume des Patterns de Mouvement

| Type | Pattern | Formule Y |
|------|---------|-----------|
| **Basic** | Sinusoidal | `baseY + 80 * sin(1.8 * t)` |
| **Tracker** | Poursuite | `y += 150 * sign(playerY - y) * dt` |
| **Zigzag** | Dents de scie | `y += 300 * direction * dt` (change /0.8s) |
| **Fast** | Sinusoidal rapide | `baseY + 48 * sin(4.5 * t)` |
| **Bomber** | Sinusoidal + derive | `baseY + 24 * sin(0.9 * t)` + baseY+=10*dt |
| **POWArmor** | Sinusoidal | Identique a Basic |

### Missiles Ennemis

| Type ennemi | Vitesse missile | Missiles/salve |
|-------------|-----------------|----------------|
| Basic | -400 px/s | 1 |
| Tracker | -400 px/s | 1 |
| Zigzag | -400 px/s | 1 |
| Fast | **-550 px/s** | 1 |
| Bomber | -300 px/s | **2** |
| POWArmor | -400 px/s | 1 |

### Limites de Position

Tous les ennemis sont contraints a rester dans l'ecran :
```cpp
y = clamp(y, 0, SCREEN_HEIGHT - HEIGHT);  // 0 - 1040 px
baseY = clamp(baseY, SPAWN_Y_MIN, SPAWN_Y_MAX);  // 100 - 900 px
```

---

## Boss

Le boss apparait toutes les **10 vagues** (10, 20, 30, ...) une fois tous les ennemis elimines.
Chaque cycle augmente ses HP de **+500**.

### Statistiques principales

| Stat | Valeur |
|------|--------|
| **HP Total (Cycle 1)** | 1500 |
| **HP Total (Cycle 2)** | 2000 |
| **HP Total (Cycle 3)** | 2500 |
| **Largeur** | 150 px |
| **Hauteur** | 120 px |
| **Vitesse normale** | 150 px/s |
| **Vitesse dash** | 800 px/s |
| **Points base** | 5000 |
| **Bonus cycle** | +1000 par cycle |

### Formule HP Boss

```
HP = 1500 + (cycle - 1) × 500
```

| Cycle | Vague | HP Boss | Points totaux* |
|-------|-------|---------|----------------|
| 1 | 10 | 1500 | 6000 - 11000 |
| 2 | 20 | 2000 | 7000 - 12000 |
| 3 | 30 | 2500 | 8000 - 13000 |
| 4 | 40 | 3000 | 9000 - 14000 |

*Points = (5000 × phase) + (cycle × 1000), phase 1-3

### Phases

| Phase | HP Range | Seuil | Attaques | Minions max |
|-------|----------|-------|----------|-------------|
| **Phase 1** | 100% - 65% | - | Linear, Spread | 0 |
| **Phase 2** | 65% - 30% | 975 HP | + Laser, Spawn | 4 |
| **Phase 3** | 30% - 0% | 450 HP | + Barrage, Homing | 8 |
| **Enrage** | < 20% | 300 HP | Toutes accelerees | 8 |

### Intervalles d'attaque

| Phase | Intervalle |
|-------|------------|
| Phase 1 | 2.0s |
| Phase 2 | 1.5s |
| Phase 3 | 1.0s |
| Enrage | 0.7s (estime) |

### Attaques detaillees

#### Linear Shots
- **Disponible** : Phase 1+
- **Description** : Tire une ligne de missiles horizontaux
- **Danger** : Faible
- **Esquive** : Se deplacer verticalement

#### Spread Shot
- **Disponible** : Phase 1+
- **Description** : Eventail de 5-8 missiles
- **Danger** : Moyen
- **Esquive** : Trouver les trous entre les missiles

#### Laser Charge
- **Disponible** : Phase 2+
- **Avertissement** : 1.0s (indicateur visuel)
- **Duree** : 2.0s
- **Danger** : Eleve
- **Esquive** : Se deplacer des que l'avertissement apparait

#### Circular Barrage
- **Disponible** : Phase 3+
- **Description** : 360 degres de missiles
- **Missiles** : 12 (normal), 16 (enrage)
- **Danger** : Tres eleve
- **Esquive** : Trouver l'angle de securite

#### Dash Charge
- **Disponible** : Phase 2+
- **Description** : Charge a travers l'ecran
- **Vitesse** : 800 px/s
- **Danger** : Eleve
- **Esquive** : Anticiper et se deplacer perpendiculairement

#### Spawn Minions
- **Disponible** : Phase 2+
- **Types** : Fast et Tracker
- **Max Phase 2** : 4 minions
- **Max Phase 3** : 8 minions
- **Intervalle** : 5.0s entre spawns

#### Teleport
- **Disponible** : Phase 2+
- **Description** : Disparait et reapparait ailleurs
- **Bonus** : Declenche un Circular Barrage apres

#### Homing Swarm
- **Disponible** : Phase 3+
- **Missiles** : 4 (normal), 6 (enrage)
- **Description** : Missiles guides vers les joueurs
- **Danger** : Eleve
- **Esquive** : Mouvement constant

### Mouvements

| Pattern | Phase | Description |
|---------|-------|-------------|
| Sinusoidal | 1 | Oscillation verticale douce |
| Figure-8 | 2 | Mouvement en forme de 8 |
| Aggressive | 3 | Poursuit le joueur le plus proche |
| Charge | Tous | Durant les attaques de dash |

### Temps de combat estime

| Arme | DPS | Temps |
|------|-----|-------|
| Standard | 66.7 | ~22.5s |
| Spread | 60.0 | ~25.0s |
| Laser | 66.7 | ~22.5s |
| Homing | 71.4 | ~21.0s |
| Wave Lv3 spam | 90.9 | ~16.5s |

---

## Systeme de Score

### Points par ennemi

| Ennemi | Points base |
|--------|-------------|
| Basic | 100 |
| Tracker | 150 |
| Zigzag | 120 |
| Fast | 180 |
| Bomber | 250 |
| Boss | 5000 |

### Bonus

| Type | Points |
|------|--------|
| Vague sans degats | 500 |

### Systeme de Combo

| Stat | Valeur |
|------|--------|
| Multiplicateur de depart | 1.0x |
| Increment par kill | +0.1x |
| Maximum | 3.0x |
| Decay (temps sans kill) | 2.0s |

**Formule de score :**
```
Score = Points_base × Combo_multiplier
```

**Exemple :**
- Kill Basic avec 1.5x combo = 100 × 1.5 = 150 points
- Kill Boss avec 3.0x combo = 5000 × 3.0 = 15000 points

---

## Systeme de Vagues

### Configuration des vagues

| Parametre | Valeur |
|-----------|--------|
| Intervalle entre vagues | 6-12 secondes |
| Ennemis par vague (base) | 2-6 |
| Zone de spawn X | 1950 px (hors ecran droite) |
| Zone de spawn Y | 100 - 900 px |
| Delai entre spawns | 0.3 - 1.2 secondes |

### Scaling Infini des Vagues

Le jeu continue **indefiniment** avec une difficulte croissante. Voici comment le scaling fonctionne :

#### Nombre d'ennemis par vague

```
Ennemis = Base(2-6) + WaveBonus + CycleBonus
```

| Composant | Formule | Maximum |
|-----------|---------|---------|
| Base | 2-6 aleatoire | 6 |
| WaveBonus | +1 toutes les 3 vagues | +4 (atteint vague 12) |
| CycleBonus | +1 par boss vaincu | +3 |

**Exemples :**

| Vague | Cycle | Ennemis (min-max) |
|-------|-------|-------------------|
| 1 | 1 | 2-6 |
| 6 | 1 | 4-8 |
| 12 | 2 | 7-11 |
| 25 | 3 | 10-13 (cap MAX_ENEMIES=16) |

#### Difficulte des ennemis

Le **Difficulty Tier** determine les probabilites d'apparition des types dangereux :

```
DifficultyTier = (Wave / 5) + (BossDefeated × 2)
```

| Tier | Bomber | Fast | Zigzag | Tracker | Basic |
|------|--------|------|--------|---------|-------|
| 0 (vagues 1-4) | 10% | 15% | 15% | 15% | 45% |
| 2 (vagues 5-9) | 16% | 19% | 19% | 19% | 27% |
| 4 (cycle 2) | 22% | 23% | 23% | 23% | 9% |
| 6+ (cycle 3+) | 30% | 25% | 25% | 20% | 0% |

**Note :** Les ennemis sont deverrouilles progressivement (Tracker vague 2, Zigzag vague 3, Fast vague 4, Bomber vague 5).

#### Vitesse de spawn

Le delai entre chaque ennemi d'une vague diminue avec la difficulte :

```
Delai = BaseDelai × max(0.5, 1.0 - DifficultyTier × 0.05)
```

| Tier | Multiplicateur | Delai effectif |
|------|----------------|----------------|
| 0 | 1.0x | 0.3 - 1.2s |
| 5 | 0.75x | 0.23 - 0.9s |
| 10+ | 0.5x (min) | 0.15 - 0.6s |

### Progression par Cycle

| Vague | Difficulte | Evenement |
|-------|------------|-----------|
| 1-3 | Facile | Principalement Basic |
| 4-6 | Moyenne | Mix de types |
| 7-9 | Difficile | Plus de Fast et Bomber |
| **10** | **Boss #1** | Boss 1500 HP |
| 11-19 | Post-boss | Continue, difficulte elevee |
| **20** | **Boss #2** | Boss 2000 HP |
| 21-29 | Cycle 2 | +1 ennemi/vague, ennemis plus dangereux |
| **30** | **Boss #3** | Boss 2500 HP |
| ... | **Infini** | Continue indefiniment |

### Boss Spawn (Recurrent)

Le boss apparait **toutes les 10 vagues** :

1. Vague multiple de 10 atteinte (10, 20, 30, ...)
2. Attendre que tous les ennemis soient elimines
3. Boss spawn a droite de l'ecran
4. Pas de nouvelles vagues pendant le combat
5. Apres victoire, vagues reprennent jusqu'au prochain boss

---

## Formules et Calculs

### DPS (Degats Par Seconde)

```
DPS = Degats / Cooldown
```

### Temps pour Tuer (TTK)

```
TTK = HP_cible / DPS
```

### Score avec Combo

```
Score = Points_base × min(3.0, 1.0 + 0.1 × kills_consecutifs)
```

### Vitesse joueur

```
Vitesse = 200 × (1.0 + 0.1 × speedLevel)
```

### Charge Wave Cannon

```
if (chargeTime >= 2.2) level = 3
else if (chargeTime >= 1.3) level = 2
else if (chargeTime >= 0.6) level = 1
else level = 0 (pas de tir)
```

---

## Hitboxes

### Resume des hitboxes

| Entite | Largeur | Hauteur |
|--------|---------|---------|
| Joueur | 64 px | 30 px |
| Missile joueur | 16 px | 8 px |
| Wave Cannon | 60 px | 20-55 px |
| Force Pod | 32 px | 32 px |
| Ennemi (tous) | 40 px | 40 px |
| Boss | 150 px | 120 px |
| Power-Up | 24 px | 24 px |

### Collision AABB

Le jeu utilise la detection de collision AABB (Axis-Aligned Bounding Box).

```cpp
bool intersects(AABB a, AABB b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}
```

---

## References Code

### Fichiers principaux

| Fichier | Contenu |
|---------|---------|
| `src/server/include/infrastructure/game/GameWorld.hpp` | Constantes principales |
| `src/common/protocol/Protocol.hpp` | Wave Cannon, Power-ups |
| `src/server/infrastructure/game/GameWorld.cpp` | Logique de combat |
| `src/common/collision/AABB.hpp` | Systeme de collision |

### Constantes cles

```cpp
// === MISSILES ===
Missile::DAMAGE_STANDARD = 20;
Missile::DAMAGE_SPREAD = 8;
Missile::DAMAGE_LASER = 12;
Missile::DAMAGE_MISSILE = 50;

Missile::COOLDOWN_STANDARD = 0.3f;
Missile::COOLDOWN_SPREAD = 0.4f;
Missile::COOLDOWN_LASER = 0.18f;
Missile::COOLDOWN_MISSILE = 0.7f;

Missile::SPEED_STANDARD = 600.0f;
Missile::SPEED_SPREAD = 550.0f;
Missile::SPEED_LASER = 900.0f;
Missile::SPEED_MISSILE = 350.0f;

// === ENNEMIS ===
Enemy::HEALTH_BASIC = 40;
Enemy::HEALTH_TRACKER = 35;
Enemy::HEALTH_ZIGZAG = 25;
Enemy::HEALTH_FAST = 20;
Enemy::HEALTH_BOMBER = 80;
Enemy::HEALTH_POW_ARMOR = 50;

Enemy::SPEED_X_BASIC = -120.0f;
Enemy::SPEED_X_TRACKER = -100.0f;
Enemy::SPEED_X_ZIGZAG = -140.0f;
Enemy::SPEED_X_FAST = -220.0f;
Enemy::SPEED_X_BOMBER = -80.0f;
Enemy::SPEED_X_POW_ARMOR = -90.0f;

// === BOSS ===
BOSS_MAX_HEALTH = 1500;
BOSS_PHASE2_THRESHOLD = 0.65f;  // 65%
BOSS_PHASE3_THRESHOLD = 0.30f;  // 30%
Boss::WIDTH = 150.0f;
Boss::HEIGHT = 120.0f;
Boss::MOVE_SPEED = 150.0f;
Boss::DASH_SPEED = 800.0f;

// === WAVE CANNON ===
WaveCannon::CHARGE_TIME_LV1 = 0.6f;
WaveCannon::CHARGE_TIME_LV2 = 1.3f;
WaveCannon::CHARGE_TIME_LV3 = 2.2f;
WaveCannon::DAMAGE_LV1 = 50;
WaveCannon::DAMAGE_LV2 = 100;
WaveCannon::DAMAGE_LV3 = 200;
WaveCannon::WIDTH_LV1 = 20.0f;
WaveCannon::WIDTH_LV2 = 35.0f;
WaveCannon::WIDTH_LV3 = 55.0f;

// === FORCE POD ===
ForcePod::CONTACT_DAMAGE = 45;
ForcePod::WIDTH = 32.0f;
ForcePod::HEIGHT = 32.0f;
ForcePod::SPEED = 400.0f;

// === SCORE ===
POINTS_BASIC = 100;
POINTS_TRACKER = 150;
POINTS_ZIGZAG = 120;
POINTS_FAST = 180;
POINTS_BOMBER = 250;
POINTS_POW_ARMOR = 200;
POINTS_BOSS = 5000;
POINTS_WAVE_BONUS = 500;
COMBO_MAX = 3.0f;
COMBO_INCREMENT = 0.1f;
COMBO_DECAY_TIME = 2.0f;

// === VAGUES ===
BOSS_SPAWN_WAVE = 10;
WAVE_INTERVAL_MIN = 6.0f;
WAVE_INTERVAL_MAX = 12.0f;
ENEMIES_PER_WAVE_MIN = 2;
ENEMIES_PER_WAVE_MAX = 6;

// === DEGATS ===
ENEMY_DAMAGE = 15;  // Degats des missiles ennemis
PLAYER_DAMAGE = 20; // Degats au joueur
DEFAULT_HEALTH = 100;
```

---

## Historique des modifications

### Version 2.0 (Janvier 2026)
- Equilibrage complet des armes pour ~65-70 DPS
- Nerf Laser : 20 dmg → 12 dmg, cooldown 0.15s → 0.18s
- Buff Homing : 30 dmg → 50 dmg, cooldown 1.0s → 0.7s
- HP Boss : 500 → 1500
- HP Ennemis augmentes (Basic 30→40, Bomber 50→80, etc.)
- Wave Cannon : temps de charge augmente, degats augmentes
- Force Pod : degats contact 30 → 45
- Fix : Wave Cannon ne touche plus plusieurs fois le meme ennemi
- Fix : Force Pod ne fait plus de degats chaque frame (cooldown 0.5s ajoute)
- Fix : Force Pod detache avance maintenant en continu (au lieu de 100px puis stop)
- Add : Force Pod peut desormais endommager le boss

### Version 1.0 (Initial)
- Implementation de base du systeme de combat
- 4 armes : Standard, Spread, Laser, Homing
- 5 types d'ennemis
- Boss avec 3 phases
- Wave Cannon avec 3 niveaux
