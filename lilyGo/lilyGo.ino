/* ------------------------------------------------------
 *
 * @inspiration:
 *     http://arduiniana.org/libraries/tinygpsplus/
 * 
 * @brief:  This program feeds TinyGPSPlus object with GNSS_SERIAL buffer data
 *          and dumps updated GNSS time on MQTT over Wifi connection.
 *          
 * @board:
 *    lilyGo 3.5
 * 
 * @GNSS module:
 *    Drotek DP0601 RTK GNSS (XL F9P)  
 *
 * @wiring:
 *      lilyGo RX2      ->  DP0601 UART1 B3 (TX) 
 *      lilyGo Vin (5V) ->  DP0601 UART1 B1 (5V)
 *      lilyGo GND      ->  DP0601 UART1 B6 (GND)
 *      
 * @ports:
 *      Serial (115200 baud)
 *      GNSS_SERIAL (configured baudrate for DP0601)
 *      
 * ------------------------------------------------------
 */
/* ############################
 * #    GLOBAL DEFINITIONS    #
 * ############################
 */
/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>

/* #################
 * #    PROGRAM    #
 * #################
 */
#define ONE_WIRE_BUS 0
#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25

// GNSS serial port
HardwareSerial Serialrx(1); // Receiver GNSS
HardwareSerial MySerial(1); // Sending RTCM to GNSS

// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;
 

// NTRIP 
char* host = "caster.centipede.fr";
int httpPort = 2101; //port 2101 is default port of NTRIP caster
char* mntpnt = "CT";
char* user   = "rover-gnss-tester-RT";
char* passwd = "";
NTRIPClient ntrip_c;

// TEMPERATURE SENSOR
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer;
int deviceCount = 0;
float temp =0;  // variable pour l'affichage de la température

// WIFI Parameters
const char* ssid     = "WifiRaspi";
const char* password = "wifiraspi";

// MQTT
// const char* mqtt_server = "mavi-mqtt.centipede.fr";
// const int mqtt_port = 8090;
const char* mqtt_server = "172.24.1.1";
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
long lastMsg_NTRIP = 0;
char msg[50];
int value = 0;
int timeInterval = 5000;
int timeInterval_NTRIP = 2000;

/***************************/
/*         SETUP           */
/***************************/
void setup() {
  // Power Up for MPCI card.
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  // Setup Serial ports for commmunication
  Serial.begin(115200);
  Serialrx.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(100);
  MySerial.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(100);

  /******* DS18B20 TEMPERATURE ********/
  setupTemperature();

   /* Setup Wifi & MQTT */
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  /* SETUP NTRIP CLIENT */
  setup_ntrip();
}

void loop() {

  long now = millis();
  sendRTCM();

  if (now - lastMsg > timeInterval ) {
    lastMsg = now;
    receiveGNSS();
    if (!client.connected()) {
      Serial.println("Reconnect to Mqtt");
      reconnect();
    }
    client.loop();
    // Print temperature
    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);  //  Get temperature(Celsius) for sensor#i
    Serial.print(temp);
    Serial.print(" °C");
    Serial.println("");

    // Sending to MQTT
    String json = "{\"user\":\""+(String)mqtt_user+"\",\"Temperature_Water\":\""+(String)sensors.getTempCByIndex(0)+"\",\"Lon\":\""+(String)gps.location.lng()+"\",\"Lat\":\""+(String)gps.location.lat()+"\"}";
    client.publish(mqtt_output, json.c_str() );
    client.disconnect();

    Serial.println("Mqtt sent to : " + (String)mqtt_output );
    Serial.println(json);
  }
}

/****************** */
/*    SETUP NTRIP   */
/****************** */
void setup_ntrip(){
  Serial.println("SETUP - NTRIP Connection BEGIN");
  Serial.println("Requesting SourceTable.");
  if(ntrip_c.reqSrcTbl(host,httpPort)){
    char buffer[512];
    delay(500);
    while(ntrip_c.available()){
      ntrip_c.readLine(buffer,sizeof(buffer));
      Serial.print(buffer); 
    }
  }
  else{
    Serial.println("SourceTable request error");
  }
  Serial.print("Requesting SourceTable is OK\n");
  ntrip_c.stop(); //Need to call "stop" function for next request.
  
  Serial.println("Requesting MountPoint's Raw data");
  if(!ntrip_c.reqRaw(host,httpPort,mntpnt,user,passwd)){
    delay(15000);
    ESP.restart();
  }
  Serial.println("Requesting MountPoint is OK");
  Serial.println("SETUP - NTRIP Connection DONE");

}

void sendRTCM(){
  long now = millis();
  // Serial.println("ntrip receiver...");
  while(ntrip_c.available() && (now - lastMsg_NTRIP <= timeInterval_NTRIP ) )
    {
        char ch = ntrip_c.read();        
        MySerial.print(ch);
        
    }
    if (now - lastMsg_NTRIP > timeInterval_NTRIP ) {
      lastMsg_NTRIP = now;
      Serial.println("delay off for NTRIP ntrip_c");
    }

  // Serial.println("sendint Rtcp...");
  now = millis();
  while (MySerial.available() && (now - lastMsg_NTRIP <= timeInterval_NTRIP ) )
    {
      now = millis();
      String s = MySerial.readStringUntil('\n');
      MySerial.println(s);
    }
    if (now - lastMsg_NTRIP > timeInterval_NTRIP ) {
      lastMsg_NTRIP = now;
      Serial.println("delay off for NTRIP MySeral available...");
    }
}

void receiveGNSS(){
  // While GNSS_SERIAL buffer is not empty
  while (Serialrx.available())
    // Read buffer
    gps.encode(Serialrx.read());

  // // Dump GNSS time (UTC)
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
/* ----------------------
 *  WIFI SETUP 
 *  ---------------------
 */
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

/* ----------------------
 *  TEMPERATURE SETUP 
 *  ---------------------
 */
void setupTemperature(){
  sensors.begin();

  // locate devices on the bus
  Serial.println("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();   // counting number of devices on the bus
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
  
  Serial.println("Printing addresses...");
  for (int i = 0;  i < deviceCount;  i++)
  {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" : ");
    sensors.getAddress(Thermometer, i);   // get each DS18b20 64bits address
    printAddress(Thermometer);            // function at the end of this code
    //Serial.print(Thermometer);
  }
}

void printAddress(DeviceAddress deviceAddress)
{ 
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
}

/* ------------------------
 *  MQTT CALLBACK
 *  -----------------------
 */
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

/* 
 *  ------------------
 *  RECONNECT MQTT
 *  ------------------
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    delay(100);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_UUID , mqtt_user, mqtt_password )) {
    //if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(mqtt_input);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
    }
  }
}