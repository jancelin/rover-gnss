![RTK_With_DataSensor_To_Mqtt](https://github.com/jancelin/rover-gnss/assets/25310798/f87b3675-8c67-465a-b33f-a398bb3e743e)

Rétablir l'ensemble des liens du document et web pour ce markdown:
# Programme de Collecte et d'Envoi de Données GNSS et Température pour LilyGO T-PCIe (ESP32)

## Table des Matières

1. [Fonctionnement du Programme](#fonctionnement-du-programme)
    - [Introduction](#introduction)
    - [Classes et Bibliothèques Utilisées](#classes-et-bibliothèques-utilisées)
    - [Fonctions Principales](#fonctions-principales)
        - [`setup()`](#setup)
        - [`loop()`](#loop)
        - [`logMessage()`](#logmessage)
        - [Autres Fonctions](#autres-fonctions)
2. [Installation et Déploiement](#installation-et-déploiement)
    - [Prérequis](#prérequis)
    - [Procédure](#procédure)
3. [Crédits des Licences](#crédits-des-licences)

---

## Fonctionnement du Programme

### Introduction

Ce programme est conçu pour fonctionner sur une carte LilyGO T-PCIe équipée d'un ESP32 et d'un module M-PCIe Drotek F9P. Il collecte les données GNSS (GPS), les températures des capteurs, et les envoie via MQTT à un serveur pour traitement.

### Classes et Bibliothèques Utilisées

1. **WiFi.h** ([Documentation](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h))
   - Gère la connexion WiFi de l'ESP32. Cela permet à l'ESP32 de se connecter à un réseau sans fil pour envoyer des données sur Internet.

2. **NTRIPClient.h**
   - Une bibliothèque spécifique pour la communication NTRIP (Networked Transport of RTCM via Internet Protocol), utilisée pour obtenir les données brutes des stations GNSS (comme les données de correction pour améliorer la précision du GPS).

3. **HardwareSerial.h**
   - Gère la communication série matérielle. Utilisé pour la communication entre l'ESP32 et d'autres appareils via les broches série (TX et RX).

4. **OneWire.h** et **DallasTemperature.h** ([DallasTemperature Documentation](https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/DallasTemperature.h))
   - Utilisées pour lire les capteurs de température DS18B20 via le bus OneWire. Cela permet de mesurer la température ambiante avec des capteurs.

5. **TinyGPSPlus.h** ([Documentation](https://github.com/mikalhart/TinyGPSPlus))
   - Analyse les données GPS à partir du module GNSS. Cette bibliothèque extrait les informations de localisation et autres données utiles des messages NMEA envoyés par les récepteurs GPS.

6. **PubSubClient.h** ([Documentation](https://github.com/knolleary/pubsubclient))
   - Gère la communication MQTT (Message Queuing Telemetry Transport) pour envoyer les données à un serveur MQTT. MQTT est un protocole de messagerie léger, parfait pour les petits capteurs et appareils mobiles.

### Fonctions Principales

#### `setup()`

- **Initialisation du Matériel** : 
  - `pinMode(POWER_PIN, OUTPUT);` configure la broche d'alimentation comme une sortie.
  - `digitalWrite(POWER_PIN, HIGH);` allume la broche d'alimentation.
  - `Serial.begin(115200);` initialise la communication série à une vitesse de 115200 bauds.
  - `MySerial.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);` initialise une autre communication série avec les broches RX et TX spécifiées.

- **Connexion WiFi** : 
  - `setup_wifi();` appelle la fonction pour se connecter au réseau WiFi en utilisant les identifiants fournis (SSID et mot de passe).

- **Initialisation des Capteurs** :
  - `sensors.begin();` démarre le capteur de température DS18B20.
  - `locateDevices();` localise tous les capteurs de température connectés.
  - `printDeviceAddresses();` affiche les adresses des capteurs trouvés.

- **Configuration MQTT** :
  - `client.setServer(mqtt_server, mqtt_port);` configure l'adresse et le port du serveur MQTT.
  - `client.setCallback(callback);` définit la fonction de rappel qui sera appelée chaque fois qu'un message est reçu sur un sujet auquel le client est abonné.

- **Demande de Données NTRIP** :
  - `requestSourceTable();` demande la table des sources NTRIP disponibles.
  - `requestMountPointRawData();` demande les données brutes du point de montage NTRIP spécifié.

#### `loop()`

- **Gestion des Reconnexions** :
  - Vérifie périodiquement si la connexion WiFi est toujours active. Si ce n'est pas le cas, elle tente de se reconnecter.
  - Vérifie si la connexion MQTT est toujours active. Si ce n'est pas le cas, elle tente de se reconnecter.
  - Vérifie si la connexion NTRIP est toujours active. Si ce n'est pas le cas, elle tente de se reconnecter.

- **Collecte des Données** :
  - `readGNSSData();` lit les données GNSS (GPS) du module.
  - `displayGNSSData();` affiche les données GNSS sur la console série.
  - `readTemperatureSensors();` lit les températures des capteurs DS18B20.

- **Publication MQTT** :
  - `publishMQTTData();` envoie les données collectées au serveur MQTT spécifié.

#### `logMessage()`

- **Fonctions de Journalisation** :
  - Plusieurs versions de la fonction `logMessage` permettent d'afficher des messages sur la console série. Elles sont utilisées pour afficher des informations de débogage, des erreurs, des avertissements ou des informations générales selon le niveau de log spécifié.

### Autres Fonctions

- **`locateDevices()`** :
  - Localise tous les capteurs de température connectés et affiche le nombre de capteurs trouvés.

- **`printDeviceAddresses()`** :
  - Affiche les adresses uniques de tous les capteurs de température connectés.

- **`connectToWiFi()`** :
  - Tente de se connecter au réseau WiFi en utilisant les identifiants fournis (SSID et mot de passe). Si la connexion échoue, elle réessaye après un certain temps.

- **`requestSourceTable()`** :
  - Demande la table des sources NTRIP disponibles.

- **`requestMountPointRawData()`** :
  - Demande les données brutes du point de montage NTRIP spécifié. Si la demande échoue, redémarre l'ESP32 après un délai.

- **`handleMQTTConnection()`** :
  - Gère la reconnexion au serveur MQTT si la connexion est perdue.

- **`readGNSSData()`** :
  - Lit les données GNSS du module et les traite.

- **`displayGNSSData()`** :
  - Affiche les données GNSS (temps, longitude, latitude, altitude, qualité et nombre de satellites) sur la console série.

- **`readTemperatureSensors()`** :
  - Lit les températures des capteurs DS18B20 et affiche les valeurs sur la console série.

- **`publishMQTTData()`** :
  - Envoie les données collectées (température, position GNSS, etc.) au serveur MQTT spécifié.

- **`handleNTRIPData()`** :
  - Gère la réception des données NTRIP. Si les données sont disponibles, les envoie via la communication série.

- **`handleSerialData()`** :
  - Gère les données reçues via la communication série et les envoie au serveur spécifié (UDP, TCP ou autre).

- **`printAddress()`** :
  - Affiche l'adresse unique d'un capteur de température.

- **`setup_wifi()`** :
  - Configure et se connecte au réseau WiFi.

- **`reconnectMQTT()`** :
  - Tente de se reconnecter au serveur MQTT si la connexion est perdue.

- **`reconnectNTRIP()`** :
  - Tente de se reconnecter au serveur NTRIP si la connexion est perdue.

- **`callback()`** :
  - Fonction de rappel appelée chaque fois qu'un message est reçu sur un sujet auquel le client est abonné.

---

## Installation et Déploiement

### Prérequis

1. **Matériel** :
   - LilyGO T-PCIe avec module M-PCIe Drotek F9P.
   - Câbles de connexion.

2. **Logiciels** :
   - [Arduino IDE](https://www.arduino.cc/en/software) pour téléverser le code sur l'ESP32.
   - [Node-RED](https://nodered.org/) pour visualiser les données reçues via MQTT.

3. **Bibliothèques Arduino** :
   - WiFi (par Espressif)
   - NTRIPClient (par drotek)
   - DallasTemperature (par Miles Burton)
   - PubSubClient (par Nick O'Leary)
   - TinyGPSPlus (par Mikal Hart)
   - OneWire (par Paul Stoffregen)

### Procédure

1. **Installation d'Arduino IDE**
   - Télécharger et installer Arduino IDE depuis [arduino.cc](https://www.arduino.cc/en/software).
   - Ajouter le support ESP32 en suivant les instructions de [GitHub ESP32](https://github.com/espressif/arduino-esp32).

2. **Installation des Bibliothèques**

   - Ouvrir Arduino IDE.
   - Paramétrer le chemin des bibliothèques nécessaires via **fichier>paramètres>le dossier téléchargé**
   - le dossier contient les sources :
     - WiFi (par Espressif)
     - NTRIPClient (par drotek)
     - DallasTemperature (par Miles Burton)
     - PubSubClient (par Nick O'Leary)
     - TinyGPSPlus (par Mikal Hart)
     - OneWire (par Paul Stoffregen)

3. **Configuration du Code**

   - Copier-coller le code fourni dans Arduino IDE.
   - Sélectionner le type de carte LilyGO T-PCIe (ESP32) dans **Outils > Type de Carte**.

4. **Paramétrage du Programme**

   - Remplacer les valeurs par défaut (SSID WiFi, mots de passe, adresses MQTT, etc.) par les paramètres de votre environnement.

5. **Téléversement du Code**

   - Connecter LilyGO T-PCIe à votre ordinateur via un câble USB.
   - Sélectionner le port correct dans **Outils > Port**.
   - Cliquer sur le bouton de téléversement dans Arduino IDE pour charger le code sur la carte LilyGO.

6. **Lancement et Vérification**

   - Ouvrir le moniteur série dans Arduino IDE pour voir les messages de journalisation.
   - Vérifier que les données GNSS et de température sont affichées correctement.
   - Configurer et utiliser Node-RED pour écouter le sujet MQTT spécifié (`lilygo/data`) et visualiser les données envoyées par le programme.

---

## Crédits des Licences

- Les bibliothèques utilisées dans ce projet sont toutes open-source et sous licence respective de leurs auteurs. Consultez les liens fournis pour plus d'informations sur chaque bibliothèque :

  - [WiFi](https://github.com/espressif/arduino-esp32)
  - [NTRIPClient](https://github.com/drotek/NTRIPClient)
  - [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
  - [OneWire](https://github.com/PaulStoffregen/OneWire)
  - [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
  - [PubSubClient](https://github.com/knolleary/pubsubclient)