# rover-gnss — Rovers GNSS RTK (Bluetooth / ESP32) + configurations récepteurs

Ce dépôt regroupe **tout le nécessaire pour fabriquer un rover GNSS RTK** autour d’un récepteur (ex. u-blox, Unicore, …) et l’utiliser :
- soit comme **rover “pass-through”** (le smartphone fait le client NTRIP et injecte les RTCM),
- soit comme **rover autonome** (ESP32 + Wi-Fi : client NTRIP embarqué),
- soit comme **rover connecté** (ESP32 + Wi-Fi + MQTT + capteurs).

Il contient également un dossier de **configurations récepteurs GNSS** (profils NMEA/RTCM, débits UART, phrases NMEA, etc.) afin d’avoir un comportement stable et reproductible.

> Objectif : fournir des briques **DIY** (matériel + firmware + tests) et documenter les variantes, tout en conservant une compatibilité maximale avec les applis mobiles GNSS/NTRIP.

---

## Sommaire

- [Choisir sa variante](#choisir-sa-variante)
- [Arborescence du dépôt](#arborescence-du-dépôt)
- [Pré-requis GNSS (indispensable)](#pré-requis-gnss-indispensable)
- [Démarrage rapide](#démarrage-rapide)
- [Rover “Bluetooth module” (sans ESP32)](#rover-bluetooth-module-sans-esp32)
- [Rover ESP32 (DIY)](#rover-esp32-diy)
- [Unit tests / prototypes](#unit-tests--prototypes)
- [Bonnes pratiques (secrets, stabilité, debug)](#bonnes-pratiques-secrets-stabilité-debug)
- [Licence](#licence)
- [Crédits](#crédits)

---

## Choisir sa variante

| Variante | Matériel principal | iOS | Android | Qui fait le NTRIP ? | Points forts | Dossier |
|---|---|---:|---:|---|---|---|
| **Bluetooth module (sans ESP32)** | Module BT (SPP) + GNSS | ⚠️ | ✅ | Smartphone | très low-power, peu de code, montage “léger” | `Rover-RTK_Bluetooth/DIY/` |
| **NavX (commercial)** | produit fini |  ⚠️  | ✅ | Smartphone | solution “plug&play” basée sur la même philosophie | `Rover-RTK_Bluetooth/NavX/` |
| **Bluetooth SPP (pass-through)** | ESP32 + GNSS | ⚠️ | ✅ | Smartphone | simple, bidirectionnel (RTCM → GNSS), efficace | `Rover-RTK_ESP32/rover-rtk_BT-SPP/` |
| **Bluetooth BLE (NUS)** | ESP32 + GNSS | ✅ | ✅ | Smartphone | compatible iOS via BLE/NUS (ex. SW Maps) | `Rover-RTK_ESP32/rover-rtk_BT-BLE/` |
| **Bluetooth + Wi-Fi + NTRIP embarqué** | ESP32 + GNSS | ⚠️ | ✅ | **ESP32** | rover autonome (Wi-Fi → NTRIP → RTCM → GNSS) | `Rover-RTK_ESP32/rover-rtk_BT-WIFI-NTRIP/` |
| **Custom : BT + Wi-Fi + NTRIP + MQTT + capteurs** | ESP32 + GNSS + capteurs |  ⚠️  | ✅ | **ESP32** | télémétrie MQTT + config via AP/web + capteurs | `Rover-RTK_ESP32/rover-rtk_BT_WIFI_NTRIP_MQTT_SENSORS/` |


> ⚠️ Remarque iOS : iOS ne supporte pas le Bluetooth SPP “classique” comme un port série générique. Pour iOS, privilégier **BLE (NUS)** ou un produit intégrant un profil compatible.

---

## Arborescence du dépôt

- `conf_GNSS/`  
  Profils / scripts / exports de configuration pour récepteurs GNSS (débits UART, NMEA, entrée RTCM…).  
  **C’est la base** pour que le rover soit stable (rate, baudrate, phrases NMEA, etc.).

- `Rover-RTK_Bluetooth/`  
  Tout ce qui concerne les rovers basés sur **module Bluetooth** (sans ESP32) :
  - `DIY/` : BOM + assemblage + photos
  - `conf/` : fichiers liés à la configuration/MAJ module (ex. DFU)
  - `NavX/` : documentation matérielle (rev) + référence commerciale

- `Rover-RTK_ESP32/`  
  Firmwares **ESP32** (Arduino) + librairies + variantes + `unit_tests/` :
  - `rover-rtk_BT-SPP/`
  - `rover-rtk_BT-BLE/`
  - `rover-rtk_BT-WIFI-NTRIP/`
  - `rover-rtk_BT_WIFI_NTRIP_MQTT_SENSORS/`
  - `libraries/NTRIPClient/`
  - `unit_tests/` : prototypes + réutilisation de dépôts précédents (voir section dédiée)

---

## Pré-requis GNSS (indispensable)

Quelque soit la variante (module BT ou ESP32), la fiabilité vient d’abord de la **configuration du récepteur GNSS**.

### Ce qu’on attend du récepteur
1. **Un port UART “rover”** qui :
   - sort des phrases **NMEA** (au minimum `GGA`, souvent `RMC`, `GSA`, …),
   - accepte l’**entrée RTCM3** sur le même lien (RTCM injecté par smartphone ou par l’ESP32).
2. Un **débit UART** cohérent avec le volume de données (NMEA + RTCM) :
   - typiquement `115200` pour des configs légères,
   - souvent `460800` (voire plus) pour des sorties plus riches / hauts taux.
3. Un **rate** adapté :
   - 1–2 Hz suffit pour beaucoup d’usages,
   - 5 Hz demande une config plus propre (débit + sélection NMEA).

👉 Les profils prêts à l’emploi sont dans `conf_GNSS/`.  
Si vous changez de récepteur (u-blox ↔ Unicore ↔ autre), **commencez par appliquer un profil** et vérifier :
- baudrate réel,
- NMEA reçu sur UART,
- RTCM bien injecté et solution RTK qui converge (FIX).

---

## Démarrage rapide

### 1) Configurer le récepteur GNSS
- Choisir un profil dans `conf_GNSS/`
- Appliquer la configuration (outil constructeur / commandes / script selon récepteur)
- Vérifier sur un terminal série :
  - des trames NMEA stables,
  - le débit (baudrate) attendu,
  - l’acceptation des RTCM (si vous injectez des corrections).

### 2) Choisir la famille de rover
- **Bluetooth module (sans ESP32)** : le smartphone fait NTRIP et envoie les RTCM via BT → GNSS
- **ESP32** : selon besoin (SPP, BLE, Wi-Fi NTRIP, MQTT…)

### 3) Câblage minimal (UART)
- GNSS TX → (ESP32 RX / module BT RX)
- GNSS RX ← (ESP32 TX / module BT TX)
- GND commun
- Attention aux **niveaux logiques** (souvent **3.3V TTL** côté GNSS/ESP32)

---

## Rover “Bluetooth module” (sans ESP32)

### DIY
Le rover “Bluetooth module” vise un montage **simple, compact, low-power** : un module Bluetooth assure la liaison série entre smartphone et récepteur GNSS.

- BOM + assemblage : `Rover-RTK_Bluetooth/DIY/BOM.md`
- Visuels / photos : `Rover-RTK_Bluetooth/DIY/pictures/`
- Schéma d’assemblage : `Rover-RTK_Bluetooth/DIY/assemblage.png`

![Assemblage DIY](Rover-RTK_Bluetooth/DIY/assemblage.png)

### Configuration du module
Selon le module utilisé, vous trouverez des éléments de configuration / mise à jour dans :
- `Rover-RTK_Bluetooth/conf/` (ex. fichier DFU)

> Le principe reste identique : fournir un lien série BT bidirectionnel pour laisser une appli mobile faire NTRIP + injection RTCM.

### Version commerciale : NavX
Il existe une version **commerciale** “prête à l’emploi” basée sur cette approche :
- Produit : https://natuition.odoo.com/shop/n2168-navx-2678  
- Documentation matérielle (rev) : `Rover-RTK_Bluetooth/NavX/`

---

## Rover ESP32 (DIY)

Tous les firmwares ESP32 sont des sketches Arduino (C++). Le dépôt inclut aussi une librairie NTRIP minimale (`libraries/NTRIPClient/`).

### Pré-requis build
- Arduino IDE (ou PlatformIO)
- Carte : ESP32 (selon modèle)
- Libs selon variante (BLE / Wi-Fi / MQTT / capteurs…)

### 1) Rover Bluetooth SPP (DIY)
- Dossier : `Rover-RTK_ESP32/rover-rtk_BT-SPP/`

Fonction :
- **UART GNSS ↔ Bluetooth SPP** (bidirectionnel)
- Le smartphone peut :
  - lire le NMEA,
  - envoyer les RTCM au GNSS via le lien BT.

Points clés :
- Débit GNSS attendu typiquement élevé (ex. `460800` selon config)
- Nom BT configurable dans le code (ex. `BT_NAME`)

### 2) Rover Bluetooth BLE (NUS) — iOS friendly
- Dossier : `Rover-RTK_ESP32/rover-rtk_BT-BLE/`

Fonction :
- Bridge UART GNSS ↔ BLE via **Nordic UART Service (NUS)**  
- Compatible avec des applis iOS/Android supportant NUS.

### 3) Rover Bluetooth + Wi-Fi avec client NTRIP intégré
- Dossier : `Rover-RTK_ESP32/rover-rtk_BT-WIFI-NTRIP/`

Fonction :
- l’ESP32 se connecte au Wi-Fi,
- ouvre une session NTRIP,
- injecte les RTCM au GNSS sur l’UART,
- optionnellement expose les données via Bluetooth/UDP selon configuration.

Configuration :
- `Secret.h` contient SSID/MdP Wi-Fi et paramètres NTRIP (host/user/pass/mountpoint).

> ⚠️ Ne versionnez pas vos vrais identifiants (voir “bonnes pratiques”).

### 4) Version custom : BT + Wi-Fi + NTRIP + MQTT + capteurs
- Dossier : `Rover-RTK_ESP32/rover-rtk_BT_WIFI_NTRIP_MQTT_SENSORS/`

Fonction :
- Client NTRIP intégré (Wi-Fi → NTRIP → RTCM → GNSS)
- Publication MQTT (télémétrie GNSS + capteurs)
- Mode AP de secours + page web de configuration
- Stockage des paramètres via `Preferences` (NVS)

Mode AP / configuration :
- Si le Wi-Fi échoue, le firmware bascule en **AP**
- SSID AP : `rover-gnss-AP` (voir code)
- Une page web permet de configurer : Wi-Fi, NTRIP, MQTT, fréquence de publication
- mDNS : accès possible via `http://rover.local` selon réseau/OS

Variantes matérielles :
- Une sous-variante existe dans `.../lilyGo/` (câblage/pins spécifiques + photo `RTK_With_DataSensor_To_Mqtt.jpg`)

---

## Unit tests / prototypes

Dossier : `Rover-RTK_ESP32/unit_tests/`

Ce répertoire regroupe des **tests unitaires / prototypes**, incluant aussi des éléments issus de dépôts précédents (historique de développement). Objectifs :
- tester rapidement une brique (BLE bridge, Wi-Fi/MQTT, lecture capteurs, etc.),
- conserver des exemples reproductibles,
- faciliter la non-régression lors de refactorings.

⚠️ À considérer comme “laboratoire” :
- certaines configs pointent vers des `conf_GNSS/` volumineux,
- certains sketches sont volontairement verbeux (logs) pour debug.

---

## Bonnes pratiques (secrets, stabilité, debug)

### Secrets / identifiants
- Les fichiers type `Secret.h` sont des **exemples**.
- Pour un usage réel :
  - dupliquez en local (`Secret.h` non commité),
  - gardez vos identifiants hors Git,
  - utilisez des variables/stockage NVS quand c’est prévu (variant MQTT/capteurs).

### Stabilité série
- Si vous poussez le GNSS à 5 Hz + phrases NMEA riches, montez le baudrate (`460800` voire plus).
- Limitez le nombre de phrases NMEA au strict nécessaire (souvent `GGA` suffit pour NTRIP).

### Debug
- La plupart des sketches ont une sortie `Serial` (USB) à `115200` pour logs.
- Commencez toujours par valider :
  1) NMEA brut sur UART
  2) Bridge (BT/BLE) fonctionnel
  3) Injection RTCM (smartphone ou NTRIP embarqué)
  4) Passage en RTK FLOAT/FIX
