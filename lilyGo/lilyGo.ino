#include "Config.h"
#include "Logger.h"
#include "Connectivity.h"

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

    loadPreferences();

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    // Initialiser le serveur web
    setupWebServer();
    
    connectToWiFi();
    requestSourceTable();
    requestMountPointRawData();

    // Initialiser le mutex
    xSemaphore = xSemaphoreCreateMutex();

    // init Bluetooth
    setup_bt();



    // Création de la tâche FreeRTOS pour les données NMEA
    xTaskCreatePinnedToCore(
        nmeaTask,          // Fonction de la tâche
        "NMEA Task",       // Nom de la tâche
        4096,              // Taille de la pile (en mots, non octets)
        NULL,              // Paramètre de la tâche
        2,                 // Priorité de la tâche
        &nmeaTaskHandle,   // Handle de la tâche
        1                  // Épingler à l'autre cœur (esp32 a 2 cœurs, 0 et 1)
    );

    // Création de la tâche FreeRTOS pour la publication des données JSON
    xTaskCreatePinnedToCore(
        publishTask,       // Fonction de la tâche
        "Publish Task",    // Nom de la tâche
        4096,              // Taille de la pile (en mots, non octets)
        NULL,              // Paramètre de la tâche
        1,                 // Priorité de la tâche
        NULL,              // Pas besoin de handle
        0                  // Épingler à l'autre cœur (esp32 a 2 cœurs, 0 et 1)
    );
}


void loop() {
    long now = millis();

    // Handle Wi-Fi reconnection
    if (WiFi.status() != WL_CONNECTED && (now - lastWifiReconnectAttempt > wifiReconnectInterval)) {
        lastWifiReconnectAttempt = now;
        connectToWiFi();
    }

    // Handle MQTT reconnection
    if (!client.connected() && (now - lastMqttReconnectAttempt > mqttReconnectInterval)) {
        lastMqttReconnectAttempt = now;
        reconnectMQTT();
    } else {
        client.loop();
    }

    // Handle NTRIP reconnection
    if (!ntripConnected && (now - lastNtripReconnectAttempt > ntripReconnectInterval)) {
        lastNtripReconnectAttempt = now;
        reconnectNTRIP();
    }

    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
        readGNSSData();
        xSemaphoreGive(xSemaphore);
    }

    handleNTRIPData();
    webServer.handleClient(); // Handle web server requests
}

//NTRIP
void requestSourceTable() {
    logMessage(LOG_LEVEL_INFO, "Requesting SourceTable.");
    if (ntrip_c.reqSrcTbl(host, httpPort)) {
        char buffer[512];
        delay(5);
        while (ntrip_c.available()) {
            ntrip_c.readLine(buffer, sizeof(buffer));
        }
    } else {
        logMessage(LOG_LEVEL_ERROR, "SourceTable request error");
    }
    logMessage(LOG_LEVEL_INFO, "Requesting SourceTable is OK");
    ntrip_c.stop();
}

void requestMountPointRawData() {
    logMessage(LOG_LEVEL_INFO, "Requesting MountPoint's Raw data");
    if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
        delay(15000);
        ESP.restart();
    }
    logMessage(LOG_LEVEL_INFO, "Requesting MountPoint is OK");
    ntripConnected = true;
}

void readGNSSData() {
    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
        while (MySerial.available() > 0) {
            char c = MySerial.read();
            gps.encode(c);
             // Afficher la qualité GPS lorsque le champ est mis à jour
            if (gnssFixMode.isUpdated()) {
                logMessage(LOG_LEVEL_INFO, "GNSS fix mode: ", gnssFixMode.value());
            }
        }
        xSemaphoreGive(xSemaphore);
    }
}

//MQTT
void handleMQTTConnection() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
}

void publishMQTTData() {
    if (client.connected() && gps.location.isValid() && gps.date.isValid() && gps.time.isValid() && gps.altitude.isValid() && gps.satellites.isValid()) {
        char timeBuffer[30];
        snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d,%02d",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second(),gps.time.centisecond());
        
        float waterTemp = sensors.getTempCByIndex(0);
        if (waterTemp == DEVICE_DISCONNECTED_C) {
            waterTemp = 0; // Set temperature to 0 if no sensor is connected
        }
        //transform NMEA geoid+alt to elevation
        float alt = gps.altitude.meters();
        float geoid_elev = atof(gnssGeoidElv.value());
        float elevation = alt + geoid_elev;

        String json = "{\"user\":\"" + (String)mqtt_user +
                      "\",\"Lon\":\"" + String(gps.location.lng(), 9) +
                      "\",\"Lat\":\"" + String(gps.location.lat(), 9) +
                      //"\",\"Geoid_elev\":\"" + String(gnssGeoidElv.value()) +
                      //"\",\"Alt\":\"" + String(gps.altitude.meters(), 3) +
                      "\",\"Elev\":\"" + String(elevation,3) +
                      "\",\"Sat\":\"" + String(gps.satellites.value()) +
                      "\",\"fix\":\"" + String(gnssFixMode.value()) + //1.gps fix 2.dgps 4.RTK 5.Float
                      "\",\"HDOP\":\"" + String(gnssHDOP.value(), 4) +
                      "\",\"PDOP\":\"" + String(gnssPDOP.value(), 4) +
                      "\",\"Temp\":\"" + String(waterTemp) +
                      "\",\"Time\":\"" + String(timeBuffer) + "\"}";
        
        if (client.publish(mqtt_output, json.c_str())) {
            logMessage(LOG_LEVEL_INFO, "Mqtt sent to : ", mqtt_output);
            logMessage(LOG_LEVEL_INFO, json.c_str());
        } else {
            logMessage(LOG_LEVEL_WARN, "Failed to send MQTT message");
        }
    } else {
        logMessage(LOG_LEVEL_WARN, "GNSS data not valid. Data not sent.");
    }
}

void callback(char* topic, byte* message, unsigned int length) {
    logMessage(LOG_LEVEL_INFO, "Message arrived on topic: ", topic);
    logMessage(LOG_LEVEL_INFO, ". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        logMessage(LOG_LEVEL_INFO, String((char)message[i]).c_str());
        messageTemp += (char)message[i];
    }
    logMessage(LOG_LEVEL_INFO, "");
}

// Capteur
void locateDevices() {
    logMessage(LOG_LEVEL_INFO, "Locating devices...");
    logMessage(LOG_LEVEL_INFO, "Found ", sensors.getDeviceCount());
}

void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        Serial.print("0x");
        if (deviceAddress[i] < 0x10) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
        if (i < 7) Serial.print(", ");
    }
    Serial.println("");
}

void printDeviceAddresses() {
    logMessage(LOG_LEVEL_INFO, "Printing addresses...");
    for (int i = 0; i < sensors.getDeviceCount(); i++) {
        logMessage(LOG_LEVEL_INFO, "Sensor ", i + 1);
        sensors.getAddress(Thermometer, i);
        printAddress(Thermometer);
    }
}

void readTemperatureSensors() {
    sensors.requestTemperatures();
    for (int i = 0; i < sensors.getDeviceCount(); i++) {
        logMessage(LOG_LEVEL_INFO, "Sensor ", i + 1);
        temp = sensors.getTempCByIndex(i);
        if (temp == DEVICE_DISCONNECTED_C) {
            temp = 0; // Set temperature to 0 if no sensor is connected
        }
        logMessage(LOG_LEVEL_INFO, " : ", temp, 2);
        logMessage(LOG_LEVEL_INFO, " °C");
    }
    logMessage(LOG_LEVEL_INFO, "");
}


void handleNTRIPData() {
    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
        bool ntripDataAvailable = false;
        int ntripDataSize = 0;

        while (ntrip_c.available()) {
            char ch = ntrip_c.read();
            MySerial.print(ch);
            ntripDataAvailable = true;
            ntripDataSize++;
        }

        if (!ntripDataAvailable) {
            // logMessage(LOG_LEVEL_WARN, "No NTRIP data available.");
            ntripConnected = false;
        } else {
            logMessage(LOG_LEVEL_DEBUG, "NTRIP data in ", ntripDataSize);
            ntripConnected = true;
        }
        xSemaphoreGive(xSemaphore);
    }// else {
        //logMessage(LOG_LEVEL_WARN, "Failed to take semaphore in handleNTRIPData.");
    //}
}

void handleSerialData() {
    WiFiClient client;
    char buffer[512]; // Buffer pour stocker les données série
    while (MySerial.available()) {
        int len = MySerial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Ajoutez un terminateur nul pour en faire une chaîne de caractères

        // logMessage(LOG_LEVEL_DEBUG, "NMEA Buffer: ", buffer);

        // Encoder les données dans TinyGPSPlus
        for (int i = 0; i < len; i++) {
            gps.encode(buffer[i]);
        }

        switch (trans) {
            case 0:  // serial out
                logMessage(LOG_LEVEL_DEBUG, buffer);
                break;
            case 1:  // udp out
                udp.beginPacket(udpAddress, udpPort);
                udp.write((uint8_t*)buffer, len);
                udp.endPacket();
                break;
            case 2:  // tcp client out
                if (!client.connect(server, port)) {
                    logMessage(LOG_LEVEL_ERROR, "connection failed");
                    return;
                }
                client.write((uint8_t*)buffer, len);
                while (client.connected()) {
                    while (client.available()) {
                        char c = client.read();
                        logMessage(LOG_LEVEL_DEBUG, String(c).c_str());
                    }
                }
                client.stop();
                break;
            case 3:  // MySerial out
                MySerial.write((uint8_t*)buffer, len);
                break;
            case 4:  // MySerial out
                MySerial.write((uint8_t*)buffer, len);
                break;
            default:  // mauvaise config
                logMessage(LOG_LEVEL_ERROR, "mauvais choix ou oubli de configuration");
                break;
        }

        // Envoyer les trames NMEA via Bluetooth immédiatement
        SerialBT.write((uint8_t*)buffer, len);
        SerialBT.write('\n');
    }
}

//TASK
void nmeaTask(void *parameter) {
    while (true) {
        if (xSemaphoreTake(xSemaphore, (TickType_t)50) == pdTRUE) {
            handleSerialData();
            xSemaphoreGive(xSemaphore);
        }
        vTaskDelay(5); // 1= Délai très court pour céder la main à d'autres tâches 5= un petit délai pour améliorer la réactivité
    }
}

void publishTask(void *parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = publish_freq / portTICK_PERIOD_MS; // 1 second

    for(;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (xSemaphoreTake(xSemaphore, (TickType_t)publish_freq ) == pdTRUE) {
            readTemperatureSensors();
            publishMQTTData();
            xSemaphoreGive(xSemaphore);
        } else {
            logMessage(LOG_LEVEL_WARN, "Failed to take semaphore in publishTask.");
        }
    }
}
