# F9p-ntrip-esp32-Mqtt

Code pour utiliser un esp32 pour injecter directement le rtcm3 dans le f9p et un envoi JSON en MQTT

ATTENTION UTILISER LA CONFIG EN 2Hz si vous utilisez un MYSERIAL_BAUD_RATE = 115200 [Config F9P en 2Hz](https://github.com/jancelin/rover-gnss/blob/master/conf_GNSS/F9P/F9P_HPG1-32_Rover_2hz_NMEA_gga_gsa_rmc.txt)
!!! Ne fonctionne pas avec la config 5Hz !!! 

Pour passer en 5Hz côté F9P, il faut augmenter le MYSERIAL_BAUD_RATE à 460800

Librairies
https://github.com/GLAY-AK2/NTRIP-client-for-Arduino  
esp32 maj : prend en charge les nouvelles cartes bien plus performante comme esp32-s3 (pas de bluetooth standard), esp32-c3(premier choix) ...  
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json  

Pour une utilisation bluetooth, il faut un esp32 standard. le code pour c3 sera prochainement disponible

![image](https://github.com/user-attachments/assets/eceb0724-3493-4ac6-a98e-df5c3475c9f9)

![Capture d’écran 2022-08-29 192657](https://user-images.githubusercontent.com/32975584/187261824-5b02ef2c-bc4a-482e-aa8f-ffc1788b9145.png)
