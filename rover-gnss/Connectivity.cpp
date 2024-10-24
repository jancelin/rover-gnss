// Connectivity.cpp
#include "Connectivity.h"
#include <ESPmDNS.h>

WebServer webServer(80);
bool isAPMode = false;

void setup_wifi() {
    delay(10);
    logMessage(LOG_LEVEL_INFO, "Try Connecting to Wifi:", ssid);
    WiFi.begin(ssid, password);

    // Wait for connection with a timeout
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
        delay(500);
        logMessage(LOG_LEVEL_DEBUG, "");
    }

    if (WiFi.status() == WL_CONNECTED) {
        logMessage(LOG_LEVEL_INFO, "WiFi connected");
        logMessage(LOG_LEVEL_INFO, "IP address: ", WiFi.localIP().toString().c_str());
    } else {
        logMessage(LOG_LEVEL_ERROR, "WiFi connection failed");
    }
}

void checkWiFiStatus() {
    if (isAPMode) {
        webServer.handleClient(); // Handle web server requests

        // Vérifier toutes les minutes si le Wi-Fi est revenu
        if (millis() - lastAPCheckTime > apCheckInterval) {
            lastAPCheckTime = millis();
            logMessage(LOG_LEVEL_INFO, "Checking if WiFi is available...");

            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid, password);

            unsigned long startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) { //après 20s passage en wifi AP
                delay(1000);
                logMessage(LOG_LEVEL_DEBUG, "Research WIFI...");
            }

            if (WiFi.status() != WL_CONNECTED) {
                logMessage(LOG_LEVEL_ERROR, "WiFi still not available, reverting to AP mode");
                switchToAPMode();
                return; // Sortir de loop pour éviter d'exécuter le reste du code
            } else {
                logMessage(LOG_LEVEL_INFO, "WiFi reconnected");
                logMessage(LOG_LEVEL_INFO, "IP address: ", WiFi.localIP().toString().c_str());
                isAPMode = false;
                connectToWiFi();
                //requestSourceTable();
                requestMountPointRawData();
                reconnectMQTT();
                reconnectNTRIP();
            }
        }
    }
}

void switchToAPMode() {
    logMessage(LOG_LEVEL_ERROR, "Switching to AP mode");
    WiFi.softAP("rover-gnss-AP", "12345678");
    logMessage(LOG_LEVEL_INFO, "AP IP address: ", WiFi.softAPIP().toString().c_str());
    isAPMode = true;
    lastAPCheckTime = millis(); // Initialiser le timer pour le mode AP

    // Réinitialiser mDNS en mode AP
    MDNS.end();  // Arrêter mDNS s'il était en cours d'exécution
    if (!MDNS.begin("rover")) {  // Redémarrer mDNS avec le même nom
        logMessage(LOG_LEVEL_ERROR, "Error setting up MDNS responder in AP mode!");
        while (1) {
            delay(1000);
        }
    }
    logMessage(LOG_LEVEL_INFO, "mDNS responder started in AP mode");
    return;
}

void tryReconnectWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        logMessage(LOG_LEVEL_ERROR, "WiFi connection failed. Switching to AP mode during (minutes):", (uint32_t)apCheckInterval /1000 /60);
        switchToAPMode();
    }
}

void connectToWiFi() {
    logMessage(LOG_LEVEL_INFO, "Try Connecting to ", ssid);
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiReconnectInterval) {
        vTaskDelay(100 / portTICK_PERIOD_MS); // Utiliser vTaskDelay au lieu de delay()
        logMessage(LOG_LEVEL_DEBUG, "Try Wifi connection...");
    }

    if (WiFi.status() == WL_CONNECTED) {
        logMessage(LOG_LEVEL_INFO, "WiFi connected");
        logMessage(LOG_LEVEL_INFO, "IP address: ", WiFi.localIP().toString().c_str());
        isAPMode = false; // Reset AP mode flag

        // Reinitialize MQTT and NTRIP connections after WiFi reconnects
        if (!isAPMode) {
            if (client_mqtt.connected()) {
                client_mqtt.disconnect();
            }
            reconnectMQTT();

            if (ntrip_c.connected()) {
                ntrip_c.stop();
            }
            reconnectNTRIP();
        }
    } else {
        logMessage(LOG_LEVEL_ERROR, "WiFi connection failed");
        switchToAPMode();
        isAPMode = true;
    }
}

void reconnectMQTT() {
    if (!mqtt_enabled || isAPMode) return;
    unsigned long startAttemptTime = millis();
    while (!client_mqtt.connected() && millis() - startAttemptTime < mqttReconnectInterval) {
        logMessage(LOG_LEVEL_INFO, "Attempting MQTT connection...");
        if (client_mqtt.connect(mqtt_UUID, mqtt_user, mqtt_password)) {
            logMessage(LOG_LEVEL_INFO, "connected");
            client_mqtt.subscribe(mqtt_input);
            break;
        } else {
            logMessage(LOG_LEVEL_ERROR, "failed, rc=", client_mqtt.state());
            logMessage(LOG_LEVEL_INFO, " try again in 5 seconds");
            vTaskDelay(500 / portTICK_PERIOD_MS); // Utiliser vTaskDelay au lieu de delay()
        }
    }

    if (!client_mqtt.connected()) {
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
    //webServer.on("/ip", handleIP);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
    logMessage(LOG_LEVEL_INFO, "HTTP server started");
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
    bool mqtt_enabledInput = webServer.hasArg("mqtt_enabled");

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
    mqtt_enabled = mqtt_enabledInput;

    savePreferences();

    webServer.send(200, "text/html", "<html><body><h1>Saved. Restarting...</h1></body></html>");
    delay(2000);
    ESP.restart();
}

void handleNotFound() {
    webServer.send(404, "text/plain", "Not Found");
}

void handleRoot() {
    String ipAddress = WiFi.localIP().toString(); // Obtenir l'adresse IP actuelle

    String html = R"rawliteral(
        <html>
        <head>
            <title>Rover RTK &#x1F41B;</title>
        </head>
        <body>
            <h1>Rover RTK &#x1F41B;</h1>
            <p>Current IP Address: )rawliteral" + ipAddress + R"rawliteral(</p>
            <form action="/save" method="post">
                <h2>WiFi Configuration</h2>
                <label for="ssid">SSID:</label>
                <input type="text" id="ssid" name="ssid" value=")rawliteral" + String(ssid) + R"rawliteral("><br>
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" value=")rawliteral" + String(password) + R"rawliteral("><br>
                <button type="button" onclick="togglePasswordVisibility('password')">Show/Hide</button><br>

                <h2>NTRIP Configuration</h2>
                <label for="host">NTRIP Host:</label>
                <input type="text" id="host" name="host" value=")rawliteral" + String(host) + R"rawliteral("><br>
                <label for="httpPort">NTRIP Port:</label>
                <input type="number" id="httpPort" name="httpPort" value=")rawliteral" + String(httpPort) + R"rawliteral("><br>
                <label for="mntpnt">NTRIP Mount Point:</label>
                <input type="text" id="mntpnt" name="mntpnt" value=")rawliteral" + String(mntpnt) + R"rawliteral("><br>
                <label for="user">NTRIP User:</label>
                <input type="text" id="user" name="user" value=")rawliteral" + String(user) + R"rawliteral("><br>
                <label for="passwd">NTRIP Password:</label>
                <input type="password" id="passwd" name="passwd" value=")rawliteral" + String(passwd) + R"rawliteral("><br>
                <button type="button" onclick="togglePasswordVisibility('passwd')">Show/Hide</button><br>

                <h2>MQTT Configuration</h2>
                <label for="mqtt_enabled">Enable MQTT:</label>
                <input type="checkbox" id="mqtt_enabled" name="mqtt_enabled" )rawliteral" + String(mqtt_enabled ? "checked" : "") + R"rawliteral(><br>
                <label for="mqtt_server">MQTT Server:</label>
                <input type="text" id="mqtt_server" name="mqtt_server" value=")rawliteral" + String(mqtt_server) + R"rawliteral("><br>
                <label for="mqtt_port">MQTT Port:</label>
                <input type="number" id="mqtt_port" name="mqtt_port" value=")rawliteral" + String(mqtt_port) + R"rawliteral("><br>
                <label for="mqtt_output">MQTT Output:</label>
                <input type="text" id="mqtt_output" name="mqtt_output" value=")rawliteral" + String(mqtt_output) + R"rawliteral("><br>
                <label for="mqtt_input">MQTT Input:</label>
                <input type="text" id="mqtt_input" name="mqtt_input" value=")rawliteral" + String(mqtt_input) + R"rawliteral("><br>
                <label for="mqtt_log">MQTT Log:</label>
                <input type="text" id="mqtt_log" name="mqtt_log" value=")rawliteral" + String(mqtt_log) + R"rawliteral("><br>
                <label for="mqtt_user">MQTT User:</label>
                <input type="text" id="mqtt_user" name="mqtt_user" value=")rawliteral" + String(mqtt_user) + R"rawliteral("><br>
                <label for="mqtt_password">MQTT Password:</label>
                <input type="password" id="mqtt_password" name="mqtt_password" value=")rawliteral" + String(mqtt_password) + R"rawliteral("><br>
                <button type="button" onclick="togglePasswordVisibility('mqtt_password')">Show/Hide</button><br>
                <label for="publish_freq">Publish Frequency:</label>
                <input type="number" id="publish_freq" name="publish_freq" value=")rawliteral" + String(publish_freq) + R"rawliteral("><br>

                <h2>SAVE Configurations</h2>
                <input type="submit" value="Save">
            </form>
            <script>
                function togglePasswordVisibility(id) {
                    var field = document.getElementById(id);
                    var fieldType = field.getAttribute('type') === 'password' ? 'text' : 'password';
                    field.setAttribute('type', fieldType);
                }
            </script>
        </body>
        </html>
    )rawliteral";
    webServer.send(200, "text/html", html);
}
