/***************
 * Find all DS18b20 addresses connected on the bus. 
 * Get each temperature and display them on serial monitor
 ***************/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Secret.h"

// Data wire is plugged into port 14 on the ESP32 (GPIO14)
#define ONE_WIRE_BUS 0

#define PIN_RX 16
#define PIN_TX 17 
// #define POWER_PIN 25
// GNSS serial port
HardwareSerial Serialrx(1);
// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// variable to hold device addresses
DeviceAddress Thermometer;
int deviceCount = 0;
float temp =0;  // variable pour l'affichage de la température

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
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int timeInterval = 1000;

void setup(void)
{
  // start serial port
  Serial.begin(115200);
  delay(100);
  Serialrx.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(100);

  // pinMode(POWER_PIN, OUTPUT);
  // digitalWrite(POWER_PIN, HIGH);

  // Start up the DS18b20 communication protocole
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

  /* Setup Wifi & MQTT */
  setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop(void)
{
  long now = millis();
  if (now - lastMsg > timeInterval ) {
    lastMsg = now;
    if (!client.connected()) {
      Serial.println("Reconnect to Mqtt");
      reconnect();
    }
    client.loop();

    // While GNSS_SERIAL buffer is not empty
    while (Serialrx.available())
      // Read buffer
      gps.encode(Serialrx.read());

    // Dump GNSS time (UTC)
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

    //String json = "{\"user\":\""+(String)mqtt_user+"\",\"Humidity\":\""+(String)sensorBME280.readFloatHumidity()+"\",\"Pressure\":\""+(String)sensorBME280.readFloatPressure()+"\",\"Altitude\":\""+(String)sensorBME280.readFloatAltitudeMeters()+"\",\"AirTemperature\":\""+(String)sensorBME280.readTempC()+"\",\"Temperature_OvenInto_Water\":\""+(String)sensorDS18B20.getTempCByIndex(1)+"\",\"Temperature_OvenIn\":\""+(String)sensorDS18B20.getTempCByIndex(0)+"\"}";
    String json = "{\"user\":\""+(String)mqtt_user+"\",\"Temperature_Water\":\""+(String)sensors.getTempCByIndex(0)+"\",\"Lon\":\""+(String)gps.location.lng()+"\",\"Lat\":\""+(String)gps.location.lat()+"\"}";
    client.publish(mqtt_output, json.c_str() );
    client.disconnect();

    Serial.println("Mqtt sent to : " + (String)mqtt_output );
    Serial.println(json);
  }


  delay(1000);
  
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