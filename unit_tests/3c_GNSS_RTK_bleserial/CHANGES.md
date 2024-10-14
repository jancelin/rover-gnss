# Changes

## Introduction

This is a modified version of the original code `rover-gnss/unit_tests/3b_GNSS_RTK` where I modified it to let the ESP32 send the GPS data using the BLE protocol.

### Problem with iPhone and Mac 
  
The original code is using the classic bluetooth profile (`SPP`, i.e., `Bluetooth Serial`), which is well supported by Android, but not by Apple devices (like on iOS).
> See <https://stackoverflow.com/questions/76604020/is-there-a-way-to-make-esp32-discoverable-as-a-bluetooth-device-on-a-flutter-ios>.
  
> **References:**
> 1. <https://randomnerdtutorials.com/esp32-ble-server-client/>
> 2. <https://randomnerdtutorials.com/esp32-bluetooth-low-energy-ble-arduino-ide/>
> 3. [Increase size of BLE messages](https://www.esp32.com/viewtopic.php?t=4546), as by default is 20mb (according to that link).

This resulted into the modified sketch `3c_GNSS_RTK_ble` in `rover-gnss-master/unit_tests`.

### Problem of first prototype sketch

After flashing, I used an Android phone and installed the app `LightBlue` to read the raw data sent by the esp32 board.

Now, I searched for an app supporting reading GNSS data over BLE. I found `GNSS Master`.
> **NB:** In the original version of the code, the app `Bluetooth GNSS` was used to read the gnss data from the esp32 board over the `SPP` protocol. However, this app does not seem to support BLE.

So I switched to `GNSS Master`.

**However**, this introduced a new problem.
`GNSS Master` supports BLE, but only with the profile *Serial*!
I has to modify the code in order to change the server profile UID.
  
On this regard, I found the library [ESP32_BleSerial](https://github.com/avinabmalla/ESP32_BleSerial/tree/master) which provides a class `BLESerial`.
This class works in the same way as the library `BluetoothSerial` used in the original sketch of the formation.
> Interestingly, in the implementation of `ESP32_BleSerial`, the developer does the same stuff as me for the BLE server and etc., but he is using a different UID for the server.
> Thus, it seems a valid candidate library to start playing from.

I created a new project named `ble_rover` in `$HOME/Arduino/`, where the new sketch is named `3c_GNSS_RTK_bleserial`.

### Other source of inspiration

For the service and characteristics UIDs, I took inspiration from the example in the repository [esp32-ble-ios-demo](https://github.com/marcboeker/esp32-ble-ios-demo/tree/master).
In particular from the files:
- [include/bletest.h](https://github.com/marcboeker/esp32-ble-ios-demo/blob/master/esp32/include/bletest.h)
- [src/main.cpp](https://github.com/marcboeker/esp32-ble-ios-demo/blob/master/esp32/src/main.cpp)

