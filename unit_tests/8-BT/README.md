Pour configurer le code PIN pour la connexion Bluetooth sur l'ESP32, la bibliothèque `BluetoothSerial` n'a pas de méthode `setPin()`. Nous devons utiliser une méthode alternative pour définir le code PIN. La bibliothèque `BluetoothSerial` sur l'ESP32 n'a pas de méthode directe pour définir un code PIN via l'API Arduino. Vous pouvez définir le code PIN via la commande AT directement sur le module Bluetooth, mais pour notre cas, nous simplifions en configurant seulement le nom Bluetooth.

### Modifications sans `setPin`

### Ajout des Bibliothèques

Ajoutez la bibliothèque `BluetoothSerial` au début du fichier :

```cpp
#include <BluetoothSerial.h>
```

### Déclaration des Objets Bluetooth

Déclarez un objet pour gérer la connexion Bluetooth :

```cpp
BluetoothSerial SerialBT;
```

### Initialisation du Bluetooth dans `setup()`

Initialisez la connexion Bluetooth avec un nom dans la fonction `setup()` :

```cpp
void setup() {
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);

    Serial.begin(115200);
    delay(100);
    MySerial.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    delay(100);

    sensors.begin();
    locateDevices();
    printDeviceAddresses();

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    connectToWiFi();
    requestSourceTable();
    requestMountPointRawData();

    // Initialiser le Bluetooth
    if (!SerialBT.begin("rover-gnss")) {
        Serial.println("An error occurred initializing Bluetooth");
    } else {
        Serial.println("Bluetooth initialized with name 'rover-gnss'");
    }
}
```

### Diffusion des Trames NMEA via Bluetooth

Ajoutez la diffusion des trames NMEA dans la fonction `handleSerialData()` :

```cpp
void handleSerialData() {
    WiFiClient client;
    while (MySerial.available()) {
        String s = MySerial.readStringUntil('\n');
        switch (trans) {
            case 0:  // serial out
                Serial.println(s);
                break;
            case 1:  // udp out
                udp.beginPacket(udpAddress, udpPort);
                udp.print(s);
                udp.endPacket();
                break;
            case 2:  // tcp client out
                if (!client.connect(server, port)) {
                    Serial.println("connection failed");
                    return;
                }
                client.println(s);
                while (client.connected()) {
                    while (client.available()) {
                        char c = client.read();
                        Serial.print(c);
                    }
                }
                client.stop();
                break;
            case 3:  // MySerial out
                MySerial.println(s);
                break;
            case 4:  // MySerial out
                MySerial.println(s);
                break;
            default:  // mauvaise config
                Serial.println("mauvais choix ou oubli de configuration");
                break;
        }

        // Envoyer les trames NMEA via Bluetooth
        SerialBT.println(s);
    }
}
```
