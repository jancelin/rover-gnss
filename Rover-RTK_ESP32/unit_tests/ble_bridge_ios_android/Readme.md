# README.md

## Table des matières / Table of Contents

1. [Introduction](#introduction--introduction)
2. [Architecture générale / Overall Architecture](#architecture-générale--overall-architecture)
3. [Description détaillée des fonctions / Detailed Function Descriptions](#description-détaillée-des-fonctions--detailed-function-descriptions)
   - [Classe `ServerCallbacks`](#classe-servercallbacks)
   - [Classe `RxCallbacks`](#classe-rxcallbacks)
   - [`setup()`](#setup)
   - [`loop()`](#loop)
4. [Gestion des trames RTCM3 pour RTK / Handling RTCM3 Frames for RTK](#gestion-des-trames-rtcm3-pour-rtk--handling-rtcm3-frames-for-rtk)
5. [Installation et déploiement / Installation and Deployment](#installation-et-déploiement--installation-and-deployment)
6. [Annexes / Appendices](#annexes--appendices)

---

## Introduction / Introduction

**FR :**
Ce projet met en place un **pont BLE-NUS (Nordic UART Service)** sur un ESP32, destiné à relayer en temps réel les trames NMEA issues d’un récepteur GNSS et les corrections RTCM3 pour obtenir un positionnement **RTK** de haute précision. L’application cliente (ex. SW Maps sur iOS) se connecte en Bluetooth Low Energy et reçoit directement les données de positionnement.

**EN :**
This project implements a **BLE-NUS (Nordic UART Service) bridge** on an ESP32, designed to stream GNSS NMEA sentences and RTCM3 correction frames in real time for **RTK** positioning. The client app (e.g., SW Maps on iOS) connects over BLE and directly receives the positioning data.

---

## Architecture générale / Overall Architecture

- **ESP32** gère :
  - **UART2** (GPIO16 = RX, GPIO17 = TX) à 460800 baud pour communiquer avec le récepteur GNSS.
  - **BLE Classic** (ESP32 Core) pour exposer un service NUS.
- **Service BLE NUS** :
  - **Service UUID** : `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
  - **TX Characteristic** (Notify → client) : `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` avec descriptor `BLE2902`.
  - **RX Characteristic** (Write ← client) : `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` pour renvoyer des trames RTCM3.
- **Flux de données** :
  - **NMEA** : du GNSS → UART2 → Notification BLE.
  - **RTCM3** : du client BLE → Write BLE → UART2 → récepteur GNSS.

---

## Description détaillée des fonctions / Detailed Function Descriptions

### Classe `ServerCallbacks`
**FR :**
Gère les événements de connexion BLE.
- **`onConnect(BLEServer*)`** : appelé lorsqu’un client se connecte, positionne `deviceConnected = true`.
- **`onDisconnect(BLEServer*)`** : appelé à la déconnexion, positionne `deviceConnected = false`.

**EN :**
Manages BLE connection events.
- **`onConnect(BLEServer*)`**: called when a client connects, sets `deviceConnected = true`.
- **`onDisconnect(BLEServer*)`**: called when a client disconnects, sets `deviceConnected = false`.

---

### Classe `RxCallbacks`
**FR :**
Traite les données reçues du client BLE (Write sur la RX char).
- **`onWrite(BLECharacteristic*)`** : récupère une `String` Arduino du client, puis la réécrit sur `Serial2` pour renvoyer les trames RTCM3 au récepteur.

**EN :**
Handles data sent by the BLE client (Write on the RX characteristic).
- **`onWrite(BLECharacteristic*)`**: retrieves an Arduino `String` from the client, then writes it back to `Serial2` to send RTCM3 frames to the GNSS receiver.

---

### Fonction `setup()`
**FR :**
1. **Initialisation Serial USB** à 115200 baud pour les messages de debug.
2. **Initialisation UART2** (`Serial2.begin(460800, SERIAL_8N1, 16, 17)`) pour la réception des trames GNSS.
3. **Initialisation BLE** :
   - `BLEDevice::init("ESP32_NMEA_Bridge")`
   - Création du `BLEServer` avec callbacks.
4. **Création du service NUS** :
   - `createService(NUS_SERVICE_UUID)`.
   - Caractéristique **TX** : Notify + `addDescriptor(new BLE2902())`.
   - Caractéristique **RX** : Write + callback `RxCallbacks`.
   - `service->start()`.
5. **Advertising** :
   - `BLEDevice::getAdvertising()` + `addServiceUUID(NUS_SERVICE_UUID)`.
   - `setScanResponse(true)`, `setMinPreferred(0x06)` et `setMinPreferred(0x12)`.
   - `start()`.

**EN :**
1. **Initialize USB Serial** at 115200 baud for debug logs.
2. **Initialize UART2** (`Serial2.begin(460800, SERIAL_8N1, 16, 17)`) to receive GNSS frames.
3. **Initialize BLE**:
   - `BLEDevice::init("ESP32_NMEA_Bridge")`
   - Create `BLEServer` with callbacks.
4. **Create NUS service**:
   - `createService(NUS_SERVICE_UUID)`.
   - **TX** characteristic: Notify + `addDescriptor(new BLE2902())`.
   - **RX** characteristic: Write + `RxCallbacks`.
   - `service->start()`.
5. **Advertising**:
   - `getAdvertising()` + `addServiceUUID(NUS_SERVICE_UUID)`.
   - `setScanResponse(true)`, `setMinPreferred(0x06)`, `setMinPreferred(0x12)`.
   - `start()`.

---

### Fonction `loop()`
**FR :**
Dans une boucle sans fin :
1. Si **`deviceConnected == true`** :
   - Lire tant que `Serial2.available()` : récupération caractère par caractère.
   - Concaténer dans une variable `String txBuf`.
   - Dès réception d’un `"\n"`, appeler :
     ```cpp
     pTxCharacteristic->setValue(txBuf);
     pTxCharacteristic->notify();
     txBuf = "";
     ```
2. Si **`deviceConnected == false`**, vider `txBuf` pour éviter l’accumulation.

**EN :**
In an infinite loop:
1. If **`deviceConnected == true`**:
   - Read while `Serial2.available()`: fetch byte-by-byte.
   - Append into a `String txBuf`.
   - Upon receiving a `"\n"`, execute:
     ```cpp
     pTxCharacteristic->setValue(txBuf);
     pTxCharacteristic->notify();
     txBuf = "";
     ```
2. If **`deviceConnected == false`**, clear `txBuf` to avoid overflow.

---

## Gestion des trames RTCM3 pour RTK / Handling RTCM3 Frames for RTK
**FR :**
Les corrections différentielles **RTCM3** (utilisées pour le **RTK**) peuvent être envoyées par l’application cliente via la **RX characteristic**. Le callback `RxCallbacks::onWrite()` :
```cpp
void onWrite(BLECharacteristic* pChar) {
  String val = pChar->getValue();
  GNSS.write((const uint8_t*)val.c_str(), val.length());
}
