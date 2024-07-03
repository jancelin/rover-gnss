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
        logMessage(LOG_LEVEL_ERROR, "WiFi connection failed. Switching to AP mode during (minutes):", apCheckInterval /1000 /60);
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
        if (!isAPMode) { // Ajoutez cette vérification
            if (client.connected()) {
                client.disconnect();
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
    if (isAPMode) return;
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

void handleRoot() {
    String html = R"rawliteral(
        <html>
        <head>
            <title>Rover GNSS Configuration</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 0;
                    padding: 0;
                    background-color: #f4f4f4;
                }
                .container {
                    padding: 0 15px;
                    margin: 15px;
                    max-width: 90%;
                }
                .header {
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    margin: 20px 0;
                }
                .header img {
                    width: 50px;
                    height: 50px;
                    margin-right: 20px;
                }
                h1 {
                    font-size: 60px;
                    margin: 0;
                    text-align: center;
                }
                h2 {
                    font-size: 50px;
                    margin: 20px 0 10px;
                    color: #333;
                    text-align: center;
                }
                form {
                    display: flex;
                    flex-direction: column;
                }
                label {
                    font-weight: bold;
                    margin: 10px 0 5px;
                    font-size: 36px;
                }
                input[type="text"],
                input[type="password"],
                input[type="number"] {
                    padding: 15px;
                    margin-bottom: 20px;
                    border: 1px solid #ccc;
                    border-radius: 5px;
                    width: 100%;
                    font-size: 40px;
                    box-sizing: border-box;
                    text-align: center;
                }
                input[type="submit"] {
                    background-color: #4CAF50;
                    color: white;
                    padding: 15px;
                    border: none;
                    border-radius: 5px;
                    cursor: pointer;
                    font-size: 50px;
                    margin-top: 20px;
                }
                input[type="submit"]:hover {
                    background-color: #45a049;
                }
                .section {
                    margin-bottom: 30px;
                    padding: 20px;
                    background: #fff;
                    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
                    border-radius: 10px;
                }
                .password-container {
                    position: relative;
                    display: flex;
                    align-items: center;
                }
                .password-container input[type="password"],
                .password-container input[type="text"] {
                    flex: 1;
                }
                .password-container button {
                    position: absolute;
                    right: 10px;
                    padding: 10px;
                    font-size: 20px;
                    border: none;
                    background: #ccc;
                    cursor: pointer;
                    border-radius: 5px;
                }
                .password-container button:hover {
                    background: #bbb;
                }
            </style>
        </head>
        <body>
          <div class="header">
            <div style="text-align: center; line-height: 1;">
              <span style="font-size: 34px; display: block;">&#x1F6F0;&#x1F6F0;&#x1F6F0;</span>
              <br>
              <span style="font-size: 34px;">&#x1F4E1;</span>
              &#x2E;&#x2E;&#x2E;&#x2E; 
              <span style="font-size: 64px;">&#x1F41B;</span>
            </div>
            <h1> Rover GNSS Conf</h1>
          </div>
            </div>
                <form action="/save" method="post">
                    <div class="section">
                        <h2>WiFi Configuration</h2>
                        <label for="ssid">SSID:</label>
                        <input type="text" id="ssid" name="ssid" value=")rawliteral" + String(ssid) + R"rawliteral(">
                        <label for="password">Password:</label>
                        <div class="password-container">
                            <input type="password" id="password" name="password" value=")rawliteral" + String(password) + R"rawliteral(">
                            <button type="button" onclick="togglePasswordVisibility('password')">&#128065;</button>
                        </div>
                    </div>
                    <div class="section">
                        <h2>NTRIP Configuration</h2>
                        <label for="host">NTRIP Host:</label>
                        <input type="text" id="host" name="host" value=")rawliteral" + String(host) + R"rawliteral(">
                        <label for="httpPort">NTRIP Port:</label>
                        <input type="number" id="httpPort" name="httpPort" value=")rawliteral" + String(httpPort) + R"rawliteral(">
                        <label for="mntpnt">NTRIP Mount Point:</label>
                        <input type="text" id="mntpnt" name="mntpnt" value=")rawliteral" + String(mntpnt) + R"rawliteral(">
                        <label for="user">NTRIP User:</label>
                        <input type="text" id="user" name="user" value=")rawliteral" + String(user) + R"rawliteral(">
                        <label for="passwd">NTRIP Password:</label>
                        <div class="password-container">
                            <input type="password" id="passwd" name="passwd" value=")rawliteral" + String(passwd) + R"rawliteral(">
                            <button type="button" onclick="togglePasswordVisibility('passwd')">&#128065;</button>
                        </div>
                    </div>                    
                    <div class="section">
                        <h2>MQTT Configuration</h2>
                        <label for="mqtt_server">MQTT Server:</label>
                        <input type="text" id="mqtt_server" name="mqtt_server" value=")rawliteral" + String(mqtt_server) + R"rawliteral(">
                        <label for="mqtt_port">MQTT Port:</label>
                        <input type="number" id="mqtt_port" name="mqtt_port" value=")rawliteral" + String(mqtt_port) + R"rawliteral(">
                        <label for="mqtt_output">MQTT Output:</label>
                        <input type="text" id="mqtt_output" name="mqtt_output" value=")rawliteral" + String(mqtt_output) + R"rawliteral(">
                        <label for="mqtt_input">MQTT Input:</label>
                        <input type="text" id="mqtt_input" name="mqtt_input" value=")rawliteral" + String(mqtt_input) + R"rawliteral(">
                        <label for="mqtt_log">MQTT Log:</label>
                        <input type="text" id="mqtt_log" name="mqtt_log" value=")rawliteral" + String(mqtt_log) + R"rawliteral(">
                        <label for="mqtt_user">MQTT User:</label>
                        <input type="text" id="mqtt_user" name="mqtt_user" value=")rawliteral" + String(mqtt_user) + R"rawliteral(">
                        <label for="mqtt_password">MQTT Password:</label>
                        <div class="password-container">
                            <input type="password" id="mqtt_password" name="mqtt_password" value=")rawliteral" + String(mqtt_password) + R"rawliteral(">
                            <button type="button" onclick="togglePasswordVisibility('mqtt_password')">&#128065;</button>
                        </div>
                        <label for="publish_freq">Publish Frequency:</label>
                        <input type="number" id="publish_freq" name="publish_freq" value=")rawliteral" + String(publish_freq) + R"rawliteral(">
                    </div>
                    <input type="submit" value="Save">
                </form>
            </div>
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