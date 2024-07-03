// Connectivity.h
#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include "Config.h"
#include "Logger.h"
#include <WebServer.h>

extern WebServer webServer;
void setup_wifi();
void connectToWiFi();
void reconnectMQTT();
void setup_bt();
void reconnectNTRIP();
void setupWebServer();
void handleRoot();
void handleSave();
void handleNotFound();
void checkWiFiStatus();
void switchToAPMode();
void tryReconnectWiFi();

#endif // CONNECTIVITY_H
