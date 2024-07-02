// Connectivity.h
#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include "Config.h"
#include "Logger.h"

void setup_wifi();
void connectToWiFi();
void reconnectMQTT();
void setup_bt();
void reconnectNTRIP();

#endif // CONNECTIVITY_H
