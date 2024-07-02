// Config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <PubSubClient.h>
#include <BluetoothSerial.h>

// Définition du niveau de log
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

#define LOG_LEVEL LOG_LEVEL_DEBUG

// Pinouts
#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25
#define ONE_WIRE_BUS 0

// WiFi configuration
extern const char* ssid;
extern const char* password;

// MQTT configuration
extern const char* mqtt_server;
extern const int mqtt_port;
extern const char* mqtt_output;
extern const char* mqtt_input;
extern const char* mqtt_log;
extern const char* mqtt_user;
extern const char* mqtt_password;
extern const char mqtt_UUID[];
extern const int publish_freq;

// NTRIP configuration
extern char* host;
extern int httpPort;
extern char* mntpnt;
extern char* user;
extern char* passwd;

// Network configuration
extern IPAddress server;
extern int port;
extern const char* udpAddress;
extern const int udpPort;
extern int trans;

// Timing intervals
extern const unsigned long wifiReconnectInterval;
extern const unsigned long mqttReconnectInterval;
extern const unsigned long ntripReconnectInterval;

// Global variables
extern WiFiClient espClient;
extern PubSubClient client;
extern TinyGPSPlus gps;
extern NTRIPClient ntrip_c;
extern WiFiUDP udp;
extern BluetoothSerial SerialBT;
extern HardwareSerial MySerial;
extern OneWire oneWire;
extern DallasTemperature sensors;
extern DeviceAddress Thermometer;
extern int deviceCount;
extern float temp;
extern unsigned long lastWifiReconnectAttempt;
extern unsigned long lastMqttReconnectAttempt;
extern unsigned long lastNtripReconnectAttempt;
extern bool ntripConnected;
extern TaskHandle_t nmeaTaskHandle;
extern SemaphoreHandle_t xSemaphore;
extern TinyGPSCustom gnssGeoidElv;
extern TinyGPSCustom gnssFixMode;
extern TinyGPSCustom gnssPDOP;
extern TinyGPSCustom gnssHDOP;

#endif // CONFIG_H
