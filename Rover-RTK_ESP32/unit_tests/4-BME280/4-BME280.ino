/* ********************************************************************** 
* Description:
* This sample code is mainly used to monitor sensorHub and sending through Mqtt Json data
*
* Software Environment: Arduino IDE 1.8.9
* Software download link: https://www.arduino.cc/en/Main/Software
*
* Install the library file：
* Copy the files from the github repository folder libraries to the libraries
* in the Arduino IDE 1.8.9 installation directory
*
* Hardware platform   : ESP32
*
* author  :  Rominco(rtourte@yahoo.fr)
* version :  V1.0
* date    :  2021-03-22
********************************************************************** */

#include "SparkFunBME280.h"

BME280 sensorBME280;

const int ledPin = 4;
int timeInterval = 7500;

long lastMsg = 0;
char msg[50];
int value = 0;

/* ------------------------
 *  MAIN SETUP 
 *  -----------------------
 */
void setup()
{
    Serial.begin(115200);
    delay(10);

    /* ********************* */
    /*  BME280 SENSOR        */ 
    /* ********************* */
    Serial.println("BME280 begin");
    // I2C device found at address 0x76
    Wire.begin();
    if (sensorBME280.beginI2C() == false) //Begin communication over I2C
    {
      Serial.println("The sensor did not respond. Please check wiring.");
      while(1); //Freeze
    }
    else
    {
      Serial.println("BME280 started & configured");
    }
}


/* 
 *  ------------------
 *  MAIN LOOP
 *  ------------------
 */
void loop() {
  
  long now = millis();
  if (now - lastMsg > timeInterval ) {
    lastMsg = now;

    Serial.print(" Air Temperature : ");
    Serial.println(sensorBME280.readTempC(), 2);

    Serial.print(" Humidity: ");
    Serial.println(sensorBME280.readFloatHumidity(), 0);
    
    Serial.print(" Pressure: ");
    Serial.println(sensorBME280.readFloatPressure(), 0);
    
    Serial.print(" Altitude : ");
    Serial.println(sensorBME280.readFloatAltitudeMeters(), 1);
    Serial.println();

    delay(500);
  }
}
