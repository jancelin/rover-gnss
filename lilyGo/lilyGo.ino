#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <PubSubClient.h>

HardwareSerial MySerial(1);
#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25

// Data wire is plugged into port 14 on the ESP32 (GPIO14)
#define ONE_WIRE_BUS 0

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// variable to hold device addresses
DeviceAddress Thermometer;
int deviceCount = 0;
float temp = 0;  // variable for displaying the temperature

// MQTT Parameters
const char* ssid = "buoy";
const char* password = "12345678";
const char* mqtt_server = "192.168.36.197";
const int mqtt_port = 1883;
const char* mqtt_output = "lilygo/data";
const char* mqtt_input = "lilygo/input";
const char* mqtt_log = "lilygo/log";
const char* mqtt_user = "LilyGo";
const char* mqtt_password = "password";
const char mqtt_UUID[] = "LilyGo-RT";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int timeInterval = 1000;

// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;

IPAddress server(192, 168, 1, 100);  // IP address of the server
int port = 80;            

char* host = "caster.centipede.fr";
int httpPort = 2101; //port 2101 is default port of NTRIP caster
char* mntpnt = "LIENSS";
char* user = "rover-gnss-tester";
char* passwd = "";
NTRIPClient ntrip_c;

const char* udpAddress = "192.168.1.255";
const int udpPort = 9999;

int trans = 3;  //0 = serial, 1 = udp, 2 = tcp client, 3 = MySerial, 4 = myserial Choose which out you want use. for rs232 set 0 and connect tx f9p directly to rs232 module

WiFiUDP udp;

void setup() {
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);

    Serial.begin(115200);
    delay(100);
    MySerial.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    delay(100);

    sensors.begin();
    locateDevices();
    printDeviceAddresses();

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    connectToWiFi();
    requestSourceTable();
    requestMountPointRawData();
}

void loop() {
    long now = millis();
    if (now - lastMsg > timeInterval) {
        lastMsg = now;
        handleMQTTConnection();
        readGNSSData();
        displayGNSSData();
        readTemperatureSensors();
        publishMQTTData();
    }

    handleNTRIPData();
    handleSerialData();
    delay(500);
}

void locateDevices() {
    Serial.println("Locating devices...");
    Serial.print("Found ");
    deviceCount = sensors.getDeviceCount();
    Serial.print(deviceCount, DEC);
    Serial.println(" devices.");
    Serial.println("");
}

void printDeviceAddresses() {
    Serial.println("Printing addresses...");
    for (int i = 0; i < deviceCount; i++) {
        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(" : ");
        sensors.getAddress(Thermometer, i);
        printAddress(Thermometer);
    }
}

void connectToWiFi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void requestSourceTable() {
    Serial.println("Requesting SourceTable.");
    if (ntrip_c.reqSrcTbl(host, httpPort)) {
        char buffer[512];
        delay(5);
        while (ntrip_c.available()) {
            ntrip_c.readLine(buffer, sizeof(buffer));
        }
    } else {
        Serial.println("SourceTable request error");
    }
    Serial.print("Requesting SourceTable is OK\n");
    ntrip_c.stop();
}

void requestMountPointRawData() {
    Serial.println("Requesting MountPoint's Raw data");
    if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
        delay(15000);
        ESP.restart();
    }
    Serial.println("Requesting MountPoint is OK");
}

void handleMQTTConnection() {
    if (!client.connected()) {
        Serial.println("Reconnect to Mqtt");
        reconnect();
    }
    client.loop();
}

void readGNSSData() {
    while (MySerial.available()) {
        gps.encode(MySerial.read());
    }
}

void displayGNSSData() {
    Serial.print(gps.time.hour());
    Serial.print(':');
    Serial.print(gps.time.minute());
    Serial.print(':');
    Serial.print(gps.time.second());
    Serial.print('.');
    Serial.print(gps.time.centisecond());
    Serial.print(" - LONG = ");
    Serial.print(gps.location.lng());
    Serial.print(" - LAT = ");
    Serial.print(gps.location.lat());
    Serial.println();
}

void readTemperatureSensors() {
    sensors.requestTemperatures();
    for (int i = 0; i < deviceCount; i++) {
        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(" : ");
        temp = sensors.getTempCByIndex(i);
        Serial.print(temp);
        Serial.print(" °C");
        Serial.println("");
    }
    Serial.println("");
}

void publishMQTTData() {
    String json = "{\"user\":\"" + (String)mqtt_user + "\",\"Temperature_Water\":\"" + (String)sensors.getTempCByIndex(0) + "\",\"Lon\":\"" + (String)gps.location.lng() + "\",\"Lat\":\"" + (String)gps.location.lat() + "\"}";
    client.publish(mqtt_output, json.c_str());
    client.disconnect();

    Serial.println("Mqtt sent to : " + (String)mqtt_output);
    Serial.println(json);
}

void handleNTRIPData() {
    while (ntrip_c.available()) {
        char ch = ntrip_c.read();
        MySerial.print(ch);
    }
}

void handleSerialData() {
    WiFiClient client;
    while (MySerial.available()) {
        String s = MySerial.readStringUntil('\n');
        switch (trans) {
            case 0:  //serial out
                Serial.println(s);
                break;
            case 1:  //udp out
                udp.beginPacket(udpAddress, udpPort);
                udp.print(s);
                udp.endPacket();
                break;
            case 2:  //tcp client out
                if (!client.connect(server, port)) {
                    Serial.println("connection failed");
                    return;
                }
                client.println(s);
                while (client.connected()) {
                    while (client.available()) {
                        char c = client.read();
                        Serial.print(c);
                    }
                }
                client.stop();
                break;
            case 3:  //MySerial out
                MySerial.println(s);
                break;
            case 4:  //MySerial out
                MySerial.println(s);
                break;
            default:  //mauvaise config
                Serial.println("mauvais choix ou oubli de configuration");
                break;
        }
    }
}

void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        Serial.print("0x");
        if (deviceAddress[i] < 0x10) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
        if (i < 7) Serial.print(", ");
    }
    Serial.println("");
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    while (!client.connected()) {
        delay(100);
        Serial.print("Attempting MQTT connection...");
        if (client.connect(mqtt_UUID, mqtt_user, mqtt_password)) {
            Serial.println("connected");
            client.subscribe(mqtt_input);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(3000);
        }
    }
}

void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();
}
