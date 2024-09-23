#ifndef HEADER_H
#define HEADER_H

#include <Arduino.h>

/* Serial */
#define BAUD_SERIAL 115200 // Debug Serial baund rate
#define BAUND_RECEIVER 0   // Auto detect baud //921600 // GPS receiver baund rate
//#define SERIAL_SIZE_RX 1024
#ifndef SERIAL_SIZE_RX
//static const uint8_t SERIAL_SIZE_RX = uint8_t(112);
#define SERIAL_SIZE_RX 112U // MAX BUFFER = 112
#endif // SERIAL_SIZE_RX

#ifndef CONFIG_RX_BUFFER_SIZE
#define RX_BUFFER_SIZE 1024 * 4
#else
#define RX_BUFFER_SIZE CONFIG_MAX_RX_BUFFER_SIZE
#endif // SERIAL_SIZE_RX

/* SD card */
#ifdef ESP32
#ifdef TTGO_BOARD
#define CS_PIN 13
// #define RXD2 32
// #define TXD2 33
#define RXD2 16
#define TXD2 17

// #define RXD1 13
// #define TXD1 23
#define RXD1 16
#define TXD1 17

#else
#define CS_PIN 5 // SD card cs_pin (default D5 GPIO5 on ESP32 DevkitV1)
#define RXD2 16
#define TXD2 17
// #define RXD1 4
// #define TXD1 15
#define RXD1 16
#define TXD1 17

#endif // TTGO_BOARD
#else
#error Platform not supported
#endif // ESP32

#endif // HEADER_H