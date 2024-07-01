#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <PubSubClient.h>
#include <BluetoothSerial.h>

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

// Définir le niveau de log actuel
#define LOG_LEVEL LOG_LEVEL_DEBUG

void logMessage(int level, const char* message);
void logMessage(int level, const char* message, int value);
void logMessage(int level, const char* message, const char* value);
void logMessage(int level, const char* message, float value, int decimals = 2);
void logMessage(int level, const char* message, int32_t value);
void logMessage(int level, const char* message, uint32_t value);

void locateDevices();
void printDeviceAddresses();
void connectToWiFi();
void requestSourceTable();
void requestMountPointRawData();
void handleMQTTConnection();
void readGNSSData();
void displayGNSSData();
void readTemperatureSensors();
void publishMQTTData();
void handleNTRIPData();
void handleSerialData();
void printAddress(DeviceAddress deviceAddress);
void setup_wifi();
void reconnectMQTT();
void reconnectNTRIP();
void callback(char* topic, byte* message, unsigned int length);
void setup_bt();

BluetoothSerial SerialBT;
HardwareSerial MySerial(1);
#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25

#define ONE_WIRE_BUS 0

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress Thermometer;
int deviceCount = 0;
float temp = 0;

const char* ssid = "buoy";
const char* password = "12345678";
const char* mqtt_server = "192.168.182.208"; //"mavi-mqtt.centipede.fr";
const int mqtt_port = 1883; //8090;
const char* mqtt_output = "lilygo/data";
const char* mqtt_input = "lilygo/input";
const char* mqtt_log = "lilygo/log";
const char* mqtt_user = "LilyGo";
const char* mqtt_password = "password";
const char mqtt_UUID[] = "LilyGo-RT";
const int publish_freq = 1000; //frequence de publication des messages en millisecondes

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int timeInterval = 1000;

TinyGPSPlus gps;

IPAddress server(192, 168, 1, 100);
int port = 80;

char* host = "caster.centipede.fr";
int httpPort = 2101;
char* mntpnt = "CT";
char* user = "rover-gnss-tester";
char* passwd = "";

//ntrip client
NTRIPClient ntrip_c;
const char* udpAddress = "192.168.1.255";
const int udpPort = 9999;
int trans = 3;

WiFiUDP udp;

unsigned long lastWifiReconnectAttempt = 0;
const unsigned long wifiReconnectInterval = 10000; // 10 seconds

unsigned long lastMqttReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 5000; // 5 seconds

unsigned long lastNtripReconnectAttempt = 0;
const unsigned long ntripReconnectInterval = 10000; // 10 seconds

bool ntripConnected = false;

TaskHandle_t nmeaTaskHandle;
void nmeaTask(void *parameter);

SemaphoreHandle_t xSemaphore;


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

    // Initialiser le mutex
    xSemaphore = xSemaphoreCreateMutex();

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
}

void logMessage(int level, const char* message) {
    if (level <= LOG_LEVEL) {
        Serial.println(message);
    }
}

void logMessage(int level, const char* message, int value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void logMessage(int level, const char* message, const char* value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void logMessage(int level, const char* message, float value, int decimals) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value, decimals);
    }
}

void logMessage(int level, const char* message, int32_t value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void logMessage(int level, const char* message, uint32_t value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void locateDevices() {
    logMessage(LOG_LEVEL_INFO, "Locating devices...");
    logMessage(LOG_LEVEL_INFO, "Found ", sensors.getDeviceCount());
}

void printDeviceAddresses() {
    logMessage(LOG_LEVEL_INFO, "Printing addresses...");
    for (int i = 0; i < sensors.getDeviceCount(); i++) {
        logMessage(LOG_LEVEL_INFO, "Sensor ", i + 1);
        sensors.getAddress(Thermometer, i);
        printAddress(Thermometer);
    }
}

void connectToWiFi() {
    logMessage(LOG_LEVEL_INFO, "Connecting to ", ssid);
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiReconnectInterval) {
        vTaskDelay(100 / portTICK_PERIOD_MS); // Utiliser vTaskDelay au lieu de delay()
        logMessage(LOG_LEVEL_DEBUG, ".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        logMessage(LOG_LEVEL_INFO, "WiFi connected");
        logMessage(LOG_LEVEL_INFO, "IP address: ", WiFi.localIP().toString().c_str());

        // Reinitialize MQTT and NTRIP connections after WiFi reconnects
        if (client.connected()) {
            client.disconnect();
        }
        reconnectMQTT();

        if (ntrip_c.connected()) {
            ntrip_c.stop();
        }
        reconnectNTRIP();
    } else {
        logMessage(LOG_LEVEL_ERROR, "WiFi connection failed");
    }
}

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

void handleMQTTConnection() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
}

void readGNSSData() {
    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
        while (MySerial.available() > 0) {
            char c = MySerial.read();
            gps.encode(c);
        }
        xSemaphoreGive(xSemaphore);
    }
}


void displayGNSSData() {
    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
        if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid() && gps.altitude.isValid() && gps.satellites.isValid()) {
            char timeBuffer[30];
            snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d",
                     gps.date.year(), gps.date.month(), gps.date.day(),
                     gps.time.hour(), gps.time.minute(), gps.time.second());

            logMessage(LOG_LEVEL_INFO, "Time: ", timeBuffer);
            logMessage(LOG_LEVEL_INFO, "LONG = ", gps.location.lng(), 8);
            logMessage(LOG_LEVEL_INFO, "LAT = ", gps.location.lat(), 8);
            logMessage(LOG_LEVEL_INFO, "ALT = ", gps.altitude.meters(), 3);
            logMessage(LOG_LEVEL_INFO, "Quality = ", gps.hdop.value());
            logMessage(LOG_LEVEL_INFO, "Satellites = ", gps.satellites.value());
        } else {
            logMessage(LOG_LEVEL_WARN, "GNSS data not available");
        }
        xSemaphoreGive(xSemaphore);
    }
}

void publishMQTTData() {
    if (client.connected() && gps.location.isValid() && gps.date.isValid() && gps.time.isValid() && gps.altitude.isValid() && gps.satellites.isValid()) {
        char timeBuffer[30];
        snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
        
        float waterTemp = sensors.getTempCByIndex(0);
        if (waterTemp == DEVICE_DISCONNECTED_C) {
            waterTemp = 0; // Set temperature to 0 if no sensor is connected
        }

        String json = "{\"user\":\"" + (String)mqtt_user +
                      "\",\"Temperature_Water\":\"" + String(waterTemp) +
                      "\",\"Lon\":\"" + String(gps.location.lng(), 8) +
                      "\",\"Lat\":\"" + String(gps.location.lat(), 8) +
                      "\",\"Alt\":\"" + String(gps.altitude.meters(), 3) +
                      "\",\"Quality\":\"" + String(gps.hdop.value()) +
                      "\",\"Satellites\":\"" + String(gps.satellites.value()) +
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


void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        Serial.print("0x");
        if (deviceAddress[i] < 0x10) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
        if (i < 7) Serial.print(", ");
    }
    Serial.println("");
}

void setup_wifi() {
    delay(10);
    logMessage(LOG_LEVEL_INFO, "Connecting to ", ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        logMessage(LOG_LEVEL_DEBUG, ".");
    }
    logMessage(LOG_LEVEL_INFO, "WiFi connected");
    logMessage(LOG_LEVEL_INFO, "IP address: ", WiFi.localIP().toString().c_str());
}

void setup_bt() {
    // Initialiser le Bluetooth
    if (!SerialBT.begin("rover-gnss")) {
        logMessage(LOG_LEVEL_INFO, "An error occurred initializing Bluetooth");
    } else {
        logMessage(LOG_LEVEL_INFO, "Bluetooth initialized with name 'rover-gnss'");
    }
}

void reconnectMQTT() {
    unsigned long startAttemptTime = millis();
    while (!client.connected() && millis() - startAttemptTime < mqttReconnectInterval) {
        logMessage(LOG_LEVEL_INFO, "Attempting MQTT connection...");
        if (client.connect(mqtt_UUID, mqtt_user, mqtt_password)) {
            logMessage(LOG_LEVEL_INFO, "connected");
            client.subscribe(mqtt_input);
            break;
        } else {
            logMessage(LOG_LEVEL_ERROR, "failed, rc=", client.state());
            logMessage(LOG_LEVEL_INFO, " try again in 5 seconds");
            vTaskDelay(500 / portTICK_PERIOD_MS); // Utiliser vTaskDelay au lieu de delay()
        }
    }

    if (!client.connected()) {
        logMessage(LOG_LEVEL_ERROR, "MQTT connection failed");
    }
}

void reconnectNTRIP() {
    logMessage(LOG_LEVEL_INFO, "Attempting to reconnect to NTRIP caster...");
    unsigned long startAttemptTime = millis();
    while (!ntrip_c.connected() && millis() - startAttemptTime < ntripReconnectInterval) {
        if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
            logMessage(LOG_LEVEL_ERROR, "NTRIP reconnect failed. Will retry...");
            ntripConnected = false;
            vTaskDelay(500 / portTICK_PERIOD_MS); // Utiliser vTaskDelay au lieu de delay()
        } else {
            logMessage(LOG_LEVEL_INFO, "NTRIP reconnected successfully.");
            ntripConnected = true;
            break;
        }
    }

    if (!ntrip_c.connected()) {
        logMessage(LOG_LEVEL_ERROR, "NTRIP connection failed");
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
