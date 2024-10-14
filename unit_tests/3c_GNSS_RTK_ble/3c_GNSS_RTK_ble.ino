/*
  README
  This is a modified version of the original code `rover-gnss/unit_tests/3c_GNSS_RTK` where I modified it to send the data through the BLE from the esp32 board.
  Why?
  The original code is the classic bluetooth profile ( SPP (Bluetooth Serial) ) which is supported only by Android, iOS does not support it.
  See https://stackoverflow.com/questions/76604020/is-there-a-way-to-make-esp32-discoverable-as-a-bluetooth-device-on-a-flutter-ios

  References:
  1. https://randomnerdtutorials.com/esp32-ble-server-client/
  2. https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/
  3. Increase size of BLE messages --> https://www.esp32.com/viewtopic.php?t=4546 , by default is 20mb see this post

  After flashing, I used an Android phone to use the app LightBlue to read the raw data sent by the esp32 board.

  Now, I searched for an app supporting reading GNSS data over BLE. I found GNSS Master.
  PS: indeed, in the original version of the code, we were using Bluetooth gnss for reading the gnss data from the esp32 board. However, this app supports only bluetooth serial!

  So switch to GNSS Master.
  New problem: GNSS Master supports BLE but only the profile Serial! I need to modify the code, to change the server profile UID.
  I found a library (https://github.com/avinabmalla/ESP32_BleSerial/tree/master) that provides a class BLESerial, which works in the same way as the library BluetoothSerial used in the original code.
  Interestingly, in the implementation of BLESerial, the developer does the same stuff as me for the BLE server and etc., but he is using a different UID for the server.
  Therefore, I moved to a new project directory named 'ble_rover' in '$HOME/Arduino/'
*/
/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <BluetoothSerial.h>
#include "Secret.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

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

//BluetoothSerial SerialBT;

#define bleServerName "GC_ESP32"
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

//1. BLECharacteristic bmeGNSSDataCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
//1. BLEDescriptor bmeGNSSDataDescriptor(BLEUUID((uint16_t)0x2902));

bool deviceConnected = false;
//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" //2.

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    delay(500);
    MySerial.begin(MYSERIAL_BAUD_RATE, SERIAL_8N1, PIN_RX, PIN_TX); // serial port to send RTCM to F9P
    delay(100);
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    #if 0
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    #endif

    /* === START -- SERVER BLE === */
    // Initialiser le Bluetooth
    /*
    switch (trans) {
      case 4:
        if (!SerialBT.begin(BT_NAME)) {
         Serial.println("An error occurred initializing Bluetooth");
        } else {
          Serial.println("Bluetooth initialized with name " + (String)BT_NAME);
        }
        break;
    }
    */

    // Create the BLE Device
    BLEDevice::init(bleServerName);
    BLEDevice::setMTU(512); //https://www.esp32.com/viewtopic.php?t=4546

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    //1. BLEService *bmeService = pServer->createService(SERVICE_UUID);
    
    pService = pServer->createService(SERVICE_UUID); //2.

    //1. bmeService->addCharacteristic(&bmeGNSSDataCharacteristics);
    //1. bmeGNSSDataDescriptor.setValue("GNSS Data");
    //1. bmeGNSSDataCharacteristics.addDescriptor(&bmeGNSSDataDescriptor);

    //2.
    pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

    pCharacteristic->setValue("Hello World!");

    // Start the service
    //1. bmeService->start();
    pService->start(); //2.

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);

    //2.
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    //pServer->getAdvertising()->start();
    
    Serial.println("Waiting a client connection to notify...");
    /* === END -- SERVER BLE === */


    #if 0
    Serial.println("Requesting SourceTable.");
    if (ntrip_c.reqSrcTbl(host, httpPort)) {
        char buffer[512];
        delay(5);
        while (ntrip_c.available()) {
            ntrip_c.readLine(buffer, sizeof(buffer));
            //Serial.print(buffer);
        }
        Serial.print("Requesting SourceTable is OK\n");

    } else {
        Serial.println("SourceTable request error");
    }
    ntrip_c.stop(); // Need to call "stop" function for next request.

    Serial.println("Requesting MountPoint's Raw data");
    if (!ntrip_c.reqRaw(host, httpPort, mntpnt, user, passwd)) {
        delay(15000);
        ESP.restart();
    }
    Serial.println("Requesting MountPoint is OK");
    #endif
}

void loop() {

  #if 0
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
  #endif


    while (MySerial.available()) {
        String s = MySerial.readStringUntil('\n');
        switch (trans) {
            case 0:  // serial out
                Serial.println(s);
                break;
            case 1:  // udp out
                #if 0
                udp.beginPacket(udpAddress, udpPort);
                udp.print(s);
                udp.endPacket();
                #endif 0
                break;
            case 2:  // tcp client out
                #if 0
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
                #endif 0
                break;
            case 3:  // MySerial out
                MySerial.println(s);
                break;
            case 4: //BT
                //SerialBT.println(s);
                if (!deviceConnected) {
                  Serial.println("waiting for BLE device");
                } else {
                  //1. uint16_t gnss_data = 34567;
                  //1. bmeGNSSDataCharacteristics.setValue(gnss_data);
                  pCharacteristic->setValue(s);
                  pCharacteristic->notify();
                  //1. bmeGNSSDataCharacteristics.notify();
                  Serial.print("Publishing gnss data | length: ");
                  Serial.print(s.length());
                  Serial.print(", content: ");
                  Serial.println(s);
                }
                // Serial.println(s); // DEbug mode, permet de voir toutes les trames envoyées par le F9P
                break;
            default:  // mauvaise config
                Serial.println("mauvais choix ou oubli de configuration");
                break;
        }
    }
}
