# Rover-RTK_Bluetooth

Ce dossier regroupe les variantes de **rover GNSS RTK basées sur un module Bluetooth “série”** (sans ESP32) ainsi que la documentation associée à la version commerciale **NavX**.

L’idée est volontairement simple et robuste :

> **Le module Bluetooth agit comme un pont UART transparent** entre le récepteur GNSS (UART) et un smartphone/tablette/PC.
> Le smartphone lit les trames GNSS (NMEA) et peut **injecter les corrections RTCM** (via NTRIP) en retour sur le même lien Bluetooth.

---

## Variantes supportées dans ce dossier

### 1) Rover Bluetooth — **DIY**
Un rover “minimal” basé sur :
- un **module Bluetooth** (ex : **Feasycom DB004-BT836B / FSC-BT836B**),
- un **récepteur GNSS** (ex : UM980, mosaic-G5, etc.),
- une **alimentation** (ex : Li-ion + chargeur/boost),
- connectique (USB-C, bouton, antenne…).

➡️ BOM + visuels : `DIY/BOM.md`, `DIY/assemblage.png`, `DIY/pictures/`

### 2) Rover Bluetooth — **NavX (commercial)**
Le **NavX** est une version commerciale “plug & play” (basée sur la même philosophie fonctionnelle : pont série + usage terrain).

- Produit : https://natuition.odoo.com/shop/n2168-navx-2678
- Doc hardware : `NavX/NavX REV1.1.pdf`

> Ce dépôt contient la documentation (PDF) côté intégration, pas la chaîne industrielle.

---

## Pré-requis côté récepteur GNSS (indispensable)

Même si ce dossier contient le “transport” Bluetooth, **la stabilité du rover dépend d’abord de la configuration du récepteur GNSS**.

À vérifier / configurer sur le récepteur :

1. **Sortie NMEA** sur l’UART utilisé
   - Minimum utile : `GGA` (souvent suffisant pour NTRIP)
   - Optionnel : `RMC`, `GSA`, `GSV` selon usages

2. **Entrée RTCM3** activée sur le même port UART
   - Le smartphone (client NTRIP) enverra les RTCM via Bluetooth → UART → GNSS

3. **Baudrate cohérent**
   - 921600 car beaucoup de trames / haut taux / RTCM dense

4. **Taux de sortie**
   - 5 Hz à 10 Hz est le plus robuste

📌 Les profils et paramétrages des récepteurs se trouvent au niveau dépôt dans le dossier `conf_GNSS/`

---

## Rover Bluetooth DIY

### BOM / composants
Voir `DIY/BOM.md` (liens + prix indicatifs) :
- alimentation (chargeur Li-ion + support 18650 + accu),
- connectique USB-C,
- module Bluetooth Feasycom,
- récepteur GNSS (UM980 / mosaic-G5…),
- antenne GNSS.

### Schéma d’assemblage
- `DIY/assemblage.png`

![Assemblage](DIY/assemblage.png)

### Câblage minimal (UART)
Le module Bluetooth doit être relié au récepteur GNSS en UART TTL (niveau logique à respecter).

**Principe :**
- GNSS **TX** → BT **RX**
- GNSS **RX** ← BT **TX**
- **GND commun**

Schéma logique :

```

[Récepteur GNSS]                 [Module Bluetooth]
TX   ----------------------->     RX
RX   <-----------------------     TX
GND  ------------------------     GND
VCC  ------------------------     VCC  

```

💡 Bonnes pratiques :
- fils courts, masse propre
- si tu vois des caractères “illisibles” : **baudrate incorrect** ou niveaux logiques non adaptés

---

## Utilisation sur smartphone (principe)

1. Appairer le module Bluetooth (*SPP* / “port série”) ou passer cette étape si connexion *BLE* (iOS)
2. Dans l’application GNSS :
   - sélectionner le **GNSS externe** via Bluetooth
   - configurer un **client NTRIP**
3. Vérifier :
   - le flux NMEA arrive (tu vois les trames ou la position),
   - l’app envoie bien des RTCM,
   - le récepteur passe en RTK (FLOAT puis FIX).

---

## Module Bluetooth : configuration & mise à jour firmware

### Configuration
Selon le module (Feasycom DB004-BT836B) :
- nom Bluetooth, PIN, paramètres série, modes…
se règlent via les outils Feasycom (ex : application **FeasyBlue**).

### Mise à jour firmware (DFU)
Le dossier `conf/` contient :
- un fichier `*.dfu` de firmware
- un mini README avec une vidéo de procédure : `conf/README.md`
- https://www.youtube.com/watch?v=9JxQYCYJvog

---

## NavX (commercial)

- Produit : https://natuition.odoo.com/shop/n2168-navx-2678
- Documentation hardware (révision) : `NavX/NavX REV1.1.pdf`

Ce PDF sert de référence d’intégration/maintenance (connectique, révision, points matériels).
Le fonctionnement côté utilisateur reste identique sur le principe : **pont série** + smartphone qui gère le NTRIP (selon mode choisi).

---

## Dépannage rapide

- **Rien ne sort en NMEA**
  - vérifier TX/RX inversés, GND commun
  - vérifier la config NMEA côté GNSS
  - vérifier baudrate

- **Caractères illisibles**
  - baudrate incorrect (GNSS ≠ BT)
  - problème de niveau logique (1.8V vs 3.3V)

- **Le RTK ne fixe pas**
  - pas de RTCM reçu (NTRIP, identifiants, mountpoint)
  - GGA non envoyé (certains cas)
  - antenne / ciel / multipath

- **Le smartphone ne “voit” pas le module**
  - mode appairage, PIN, distance, alimentation
  - vérifier que le module est bien alimenté et annonce le service SPP

---
