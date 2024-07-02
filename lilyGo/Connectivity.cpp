// Connectivity.cpp
#include "Connectivity.h"

WebServer webServer(80);

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

void setupWebServer() {
    webServer.on("/", handleRoot);
    webServer.on("/save", handleSave);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
    logMessage(LOG_LEVEL_INFO, "HTTP server started");
}

void handleRoot() {
    String html = "<html><body><h1>Configuration</h1>"
                  "<form action=\"/save\" method=\"post\">"
                  "SSID: <input type=\"text\" name=\"ssid\" value=\"" + String(ssid) + "\"><br>"
                  "Password: <input type=\"text\" name=\"password\" value=\"" + String(password) + "\"><br>"
                  "MQTT Server: <input type=\"text\" name=\"mqtt_server\" value=\"" + String(mqtt_server) + "\"><br>"
                  "MQTT Port: <input type=\"text\" name=\"mqtt_port\" value=\"" + String(mqtt_port) + "\"><br>"
                  "MQTT Output: <input type=\"text\" name=\"mqtt_output\" value=\"" + String(mqtt_output) + "\"><br>"
                  "MQTT Input: <input type=\"text\" name=\"mqtt_input\" value=\"" + String(mqtt_input) + "\"><br>"
                  "MQTT Log: <input type=\"text\" name=\"mqtt_log\" value=\"" + String(mqtt_log) + "\"><br>"
                  "MQTT User: <input type=\"text\" name=\"mqtt_user\" value=\"" + String(mqtt_user) + "\"><br>"
                  "MQTT Password: <input type=\"text\" name=\"mqtt_password\" value=\"" + String(mqtt_password) + "\"><br>"
                  "Publish Frequency: <input type=\"text\" name=\"publish_freq\" value=\"" + String(publish_freq) + "\"><br>"
                  "NTRIP Host: <input type=\"text\" name=\"host\" value=\"" + String(host) + "\"><br>"
                  "NTRIP Port: <input type=\"text\" name=\"httpPort\" value=\"" + String(httpPort) + "\"><br>"
                  "NTRIP Mount Point: <input type=\"text\" name=\"mntpnt\" value=\"" + String(mntpnt) + "\"><br>"
                  "NTRIP User: <input type=\"text\" name=\"user\" value=\"" + String(user) + "\"><br>"
                  "NTRIP Password: <input type=\"text\" name=\"passwd\" value=\"" + String(passwd) + "\"><br>"
                  "<input type=\"submit\" value=\"Save\">"
                  "</form></body></html>";
    webServer.send(200, "text/html", html);
}

void handleSave() {
    String ssidInput = webServer.arg("ssid");
    String passwordInput = webServer.arg("password");
    String mqtt_serverInput = webServer.arg("mqtt_server");
    int mqtt_portInput = webServer.arg("mqtt_port").toInt();
    String mqtt_outputInput = webServer.arg("mqtt_output");
    String mqtt_inputInput = webServer.arg("mqtt_input");
    String mqtt_logInput = webServer.arg("mqtt_log");
    String mqtt_userInput = webServer.arg("mqtt_user");
    String mqtt_passwordInput = webServer.arg("mqtt_password");
    int publish_freqInput = webServer.arg("publish_freq").toInt();
    String hostInput = webServer.arg("host");
    int httpPortInput = webServer.arg("httpPort").toInt();
    String mntpntInput = webServer.arg("mntpnt");
    String userInput = webServer.arg("user");
    String passwdInput = webServer.arg("passwd");

    ssidInput.toCharArray(ssid, sizeof(ssid));
    passwordInput.toCharArray(password, sizeof(password));
    mqtt_serverInput.toCharArray(mqtt_server, sizeof(mqtt_server));
    mqtt_port = mqtt_portInput;
    mqtt_outputInput.toCharArray(mqtt_output, sizeof(mqtt_output));
    mqtt_inputInput.toCharArray(mqtt_input, sizeof(mqtt_input));
    mqtt_logInput.toCharArray(mqtt_log, sizeof(mqtt_log));
    mqtt_userInput.toCharArray(mqtt_user, sizeof(mqtt_user));
    mqtt_passwordInput.toCharArray(mqtt_password, sizeof(mqtt_password));
    publish_freq = publish_freqInput;
    hostInput.toCharArray(host, sizeof(host));
    httpPort = httpPortInput;
    mntpntInput.toCharArray(mntpnt, sizeof(mntpnt));
    userInput.toCharArray(user, sizeof(user));
    passwdInput.toCharArray(passwd, sizeof(passwd));

    savePreferences();

    webServer.send(200, "text/html", "<html><body><h1>Saved. Restarting...</h1></body></html>");
    delay(2000);
    ESP.restart();
}

void handleNotFound() {
    webServer.send(404, "text/plain", "Not Found");
}