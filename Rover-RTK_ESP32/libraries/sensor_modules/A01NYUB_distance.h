/* 
 *****************************
 *   A01NYUB SENSOR MODULE   *
 *****************************
 * @brief:
 *    This module is loaded to handle A01NYUB sensor setup and 
 *    distance acquisition.
 */
/*
 ****************
 *  LIBRARIES   *
 ****************
 */
// None required

/*
 **************************
 *   GLOBAL DEFINITIONS   *
 **************************
 */
// A01NYUB read value when sensor disconnected
#define DIST_NO_VALUE  -257/10.0
// A01YUB response time
#define RESP_TIME   150/*ms*/

/*
 *********************
 *   SENSOR CONFIG   *
 *********************
 */
// Sensor serial port
#define A01NYUB_SERIAL  Serial2
// Sensor baudrate
#define A01NYUB_BAUDRATE 9600
// Sensor TX pin
#define A01NYUB_TX_PIN  9

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
 *    Sets up the A01NYUB ultrasoic sensor
 * @params:
 *    deviceConnected: Bool to store if A01NYUB is connected or not.
 */
void setupDistSensor(volatile bool& deviceConnected)  {

  A01NYUB_SERIAL.begin(A01NYUB_BAUDRATE);
  digitalWrite(A01NYUB_TX_PIN, LOW);

  readDistance(25, deviceConnected);

  if (!deviceConnected)
    waitForReboot("No A01NYUB distance sensor detected, check wiring...");
}

/*
 * @brief: 
 *    returns distance read in URM14 sensor.
 * @params:
 *    deviceConnected: bool to store if URM14 is connected or not.
 * @retrun:
 *    extTemp_C: temperature to use for compensation.
 */
float readDistance(const float& extTemp_C, volatile bool& deviceConnected)   {

    A01NYUB_SERIAL.clear();
    delay(RESP_TIME);   

    if(A01NYUB_SERIAL.read() == 0xFF)   {
        deviceConnected = true;
        return A01NYUB_SERIAL.read()*256 + A01NYUB_SERIAL.read();
    }
    else
        deviceConnected = false;
    return DIST_NO_VALUE;   
}