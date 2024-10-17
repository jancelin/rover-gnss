---
layout: default
parent: Tests unitaires
grand_parent: Satellite Cyclopée
title: Test de réception du temps GNSS
nav_order: 6
has_children: False
---

Test de réception du temps GNSS
===============================

## En bref
Ce programme permet de tester la réception du temps GNSS communiqué par un récepteur GNSS via les trames NMEA `$GPGGA` et `$GPRMC`.

## Matériel
- [Esp32 Feather](https://www.gotronic.fr/art-carte-feather-esp32-v2-ada5400-35213.htm);
- Drotek DP0601 RTK GNSS (XL F9P)
- ou UM980
  
## Bibliothèque
- `TinyGPSPlus`.

## Inspiration
Ce programme est inspiré des exemples présents sur le site [Arduiniana](http://arduiniana.org/libraries/tinygpsplus/).

## Ports
Pour ce montage, le port de communication `Serial` est utilisé pour le debug via USB (moniteur série de l'IDE Arduino) à 115200 baud. <br>
Le récepteur Drotek DP0601 a été configuré pour diffuser les trames NMEA `$GPGGA` et `$GPRMC` sur son port `UART1`.<br>
L'ESP32 utilisera son port `Serialrx` pour recevoir les trames NMEA du récepteur GNSS.

## Branchements avec F9P
![image](https://github.com/user-attachments/assets/eceb0724-3493-4ac6-a98e-df5c3475c9f9)
Le montage consiste simplement à connecter le port série `Serial1` ( PIN TX 16 & RX 17) de l'ESP32 au port série `UART1` du recepteur Drotek.

|ESP32 |DP0601|
|------|------|
|RX|UART1 B3 (TX)|
|Vin (5V)|UART1 B1 (5V)|
|GND|UART1 B6 (Gnd)|

## Branchements avec UM980
![sch_UM980-ESP32](https://github.com/user-attachments/assets/54ff61c4-19b8-4220-a641-435edf369864)
Le montage consiste simplement à connecter le port série `Serial1` ( PIN TX 16 & RX 17) de l'ESP32 au port série `UART1` du recepteur UM980.

|ESP32 |UM980|
|------|------|
|RX( pin 16) | UART1 2 (TX)|
|Vin (5V)    | UART1 12 (5V)|
|GND         | UART1 11 (Gnd)|


