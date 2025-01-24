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
#include <Preferences.h>

// Définition du niveau de log
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

#define LOG_LEVEL LOG_LEVEL_DEBUG

// Connection to use to transmit  data
// RS2323_SERIAL : and connect tx f9p directly to rs232 module
// UDP_SERVER
// TCP_MQTT
// RS2323_MYSERIAL
#define LOG 0
#define UDP_SERVER 1
#define TCP_SERVER 2
#define MYSERIAL 3
#define TRANSMITION_MODE MYSERIAL

// Pinouts
#define PIN_TX 18  //esp32 
#define PIN_RX 19 //esp32
#define POWER_PIN 25
#define ONE_WIRE_BUS 0 // capteur T°C
#define PIN2_TX 16 
#define PIN2_RX 17 // capteur Distancemètre

// WiFi configuration
extern char ssid[64];
extern char password[64];

// MQTT configuration
extern char mqtt_server[64];
extern int mqtt_port;
extern char mqtt_output[64];
extern char mqtt_input[64];
extern char mqtt_log[64];
extern char mqtt_user[64];
extern char mqtt_password[64];
extern const char mqtt_UUID[];
extern int publish_freq;

// NTRIP configuration
extern char host[64];
extern int httpPort;
extern char mntpnt[32];
extern char user[32];
extern char passwd[32];

// Network configuration
extern IPAddress tcp_server;
extern int port;
extern const char* udpAddress;
extern const int udpPort;

// Timing intervals
extern const unsigned long wifiReconnectInterval;
extern const unsigned long mqttReconnectInterval;
extern const unsigned long ntripReconnectInterval;

// Global variables
extern WiFiClient espClient;
extern PubSubClient mqtt_client;
extern TinyGPSPlus gps;
extern NTRIPClient ntrip_c;
extern WiFiUDP udp;
extern BluetoothSerial SerialBT;
extern HardwareSerial MySerial;
extern HardwareSerial MySerial2;
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
//Web
extern Preferences preferences;
void loadPreferences();
void savePreferences();
void resetPreferences();
void requestSourceTable(); 
void requestMountPointRawData();
//Wifi AP
extern bool isAPMode; 
extern unsigned long lastAPCheckTime;
extern const unsigned long apCheckInterval;
//MQTT option
extern bool mqtt_enabled;
//distance
extern float distance;

#endif // CONFIG_H
