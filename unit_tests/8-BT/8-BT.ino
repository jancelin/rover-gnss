//https://github.com/IgorKilipenko/ublox_esp_ble_uart/blob/main/src/main.cpp
//#include <Arduino.h>
#include "header.h"
#include "queue"
#include "BluetoothSerial.h"

/* CONFIG for UART1 */
#define PIN_RX 16
#define PIN_TX 17 

HardwareSerial *RTCM{&Serial1};
HardwareSerial *Receiver{&Serial2};

const std::vector<uint8_t> readReceiverData()
{
    int count = 0;
    if ((count = Receiver->available()) > 0)
    {
        std::vector<uint8_t> buffer(count);
        int len = Receiver->readBytes(buffer.data(), count);
        if (len != count)
        {
            log_e("Not all bytes read from uart, available: [%i], read: [%i]\n", count, len);
        }
        if (len > 0)
        {
            // queue_buffer.push(buffer);
            log_d("data count %d\n", buffer.size());
            return buffer;
        }
    }
    return std::vector<uint8_t>(0);
}

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

std::vector<uint8_t> buffer(RX_BUFFER_SIZE * 2);
int tryCount = 0;

void receiveFnc()
{
    log_d("Received UART Data. Available For Reading = %u\n", Receiver->available());
    while (Receiver->available())
        Receiver->read(); // Discard received data.
}

void receiveErrorFnc(hardwareSerial_error_t error)
{
    log_e("UART Reception Error: ");
    bool bufferOverflow = false;
    switch (error)
    {
    case UART_BREAK_ERROR:
        log_e("UART_BREAK_ERROR\n");
        break;
    case UART_BUFFER_FULL_ERROR:
        log_e("UART_BUFFER_FULL_ERROR\n");
        bufferOverflow = true;
        break;
    case UART_FIFO_OVF_ERROR:
        log_e("UART_FIFO_OVF_ERROR\n");
        bufferOverflow = true;
        break;
    case UART_FRAME_ERROR:
        log_e("UART_FRAME_ERROR\n");
        break;
    case UART_PARITY_ERROR:
        log_e("UART_PARITY_ERROR\n");
        break;
    }
    if (bufferOverflow)
    {
        log_w("Clear buffers");
        while (Receiver->available())
            Receiver->read();
        buffer.clear();
    }
}

void onGnssReceiveCb()
{
    if (!SerialBT.hasClient())
    {
        while (Receiver->available()) {
            Receiver->read();
            // Serial.print(Receiver->read());
        }
        return;
    }
    if (Receiver->available())
    {
        while (Receiver->available())
            buffer.push_back(Receiver->read());
    }
}

void onRtcmReceiveCb()
{
    if (!SerialBT.hasClient())
    {
        while (RTCM->available())
            Receiver->read();
        return;
    }
    /*const int len = RTCM->available();
    if (len)
    {
        while (RTCM->available())
            buffer.push_back(RTCM->read());
    }*/
}

void setup()
{
    Serial.begin(BAUD_SERIAL);
    SerialBT.begin("RoveF9pESP32_ROM"); // Bluetooth device name
    Serial.println("The device started, now you can pair it with bluetooth!");

    Receiver->setRxBufferSize(RX_BUFFER_SIZE);
    Receiver->onReceive(onGnssReceiveCb, false);
    Receiver->onReceiveError(receiveErrorFnc);
    // Receiver->setRxTimeout(1);

    Receiver->begin(115200, SERIAL_8N1, PIN_RX, PIN_TX /*, false, 20000UL, static_cast<uint8_t>(SERIAL_SIZE_RX)*/);
    // Receiver->begin(BAUND_RECEIVER, SERIAL_8N1, RXD2, TXD2 /*, false, 20000UL, static_cast<uint8_t>(SERIAL_SIZE_RX)*/);

    // delay(500);
  
    unsigned long detectedBaudRate = Receiver->baudRate();
    if (detectedBaudRate)
        log_d("Receiver uart baudrate detecyed -> [ %lu ]", detectedBaudRate);
    else
    {
        log_w("Receiver baudrate not detected");
        Receiver->begin(115200, SERIAL_8N1, PIN_RX, PIN_TX /*, false, 20000UL, SERIAL_SIZE_RX*/);
        // Receiver->begin(BAUD_SERIAL, SERIAL_8N1, RXD2, TXD2 /*, false, 20000UL, SERIAL_SIZE_RX*/);

    }

    RTCM->setRxBufferSize(RX_BUFFER_SIZE);
    RTCM->onReceive(onRtcmReceiveCb, false);
    RTCM->onReceiveError(receiveErrorFnc);

    // RTCM->begin(115200, SERIAL_8N1, 16, 17);
    // RTCM->begin(115200, SERIAL_8N1, RXD1, TXD1);
}

void loop()
{


    if (SerialBT.hasClient())
    {
        const size_t bufferSize = buffer.size();
        if (bufferSize >= RX_BUFFER_SIZE || (bufferSize > 0 && tryCount > 3))
        {
            const size_t writeCount = SerialBT.write(buffer.data(), min(bufferSize, (size_t)RX_BUFFER_SIZE * 4));
            log_d("Write %lu bytes from %lu", writeCount, bufferSize);
            int partSize = bufferSize - writeCount;
            if (partSize > 0)
            {
                log_w("WARN BT Buffer overflow = %lu, Total buffer size = %lu\n", partSize, bufferSize);
            }

            tryCount = 0;
            buffer.clear();

            if (writeCount >= RX_BUFFER_SIZE * 2)
            {
                delay(10);
                SerialBT.flush();
            }
            else
            {
                delay(1);
            }
        }
        else
        {
            if (bufferSize > 0)
            {
                tryCount++;
                SerialBT.flush();
            }
        }

        const int inLen = SerialBT.available();
        if (inLen)
        {
            uint8_t bytes[inLen];
            const size_t sizeRes = SerialBT.readBytes(bytes, inLen);
            const size_t writeCount = Receiver->write(bytes, inLen);
            if (writeCount > 0)
            {
                delay(10);
            }
            else
            {
                delay(1);
            }
        }
    }
    else
    {
        if (buffer.size() > 0)
        {
            buffer.clear();
            tryCount = 0;
        }
    }
    delay(1);
}
