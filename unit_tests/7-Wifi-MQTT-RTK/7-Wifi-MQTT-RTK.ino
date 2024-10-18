#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <PubSubClient.h>
#include "Secret.h"

HardwareSerial MySerial(1);
#define PIN_RX 16
#define PIN_TX 17 
#define MYSERIAL_BAUD_RATE 460800 // 115200 Works with F9P config on 2Hz ( not in 5Hz ! )

// #define POWER_PIN 25

// Data wire is plugged into port 14 on the ESP32 (GPIO14)
#define ONE_WIRE_BUS 0

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// variable to hold device addresses
DeviceAddress Thermometer;
int deviceCount = 0;
float temp =0;  // variable for displaying the temperature

// WIFI Parameters
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// MQTT Parameters
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_output = MQTT_OUTPUT;
const char* mqtt_input = MQTT_INPUT;
const char* mqtt_log = MQTT_LOG;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char mqtt_UUID[] = MQTT_UUID;

WiFiClient espClient;
PubSubClient client_mqtt(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int timeInterval = 3000;
const long readDuration = 1000;  // Duration of NMEA reading in milliseconds
unsigned long previousMillis = 0; // timer
unsigned long currentMillis = 0;  // timer

// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;
TinyGPSCustom gnssFixMode(gps, "GNGGA", 6);

IPAddress server(192, 168, 1, 100);  // IP address of the server
int port = 80;            

char* host = NTRIP_SERVER_HOST;
int httpPort = 2101; // port 2101 is default port of NTRIP caster
char* mntpnt = NTRIP_CASTER_MOUNTPOINT;
char* user   = NTRIP_USER;
char* passwd = NTRIP_PASSWORD;
bool sendGGA = true;
NTRIPClient ntrip_c;
String nmeaMessage = "";
String ggaMessage = "";

const char* udpAddress = "192.168.1.255";
const int udpPort = 9999;

// Connection to use to transmit  data
// For serial rs232 set 0 and connect tx f9p directly to rs232 module
// For UDP server use 1 and use
// For TCP MQTT server using client_mqt use 2  = tcp , 
// For Myserial use 3 = MySerial
int transmition_mode = 3; 

WiFiUDP udp;

void setup() {
    // POWER_PIN : This pin controls the power supply of the MICRO PCI card
    // pinMode(POWER_PIN, OUTPUT);
    // digitalWrite(POWER_PIN, HIGH);

    // Start serial ports
    Serial.begin(115200);
    delay(100);
    MySerial.begin(MYSERIAL_BAUD_RATE, SERIAL_8N1, PIN_RX, PIN_TX); // 115200 Works with 
    delay(100);

    // Start up the DS18b20 communication protocol
    sensors.begin();

    // Locate devices on the bus
    Serial.println("Locating devices...");
    Serial.print("Found ");
    deviceCount = sensors.getDeviceCount();   // counting number of devices on the bus
    Serial.print(deviceCount, DEC);
    Serial.println(" devices.");
    Serial.println("");
    
    Serial.println("Printing addresses...");
    for (int i = 0;  i < deviceCount;  i++) {
        Serial.print("Sensor ");
        Serial.print(i+1);
        Serial.print(" : ");
        sensors.getAddress(Thermometer, i);   // get each DS18b20 64bits address
        printAddress(Thermometer);            // function at the end of this code
    }

    // Setup Wifi & MQTT
    setup_wifi();
    client_mqtt.setServer(mqtt_server, mqtt_port);
    client_mqtt.setCallback(callback);

    Serial.println("Requesting SourceTable.");
    if(ntrip_c.reqSrcTbl(host,httpPort)){
        char buffer[512];
        delay(5);
        while(ntrip_c.available()){
            ntrip_c.readLine(buffer,sizeof(buffer));
            //Serial.print(buffer); //print sourcetable
        }
        Serial.print("Requesting SourceTable is OK\n");

    } else {
        Serial.println("SourceTable request error");
    }
    ntrip_c.stop(); // Need to call "stop" function for next request.
    
    Serial.println("Requesting MountPoint's Raw data");
    if(!ntrip_c.reqRaw(host,httpPort,mntpnt,user,passwd)){
        delay(15000);
        ESP.restart();
    }
    Serial.println("Requesting MountPoint is OK");
}

void loop() {
    
  if (sendGGA) {
      currentMillis = millis();
        if (currentMillis - previousMillis >= timeInterval) {
            previousMillis = currentMillis;

            unsigned long readStartMillis = millis();
            bool ggaFound = false;
            while (millis() - readStartMillis < readDuration && !ggaFound) {
                while (MySerial.available()) {
                    char c = MySerial.read();
                    gps.encode(c);
                    if (c == '\n' || c == '\r') {
                        if (nmeaMessage.startsWith("$GNGGA") || nmeaMessage.startsWith("$GPGGA")) {
                            // Validation du format GGA
                            int numFields = 0;
                            for (char ch : nmeaMessage) {
                                if (ch == ',') numFields++;
                            }
                            if (numFields == 14) { // 14 virgules attendues dans un message GGA complet
                                ntrip_c.setLastGGA(nmeaMessage);                  // Stocker le dernier message GGA reçu
                                Serial.println("Extracted GGA: " + nmeaMessage);  // Log du message GGA extrait
                                ggaFound = true;                                  // Mettre à jour le drapeau pour arrêter la lecture
                                break;                                            // Sortir de la boucle intérieure
                            }
                        }
                        nmeaMessage = "";
                    } else {
                        nmeaMessage += c;
                    }
                }
            }

            // Send the last GGA message stored
            String lastGGA = ntrip_c.getLastGGA();
            if (lastGGA != "") {
                ntrip_c.sendGGA(lastGGA.c_str(), host, httpPort, user, passwd, mntpnt);
                Serial.println("Sent GGA: " + lastGGA);  // Log sent GGA message
                lastGGA = "";
                //Serial.println("Cleaned GGA");
            } else {
                Serial.println("No GGA message to send.");
            }
        }
    }

    // NTRIP Data Handling
    while (ntrip_c.available()) {
        char ch = ntrip_c.read();
        MySerial.print(ch);
    }

    long now = millis();
    if (now - lastMsg > timeInterval ) {
        lastMsg = now;
        if (!client_mqtt.connected()) {
            Serial.println("Reconnect to Mqtt");
            reconnect();
        }
        client_mqtt.loop();

        // // While GNSS_SERIAL buffer is not empty
        // while (MySerial.available())
        //     // Read buffer
        //     gps.encode(MySerial.read());

        // Dump GNSS time (UTC)
        Serial.print(gps.time.hour());
        Serial.print(':');
        Serial.print(gps.time.minute());
        Serial.print(':');
        Serial.print(gps.time.second());
        Serial.print('.');
        Serial.print(gps.time.centisecond());
        Serial.print(" - LONG = ");
        Serial.print(gps.location.lng(),9);
        Serial.print(" - LAT = ");
        Serial.print(gps.location.lat(),9);
        Serial.print(" - COURSE = ");
        Serial.print(gps.course.value());
        Serial.print(" - SATELLITES = ");
        Serial.print(gps.satellites.value());
        Serial.print(" - FIX MODE = ");
        Serial.print(gnssFixMode.value());
        Serial.println();  

        sensors.requestTemperatures();   // request temperature conversion for all sensors
        for (int i = 0;  i < deviceCount;  i++) {
            Serial.print("Sensor ");
            Serial.print(i+1);
            Serial.print(" : ");
            temp = sensors.getTempCByIndex(i);  //  Get temperature(Celsius) for sensor#i
            Serial.print(temp);
            Serial.print(" °C");
            Serial.println("");
        }
        Serial.println("");

        String json = "{\"user\":\""+(String)mqtt_user+"\",";
        json += "\"Temperature_Water\":\""+(String)sensors.getTempCByIndex(0)+"\",";
        json += "\"Lon\":\""+String(gps.location.lng(), 9)+"\",";
        json += "\"Lat\":\""+String(gps.location.lat(), 9)+"\",";
        json += "\"FIXE\":\""+String(gnssFixMode.value())+"\"}";
        client_mqtt.publish(mqtt_output, json.c_str() );
        client_mqtt.disconnect();

        Serial.println("Mqtt sent to : " + (String)mqtt_output );
        Serial.println(json);
    }

    WiFiClient client_wifi;  // Declare the WiFiClient outside the switch

    while (MySerial.available()) {
        String s = MySerial.readStringUntil('\n');
        switch (transmition_mode) {
            case 0:  //serial out
                Serial.println(s);
                break;
            case 1:  //udp out
                udp.beginPacket(udpAddress, udpPort);
                udp.print(s);
                udp.endPacket();
                break;
            case 2:  //tcp client_wifi out
                if (!client_wifi.connect(server, port)) {
                    Serial.println("connection failed");
                    return;
                }
                client_wifi.println(s);
                while (client_wifi.connected()) {
                    while (client_wifi.available()) {
                        char c = client_wifi.read();
                        Serial.print(c);
                    }
                }
                client_wifi.stop();
                break;
            case 3:  //MySerial out
                MySerial.println(s);
                break;
            default:  //mauvaise config
                Serial.println("mauvais choix ou oubli de configuration");
                break;
        }
    }

    delay(1000); // to reduce busy-looping
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
    // We start by connecting to a WiFi network
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
    // Loop until we're reconnected
    while (!client_mqtt.connected()) {
        delay(100);
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client_mqtt.connect(mqtt_UUID , mqtt_user, mqtt_password )) {
            Serial.println("connected");
            // Subscribe
            client_mqtt.subscribe(mqtt_input);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client_mqtt.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
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
