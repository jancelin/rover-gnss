#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>

HardwareSerial MySerial(1);
#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25

const char* ssid     = "ici";
const char* password = "12345678";
IPAddress server(192, 168, 1, 100);  // IP address of the server
int port = 80;            

char* host = "caster.centipede.fr";
int httpPort = 2101; //port 2101 is default port of NTRIP caster
char* mntpnt = "CT";
char* user   = "rover-gnss-tester";
char* passwd = "";
NTRIPClient ntrip_c;

const char* udpAddress = "192.168.1.255";
const int udpPort = 9999;

int trans = 3;  //0 = serial, 1 = udp, 2 = tcp client, 3 = MySerial, 4 = myserial Choose wich out you want use. for rs232 set 0 et connect tx f9p directly to rs232 module

WiFiUDP udp;

void setup() {

    // POWER_PIN : This pin controls the power supply of the MICRO PCI card
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);
  MySerial.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);//serial port to send rtcm to f9p
  delay(100);
  MySerial.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(100);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Requesting SourceTable.");
  if(ntrip_c.reqSrcTbl(host,httpPort)){
    char buffer[512];
    delay(5);
    while(ntrip_c.available()){
      ntrip_c.readLine(buffer,sizeof(buffer));
      Serial.print(buffer); 
    }
  }
  else{
    Serial.println("SourceTable request error");
  }
  Serial.print("Requesting SourceTable is OK\n");
  ntrip_c.stop(); //Need to call "stop" function for next request.
  
  Serial.println("Requesting MountPoint's Raw data");
  if(!ntrip_c.reqRaw(host,httpPort,mntpnt,user,passwd)){
    delay(15000);
    ESP.restart();
  }
  Serial.println("Requesting MountPoint is OK");

  
}

void loop() {

 WiFiClient client;
 
  while(ntrip_c.available())
    {
        char ch = ntrip_c.read();        
        MySerial.print(ch);
        
    }
  while (MySerial.available())
    {
    String s = MySerial.readStringUntil('\n');
    switch (trans)
      {
        case 0:  //serial out
        Serial.println(s);
        break;
        case 1:  //udp out
        udp.beginPacket(udpAddress, udpPort);
        udp.print(s);
        udp.endPacket();
        break;
        case 2:  //tcp client out
        if (!client.connect(server, port))
          {
            Serial.println("connection failed");
            return;
          }
        client.println(s);
        while (client.connected())
          {
            while (client.available())
            {
              char c = client.read();
              Serial.print(c);
            }
          }
        client.stop();
        break;
        case 3:  //MySerial out
        MySerial.println(s);
        break;
        case 4:  //MySerial out
        MySerial.println(s);
        break;
        default:  //mauvaise config
        Serial.println("mauvais choix ou oubli de configuration");
        break;
      }
    }
}
