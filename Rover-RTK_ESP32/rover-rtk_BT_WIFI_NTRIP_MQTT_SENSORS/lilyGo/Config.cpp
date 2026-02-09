// Config.cpp
#include "Config.h"

Preferences preferences;

// WiFi configuration
char ssid[64] = "ici";
char password[64] = "12345678";

// MQTT configuration
char mqtt_server[64] = "mavi-mqtt.centipede.fr";
int mqtt_port = 8090;
char mqtt_output[64] = "lilygo/data";
char mqtt_input[64] = "lilygo/input";
char mqtt_log[64] = "lilygo/log";
char mqtt_user[64] = "LilyGo";
char mqtt_password[64] = "password";
const char mqtt_UUID[] = "LilyGo-RT";
int publish_freq = 1000; // fréquence de publication des messages en millisecondes
bool mqtt_enabled = true; // MQTT est activé par défaut

// NTRIP configuration
char host[64] = "caster.centipede.fr";
int httpPort = 2101;
char mntpnt[32] = "CT";
char user[32] = "rover-gnss-tester";
char passwd[32] = "";

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
//wifi
unsigned long lastAPCheckTime = 0;
const unsigned long apCheckInterval = 120000; //check si wifi OK pendant wifi AP

//Web
void loadPreferences() {
    preferences.begin("config", true); // read-only
    String temp;
    
    temp = preferences.getString("ssid", ssid);
    temp.toCharArray(ssid, sizeof(ssid));
    
    temp = preferences.getString("password", password);
    temp.toCharArray(password, sizeof(password));
    
    temp = preferences.getString("mqtt_server", mqtt_server);
    temp.toCharArray(mqtt_server, sizeof(mqtt_server));
    
    mqtt_port = preferences.getInt("mqtt_port", mqtt_port);
    
    temp = preferences.getString("mqtt_output", mqtt_output);
    temp.toCharArray(mqtt_output, sizeof(mqtt_output));
    
    temp = preferences.getString("mqtt_input", mqtt_input);
    temp.toCharArray(mqtt_input, sizeof(mqtt_input));
    
    temp = preferences.getString("mqtt_log", mqtt_log);
    temp.toCharArray(mqtt_log, sizeof(mqtt_log));
    
    temp = preferences.getString("mqtt_user", mqtt_user);
    temp.toCharArray(mqtt_user, sizeof(mqtt_user));
    
    temp = preferences.getString("mqtt_password", mqtt_password);
    temp.toCharArray(mqtt_password, sizeof(mqtt_password));
    
    publish_freq = preferences.getInt("publish_freq", publish_freq);
    
    temp = preferences.getString("host", host);
    temp.toCharArray(host, sizeof(host));
    
    httpPort = preferences.getInt("httpPort", httpPort);
    
    temp = preferences.getString("mntpnt", mntpnt);
    temp.toCharArray(mntpnt, sizeof(mntpnt));
    
    temp = preferences.getString("user", user);
    temp.toCharArray(user, sizeof(user));
    
    temp = preferences.getString("passwd", passwd);
    temp.toCharArray(passwd, sizeof(passwd));

    mqtt_enabled = preferences.getBool("mqtt_enabled", true);
    
    preferences.end();
}

void savePreferences() {
    preferences.begin("config", false); // read-write
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("mqtt_server", mqtt_server);
    preferences.putInt("mqtt_port", mqtt_port);
    preferences.putString("mqtt_output", mqtt_output);
    preferences.putString("mqtt_input", mqtt_input);
    preferences.putString("mqtt_log", mqtt_log);
    preferences.putString("mqtt_user", mqtt_user);
    preferences.putString("mqtt_password", mqtt_password);
    preferences.putInt("publish_freq", publish_freq);
    preferences.putString("host", host);
    preferences.putInt("httpPort", httpPort);
    preferences.putString("mntpnt", mntpnt);
    preferences.putString("user", user);
    preferences.putString("passwd", passwd);
    preferences.putBool("mqtt_enabled", mqtt_enabled);
    preferences.end();
}

void resetPreferences() {
    preferences.begin("config", false); // read-write
    preferences.clear();
    preferences.end();
}
