/***************
 * Find all DS18b20 addresses connected on the bus. 
 * Get each temperature and display them on serial monitor
 ***************/
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 14 on the ESP32 (GPIO14)
#define ONE_WIRE_BUS 14

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// variable to hold device addresses
DeviceAddress Thermometer;

int deviceCount = 0;

float temp =0;  // variable pour l'affichage de la température

void setup(void)
{
  // start serial port
  Serial.begin(115200);

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
}

void loop(void)
{
     sensors.requestTemperatures();   // request temperature conversion for all sensors
     for (int i = 0;  i < deviceCount;  i++)
    {
      Serial.print("Sensor ");
      Serial.print(i+1);
      Serial.print(" : ");
      temp = sensors.getTempCByIndex(i);  //  Get temperature(Celsius) for sensor#i
      Serial.print(temp);
      Serial.print(" °C");
      Serial.println("");
    }
  Serial.println("");
  
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
