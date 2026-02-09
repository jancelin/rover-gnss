/* 
 *****************************
 *   DS18B20 SENSOR MODULE   *
 *****************************
 * @brief:
 *    This module is loaded to handle DS18B20 sensor setup and 
 *    temperature acquisition.
 */
/*
 *****************
 *   LIBRARIES   *
 *****************
 */
#include <OneWire.h>
#include <DallasTemperature.h>

/*
 **************************
 *   GLOBAL DEFINITIONS   *
 **************************
 */
// Temperature value read if sensor is disconnected or out of range 
#define TEMP_NO_VALUE DEVICE_DISCONNECTED_C

/*
 *********************
 *   SENSOR CONFIG   *
 *********************
 */
// Sensor ID
#define DS18B20_ID    0
// Sensor resolution
#define DS18B20_RES   11/*bits*/
// Teensy temperature data wire pin
#define TEMPERATURE_PIN  24

/*
 ***********************
 *   GLOBAL VARIBLES   *
 ***********************
 */
// OneWire bus
OneWire oneWire(TEMPERATURE_PIN);
// Dallas sensor bus
DallasTemperature sensors(&oneWire);
// DS18B20 address
byte ds18b20_addr[8];

/*
 ***************************
 *   FUNCTION PROTOTYPES   *
 ***************************
 */
void setupTempSensor(volatile bool& deviceConnected);
float readTemperature(volatile bool& deviceConnected);
/*
 ****************************
 *   FUNCTION DEFINITIONS   *
 ****************************
 */
/*
 * @brief: 
 *    Sets up the DS18B20 temperature sensor.
 * @params:
 *    deviceConnected: bool to store if DS18B20 is connected or not.
 */
void setupTempSensor(volatile bool& deviceConnected) {

  sensors.begin();
  // Setting resolution for temperature (the lower, the quicker the sensor responds)
  sensors.setResolution(DS18B20_RES);
  // Store DS18B20 OneWire adress for qicker data acquitsition
  sensors.getAddress(ds18b20_addr, DS18B20_ID);
  // Check for sensor presence
  if (sensors.getTempC(ds18b20_addr) == DEVICE_DISCONNECTED_C)  {
    waitForReboot("No DS18B20 connected...");
  }
  SERIAL_DBG("DS18B20 found!\n")
    
  deviceConnected = true;
  SERIAL_DBG("Done.\n")
}

/*
 * @brief: 
 *    returns temperature read in DS18B20 sensor.
 * @note:
 *    temperature digital convertion is long an depends on sensor resolution.
 * @params:
 *    deviceConnected: bool to store if DS18B20 is connected or not.
 * @retrun:
 *    temp_C: read temperature.
 */
float readTemperature(volatile bool& deviceConnected)	{
	
  // Read DS18B20 temperature
  sensors.requestTemperatures();
  float temp_C = sensors.getTempC(ds18b20_addr);
  // Check for OneWire errors
  if (temp_C == DEVICE_DISCONNECTED_C) {
    SERIAL_DBG("DS18B20 disconnected...")
    deviceConnected = false;
  }
  else
    deviceConnected = true;

	return temp_C;
}