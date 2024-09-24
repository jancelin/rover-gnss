/* ------------------------------------------------------
 *
 * @insipration:
 *     http://arduiniana.org/libraries/tinygpsplus/
 * 
 * @brief:  This program feeds TinyGPSPlus object with GNSS_SERIAL buffer data
 *          and dumps updated GNSS time on USB (Serial) port.
 *          
 * @board:
 *    ESP32 Feather
 * 
 * @GNSS module:
 *    Drotek DP0601 RTK GNSS (XL F9P)  
 *
 * @wiring:
 *      ESP32 RX        ->  DP0601 UART1 B3 (TX) 
 *      ESP32 Vin (5V)  ->  DP0601 UART1 B1 (5V)
 *      ESP32 GND       ->  DP0601 UART1 B6 (GND)
 *      
 * @ports:
 *      Serial (115200 baud)
 *      GNSS_SERIAL (configured baudrate for DP0601)
 *      
 * ------------------------------------------------------
 */
/* ############################
 * #    GLOBAL DEFINITIONS                                         #
 * ############################
 */
/* ###################
 * #    LIBRARIES    #
 * ###################
 */
#include <TinyGPSPlus.h>

/* #################
 * #    PROGRAM    #
 * #################
 */

// GNSS serial port
HardwareSerial Serialrx(1);
#define PIN_RX 16 
#define PIN_TX 17
// #define POWER_PIN 25 // used for F9P on Micro PCI Card

// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;
TinyGPSCustom gnssFixMode(gps, "GNGGA", 6);
 
void setup() {
  // pinMode(POWER_PIN, OUTPUT);
  // digitalWrite(POWER_PIN, HIGH);

  // Setup Serial ports for commmunication
  Serial.begin(115200);
  Serialrx.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(100);
}

void loop() {
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
  Serial.print(gps.location.lng(),8); //Degree precision versus length (https://en.wikipedia.org/wiki/Decimal_degrees)
  Serial.print(" - LAT = ");
  Serial.print(gps.location.lat(),8);
  Serial.print(" - COURSE = ");
  Serial.print(gps.course.value());
  Serial.print(" - SATELLITES = ");
  Serial.print(gps.satellites.value());
  Serial.print(" - FIX MODE = ");
  Serial.print(gnssFixMode.value());
  
  Serial.println();  
}
