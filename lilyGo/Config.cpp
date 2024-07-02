// Config.cpp
#include "Config.h"

// WiFi configuration
const char* ssid = "ici";
const char* password = "12345678";

// MQTT configuration
const char* mqtt_server = "mavi-mqtt.centipede.fr";
const int mqtt_port = 8090;
const char* mqtt_output = "lilygo/data";
const char* mqtt_input = "lilygo/input";
const char* mqtt_log = "lilygo/log";
const char* mqtt_user = "LilyGo";
const char* mqtt_password = "password";
const char mqtt_UUID[] = "LilyGo-RT";
const int publish_freq = 1000; // fréquence de publication des messages en millisecondes

// NTRIP configuration
char* host = "caster.centipede.fr";
int httpPort = 2101;
char* mntpnt = "CT";
char* user = "rover-gnss-tester";
char* passwd = "";

// Network configuration
IPAddress server(192, 168, 1, 100);
int port = 80;
const char* udpAddress = "192.168.1.255";
const int udpPort = 9999;
int trans = 3;

// Timing intervals
const unsigned long wifiReconnectInterval = 10000; // 10 seconds
const unsigned long mqttReconnectInterval = 5000; // 5 seconds
const unsigned long ntripReconnectInterval = 10000; // 10 seconds

// Global variables
WiFiClient espClient;
PubSubClient client(espClient);
TinyGPSPlus gps;
NTRIPClient ntrip_c;
WiFiUDP udp;
BluetoothSerial SerialBT;
HardwareSerial MySerial(1);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer;
int deviceCount = 0;
float temp = 0;
unsigned long lastWifiReconnectAttempt = 0;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastNtripReconnectAttempt = 0;
bool ntripConnected = false;
TaskHandle_t nmeaTaskHandle;
SemaphoreHandle_t xSemaphore;
TinyGPSCustom gnssGeoidElv(gps, "GNGGA", 11);
TinyGPSCustom gnssFixMode(gps, "GNGGA", 6);
TinyGPSCustom gnssPDOP(gps, "GNGSA", 15);
TinyGPSCustom gnssHDOP(gps, "GNGSA", 16);
