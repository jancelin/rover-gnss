/* ----------------------------------------------
* @inspiration :
* 
*   DallasTemperature library :
*     Example Simple.ino
*
* @brief:
*    This program reads temperature on DS18B20 sensor
*    over OneWrie bus.
*
* @board :
*    Teensy 3.5 or ESP32
*
* @RS485 interface :
*    Mikroe RS485 click 2 (MAX3471)
*
* @wiring :
*    Board pin -> DS18B20 Yellow
*    Board 3V3 -> DS18B20 Orange
*    Board GND -> DS18B20 White
*
* @baudrates :
*    Serial (USB Debug) : 115200
*    Serial4 (Modbus communication) : 9600 (Baudrate configured in sensor)
* ---------------------------------------------------    
*/

// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin on which is plugged DS18B20 data wire (yellow)
#define ONE_WIRE_BUS 24

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{
  // start serial port
  Serial.begin(115200);
  //Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
}

/*
 * Main function, get and show the temperature
 */
void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  //Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
  } 
  else
  {
    Serial.println("Error: Could not read temperature data");
  }
}
