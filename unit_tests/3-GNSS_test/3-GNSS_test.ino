/* ------------------------------------------------------
 *
 * @insipration:
 *     http://arduiniana.org/libraries/tinygpsplus/
 * 
 * @brief:  This program feeds TinyGPSPlus object with GNSS_SERIAL buffer data
 *          and dumps updated GNSS time on USB (Serial) port.
 *          
 * @board:
 *    LiLyGo
 * 
 * @GNSS module:
 *    Drotek DP0601 RTK GNSS (XL F9P)  
 *
 * @wiring:
 *      LiLyGo RX2      ->  DP0601 UART1 B3 (TX) 
 *      LiLyGo Vin (5V) ->  DP0601 UART1 B1 (5V)
 *      LiLyGo GND      ->  DP0601 UART1 B6 (GND)
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
#include <TinyGPSPlus.h>

/* #################
 * #    PROGRAM    #
 * #################
 */

#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25

// GNSS serial port
HardwareSerial Serialrx(1);

// TinyGPSPlus instance to store GNSS NMEA data (datetim, position, etc.)
TinyGPSPlus gps;
 
void setup() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

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
  Serial.print(gps.location.lng());
  Serial.print(" - LAT = ");
  Serial.print(gps.location.lat());
  Serial.println();  
}
