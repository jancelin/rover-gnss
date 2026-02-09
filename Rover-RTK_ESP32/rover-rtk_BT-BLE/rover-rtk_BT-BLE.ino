#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UART2 (GNSS) pins & baud
#define PIN_RX          16
#define PIN_TX          17
#define BAUD_RECEIVER   460800

// Nordic UART Service (NUS) UUIDs
static BLEUUID  NUS_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID  NUS_CHAR_RX_UUID ("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // Write from client → ESP32
static BLEUUID  NUS_CHAR_TX_UUID ("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // Notify    ESP32 → client

HardwareSerial &GNSS = Serial2;        // alias pour UART2
BLECharacteristic *pTxCharacteristic;  // TX caractéristique (notify → SW Maps)
bool deviceConnected = false;

// Callback BLE pour gestion connexion
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override   { deviceConnected = true; }
  void onDisconnect(BLEServer*) override{ deviceConnected = false; }
};

// Callback BLE pour réception (Write) — optionnel
class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override {
    // On récupère un String Arduino
    String val = pChar->getValue();
    if(val.length()) {
      // On renvoie sur UART2 si nécessaire
      GNSS.write((const uint8_t*)val.c_str(), val.length());
    }
  }
};

void setup() {
  // 1) Debug USB
  Serial.begin(115200);
  delay(100);
  Serial.println("=== BLE-Bridge NMEA démarrage ===");

  // 2) Initialisation UART2 pour GNSS
  GNSS.begin(BAUD_RECEIVER, SERIAL_8N1, PIN_RX, PIN_TX);
  Serial.printf("UART2 initialisé à %u bauds\n", BAUD_RECEIVER);

  // 3) Init BLE
  BLEDevice::init("ESP32_NMEA_Bridge");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // 4) Création du service Nordic UART
  BLEService *pService = pServer->createService(NUS_SERVICE_UUID);

  // 5) Caractéristique TX (notify → SW Maps)
  pTxCharacteristic = pService->createCharacteristic(
    NUS_CHAR_TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  // Indispensable pour que iOS autorise les notifications
  pTxCharacteristic->addDescriptor(new BLE2902());

  // 6) Caractéristique RX (write ← client)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    NUS_CHAR_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  // 7) Lancement du service
  pService->start();

  // 8) Advertising BLE
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // optimise pour iOS
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();

  Serial.println("Advertising NUS démarré, prêt pour SW Maps");
}

void loop() {
  static String txBuf = "";

  if (deviceConnected) {
    // 9) Relais UART2 → BLE (ligne par ligne)
    while (GNSS.available()) {
      char c = GNSS.read();
      txBuf += c;
      if (c == '\n') {
        pTxCharacteristic->setValue(txBuf);
        pTxCharacteristic->notify();
        txBuf = "";
      }
    }
  } else {
    // 10) Si déconnecté, vider le tampon
    txBuf = "";
  }

  delay(1);
}
