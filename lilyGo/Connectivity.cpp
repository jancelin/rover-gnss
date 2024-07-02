// Connectivity.cpp
#include "Connectivity.h"

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

void setup_bt() {
    // Initialiser le Bluetooth
    if (!SerialBT.begin("rover-gnss")) {
        logMessage(LOG_LEVEL_INFO, "An error occurred initializing Bluetooth");
    } else {
        logMessage(LOG_LEVEL_INFO, "Bluetooth initialized with name 'rover-gnss'");
    }
}
