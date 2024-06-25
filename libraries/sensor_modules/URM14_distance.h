/* 
 ***************************
 *   URM14 SENSOR MODULE   *
 ***************************
 * @brief:
 *    This module is loaded to handle URN14 sensor setup and 
 *    distance acquisition.
 */
/*
 ****************
 *  LIBRARIES   *
 ****************
 */
#include <ModbusMaster.h>

/*
 **************************
 *   GLOBAL DEFINITIONS   *
 **************************
 */
// URM14 read value when sensor disconnected
#define DIST_NO_VALUE  UINT16_MAX / 10.0

/*
 *********************
 *   SENSOR CONFIG   *
 *********************
 */
// Sensor serial port
#define URM14_SERIAL  Serial4
// Sensor baudrate
#define URM14_BAUDRATE 9600
// Sensor ID
#define URM14_ID  (uint16_t)0x11
// Sensor registers
#define URM14_ID_REG        (uint16_t)0x02
#define URM14_DISTANCE_REG  (uint16_t)0x05
#define URM14_EXT_TEMP_REG  (uint16_t)0x07
#define URM14_CONTROL_REG   (uint16_t)0x08
// Sensor config register bit values
#define TEMP_CPT_SEL_BIT      ((uint16_t)0x01)      // Use custom temperature compensation
#define TEMP_CPT_ENABLE_BIT   ((uint16_t)0x01 << 1) // Enable temperature compensation
#define MEASURE_MODE_BIT      ((uint16_t)0x00 << 2) // Passive(1)/auto(0) measure mode
#define MEASURE_TRIG_BIT      ((uint16_t)0x00 << 3) // Request mesure in passive mode. Unused in auto mode
// Modbus DE & RE pins
#define DE_PIN  30 // RE = ~DE => Wired to pin 30 as well

/*
 ***********************
 *   GLOBAL VARIBLES   *
 ***********************
 */
// ModbusMaster sensor object for URM14
ModbusMaster urm14;
// URM14 config
uint16_t urm14_config_bits = MEASURE_TRIG_BIT | MEASURE_MODE_BIT | TEMP_CPT_ENABLE_BIT | TEMP_CPT_SEL_BIT;

/*
 ***************************
 *   FUNCTION PROTOTYPES   *
 ***************************
 */
void preTransCbk()  {  digitalWrite(DE_PIN, HIGH); }
void postTransCbk() {  digitalWrite(DE_PIN, LOW);  }
void setupDistSensor(volatile bool& deviceConnected);
float readDistance(const float& extTemp_C, volatile bool& deviceConnected);
/*
 ****************************
 *   FUNCTION DEFINITIONS   *
 ****************************
 */
/*
 * @brief: 
 *    Sets up the URM14 ultrasoic sensor
 * @params:
 *    sensor: ModbusMaster instance that represents the senter on the bus.
 *    sensorID: Sensor id on the bus.
 *    sensorBaudrate: Baudrate to communication with the sensor.
 *    preTransCbk: Callback called before modbus tansmission to set RS485 interface in transmission mode.
 *    postTransCbk: Callback called after modbus tansmission to set RS485 interface back in reception mode.
 *    deviceConnected: Bool to store if URM14 is connected or not.
 */
void setupDistSensor(volatile bool& deviceConnected)  {

  // Modbus communication errors
  uint8_t mbError;

  // RS485 DE pin setup
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);

  // Set Modbus communication
  URM14_SERIAL.begin(URM14_BAUDRATE);
  // Modbus slave id 0x11 on serial port URM14_SERIAL
  urm14.begin(URM14_ID, URM14_SERIAL);
  // Set pre/post transmision callbacks in ModbusMaster object
  urm14.preTransmission(preTransCbk);
  urm14.postTransmission(postTransCbk);

  // Writing config
  mbError = urm14.writeSingleRegister(URM14_CONTROL_REG, urm14_config_bits); //Writes the setting value to the control register
  if (mbError != ModbusMaster::ku8MBSuccess)
    waitForReboot("Modbus : Config could not be written to UMR14 sensor, check wiring.");
  else
    SERIAL_DBG("Modbus : UMR14 sensor found and configured!\n")
  
  deviceConnected = true;
  SERIAL_DBG("Done.\n")
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

    // Store Modbus communication errors
    uint8_t mbError;
    // Store distance
    float dist;

    // External compensation: Updade external URM14 temperature register
    if (!TEMP_CPT_ENABLE_BIT && TEMP_CPT_SEL_BIT && extTemp_C != TEMP_NO_VALUE)  {
        mbError = urm14.writeSingleRegister(URM14_EXT_TEMP_REG, (uint16_t)extTemp_C * 10.0);
        // Check for Modbus errors
        if (mbError != ModbusMaster::ku8MBSuccess)  {
          dist = DIST_NO_VALUE;
          deviceConnected = false;
        }
        else
          deviceConnected = true;
    }

    // Trigger mode: Set trigger bit to request one measurement
    if (MEASURE_MODE_BIT) {
        mbError = urm14.writeSingleRegister(URM14_CONTROL_REG, urm14_config_bits); //Writes the setting value to the control register
        if (mbError != ModbusMaster::ku8MBSuccess)  {
            dist = DIST_NO_VALUE;
            deviceConnected = false;
        }
        else
            deviceConnected = true;
    }

    // Readng distance input register at 0x05
    // Should use readInputRegisters() but somehow doesn't work
    // Trhows ku8MBIllegalDataAddress error (0x02)
    // ToDo : understand error (might be manufacturer who did not follow Modbus standard)
    mbError = urm14.readHoldingRegisters(URM14_DISTANCE_REG, 1);
    // Check for Modbus errors
    if (mbError != ModbusMaster::ku8MBSuccess)  {
        dist = DIST_NO_VALUE;
        deviceConnected = false;
    }
    else  {
        dist = urm14.getResponseBuffer(0) / 10.0;
        deviceConnected = true;
    }
    return dist;
}