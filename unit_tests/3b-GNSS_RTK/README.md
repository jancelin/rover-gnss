# esp32 avec client ntrip embarqué

Code pour utiliser un esp32 pour injecter directement le rtcm3 dans le récepteur GNSS et un rs232 ou bluetooth en sortie utile pour fournir une trame nmea corrigée sur une console qui ne gere pas le rtk  
  
Librairies
- https://github.com/GLAY-AK2/NTRIP-client-for-Arduino  
esp32 maj : prend en charge les nouvelles cartes bien plus performante comme esp32-s3 (pas de bluetooth standard), esp32-c3(premier choix) ...  
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json  

Pour une utilisation bluetooth, il faut un esp32 standard. le code pour c3 sera prochainement disponible

## Branchements avec F9P

![image](https://github.com/user-attachments/assets/eceb0724-3493-4ac6-a98e-df5c3475c9f9)

|  ESP32       |   F9P   |
|--------------|-----------|
|  RX (pin 16) | UART1 (TX)|
|  TX (pin 17) | UART1 (RX)|
|  Vin (5V)    | UART1 (5V)|
|  GND         | UART1 (Gnd)|

## Branchements avec UM980
![sch_UM980-ESP32](https://github.com/user-attachments/assets/54ff61c4-19b8-4220-a641-435edf369864)
Le montage consiste simplement à connecter le port série `Serial1` ( PIN TX 16 & RX 17) de l'ESP32 au port série `UART1` du recepteur UM980.

|  ESP32       |   UM980   |
|--------------|-----------|
|  RX (pin 16) | UART1 pin 2 (TX)|
|  TX (pin 17) | UART1 pin 1 (RX)|
|  Vin (5V)    | UART1 pin 12 (5V)|
|  GND         | UART1 pin 11 (Gnd)|

## Parametres de configuration Arduino ( pour le téléversement ) 

Menu Outils > Partition Schema > No OTA ( 2 Mb App / 2 Mb SPIFF )

![Capture d’écran 2022-08-29 192657](https://user-images.githubusercontent.com/32975584/187261824-5b02ef2c-bc4a-482e-aa8f-ffc1788b9145.png)
