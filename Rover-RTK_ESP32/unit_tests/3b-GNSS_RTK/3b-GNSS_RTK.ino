/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <BluetoothSerial.h>
#include "Secret.h"

// GNSS serial port
HardwareSerial MySerial(1);
#define PIN_RX 16
#define PIN_TX 17
#define MYSERIAL_BAUD_RATE 460800 // 115200 Works with F9P config on 2Hz ( not in 5Hz ! )

// BLUETOOTH Name
#define BT_NAME "ESP32_BT_GNSS_RTK"

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;
IPAddress server(192, 168, 1, 100);  // IP address of the server
int port = 80;

char* host = NTRIP_SERVER_HOST;
int httpPort = 2101; // port 2101 is default port of NTRIP caster
char* mntpnt = NTRIP_CASTER_MOUNTPOINT;
char* user   = NTRIP_USER;
char* passwd = NTRIP_PASSWORD;
bool sendGGA = true;
NTRIPClient ntrip_c;

const char* udpAddress = "192.168.0.13";
const int udpPort = 9999;

int trans = 4; 
// 0 = serial, 1 = udp, 2 = tcp client, 3 = MySerial, 4 = Bluetooth Choose which output you want to use. for RS232 set 0 and connect tx F9P directly to RS232 module

WiFiUDP udp;

// send GGA 
NTRIPClient ntripClient;
String nmeaMessage = "";
String ggaMessage = "";

unsigned long previousMillis = 0; // timer
unsigned long currentMillis = 0;  // timer

const long interval = 10000;     // Duration between 2 GGA Sending
const long readDuration = 1000;  // Duration of NMEA reading in milliseconds

BluetoothSerial SerialBT;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    delay(500);
    MySerial.begin(MYSERIAL_BAUD_RATE, SERIAL_8N1, PIN_RX, PIN_TX); // serial port to send RTCM to F9P
    delay(100);
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

    // Initialiser le Bluetooth
    switch (trans) {
      case 4:
        if (!SerialBT.begin(BT_NAME)) {
         Serial.println("An error occurred initializing Bluetooth");
        } else {
          Serial.println("Bluetooth initialized with name " + (String)BT_NAME);
        }
        break;
    }

//    Serial.println("Requesting SourceTable.");
//    if (ntrip_c.reqSrcTbl(host, httpPort)) {
//        char buffer[512];
//        delay(5);
//        while (ntrip_c.available()) {
//            ntrip_c.readLine(buffer, sizeof(buffer));
//            //Serial.print(buffer);
//        }
//        Serial.print("Requesting SourceTable is OK\n");

//    } else {
//        Serial.println("SourceTable request error");
//    }
//    ntrip_c.stop(); // Need to call "stop" function for next request.

    Serial.println("Requesting MountPoint's Raw data");
    if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
        delay(15000);
        ESP.restart();
    }
    Serial.println("Requesting MountPoint is OK");
}

void loop() {

  WiFiClient client;
  if (sendGGA) {
      currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
          previousMillis = currentMillis;

          unsigned long readStartMillis = millis();
          bool ggaFound = false;
          while (millis() - readStartMillis < readDuration && !ggaFound) {
              while (MySerial.available()) {
                  char c = MySerial.read();
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

    while (ntrip_c.available()) {
        char ch = ntrip_c.read();
        MySerial.print(ch);
    }

    while (MySerial.available()) {
        String s = MySerial.readStringUntil('\n');
        switch (trans) {
            case 0:  // serial out
                Serial.println(s);
                break;
            case 1:  // udp out
                udp.beginPacket(udpAddress, udpPort);
                udp.print(s);
                udp.endPacket();
                break;
            case 2:  // tcp client out
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
            case 3:  // MySerial out
                MySerial.println(s);
                break;
            case 4: //BT
                SerialBT.println(s);
                // Serial.println(s); // DEbug mode, permet de voir toutes les trames envoyées par le F9P
                break;
            default:  // mauvaise config
                Serial.println("mauvais choix ou oubli de configuration");
                break;
        }
    }
}
