/* 
 *******************************
 *   JSN SR04T SENSOR MODULE   *
 *******************************
 * @brief:
 *    This module is loaded to handle URN14 sensor setup and 
 *    distance acquisition.
 */
/*
 ****************
 *  LIBRARIES   *
 ****************
 */
// None

/*
 **************************
 *   GLOBAL DEFINITIONS   *
 **************************
 */
// JSN SR04T sensor disconnection value
#define DIST_NO_VALUE  0

/*
 *********************
 *   SENSOR CONFIG   *
 *********************
 */
// Teensy pins
#define TRIG_PIN 32
#define ECHO_PIN 31

/*
 ***********************
 *   GLOBAL VARIBLES   *
 ***********************
 */

/*
 ***************************
 *   FUNCTION PROTOTYPES   *
 ***************************
 */
void setupDistSensor(volatile bool& deviceConnected);
float readDistance(const float& extTemp_C, volatile bool& deviceConnected);

/*
 ****************************
 *   FUNCTION DEFINITIONS   *
 ****************************
 */
/*
 * @brief: 
 *    Sets up the JSN SR04T distance sensor.
 * @params:
 *    deviceConnected: bool to store if JSN SR04T is connected or not.
 */

void setupDistSensor(volatile bool& deviceConnected) {

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
 
  if (readDistance(TEMP_NO_VALUE, deviceConnected) == DIST_NO_VALUE)
    waitForReboot("Distance sensor JSN SR04T not responding.");
  else
    SERIAL_DBG("JSN SR04T sensor found.")
}

/*
 * @brief: 
 *      returns distance computed from JSN SR04T sensor data.
 * @params:
 *      extTemp_C: temperature used for compensation.
 *      deviceConnected: bool to store if JSN SR04T is connected or not.
 * @retrun:
 *      dist: computed distance.
 */
float readDistance(const float& extTemp_C, volatile bool& deviceConnected)  {


    unsigned long travelTime;
    float soundVelocity = 331.1 + 24.0*0.606;//( extTemp_C == TEMP_NO_VALUE ? 0 : (0.606 * extTemp_C) );
    float dist;

    digitalWrite(TRIG_PIN, HIGH);   // Set trigger pin HIGH 
    delayMicroseconds(20);          // Hold pin HIGH for 20 uSec
    digitalWrite(TRIG_PIN, LOW);    // Return trigger pin back to LOW again.
  
    // Measure time in µs for echo to come back.
    // 10 000µs timeout
    travelTime = pulseIn(ECHO_PIN, HIGH, 10000) / 2;
    // Compute distance
    dist = (soundVelocity * 1000.0/*mm/m*/ / 1000000.0/*µs/s*/) * travelTime;

    // Update sensor connection state
    if (dist == DIST_NO_VALUE)
        deviceConnected = false;
    else
        deviceConnected = true;
    
    return dist;
}
