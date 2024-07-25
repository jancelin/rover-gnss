/*
       @File  : DFRobot_Distance_A01.ino
       @Brief : This example use A01NYUB ultrasonic sensor to measure distance
                With initialization completed, We can get distance value
       @Copyright [DFRobot](https://www.dfrobot.com),2016
                  GUN Lesser General Pulic License
       @version V1.0
       @data  2019-8-28
*/

#include <HardwareSerial.h>
HardwareSerial mySerial(1); // RX, TX
unsigned char data[4] = {};
float distance;

void setup()
{
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, 17,18);
}

void loop()
{
  do {
    for (int i = 0; i < 4; i++)
    {
      data[i] = mySerial.read();
    }
  } while (mySerial.read() == 0xff);

  mySerial.flush();

  if (data[0] == 0xff)
  {
    int sum;
    sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if (sum == data[3])
    {
      distance = (data[1] << 8) + data[2];
      if (distance > 280)
      {
        Serial.print("distance=");
        Serial.print(distance / 10);
        Serial.println("cm");
      } else
      {
        Serial.println("Below the lower limit");
      }
    } else Serial.println("ERROR");
  }
  delay(150);
}
