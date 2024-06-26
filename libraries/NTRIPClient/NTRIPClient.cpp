#include"NTRIPClient.h"
bool NTRIPClient::reqSrcTbl(char* host,int &port)
{
  if(!connect(host,port)){
      Serial.print("Cannot connect to ");
      Serial.println(host);
      return false;
  }
  String p;
  p = String("GET /");
  p = p + String(" HTTP/1.0\r\n");
  p = p + String("Host: caster.centipede.fr:2101\r\n");
  p = p + String("User-Agent: NTRIP Client/buchedv1.01\r\n");
  p = p + String("Accept: text/html\r\n");
  p = p + String("User-Agent: Mozilla/5.0\r\n");
  p = p + String("Connection: keep-alive\r\n");
  p = p + String("\r\n");
  print(p);

  unsigned long timeout = millis();
  while (available() == 0) {
     if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        stop();
        return false;
     }
     delay(10);
  }
  char buffer[12];
  readLine(buffer,sizeof(buffer));
  if(strstr(buffer,"SOURCETABLE 200 OK")!=NULL)
  {
    Serial.print((char*)buffer);
    return false;
  }
  return true;
}

bool NTRIPClient::reqRaw(char* host,int &port,char* mntpnt,char* user,char* psw)
{
    if(!connect(host,port))return false;
    String p="GET /";
    String auth="";
    Serial.println("Request NTRIP");
    
    p = p + mntpnt + String(" HTTP/1.0\r\n"
        "User-Agent: NTRIPClient for Arduino v1.0\r\n"
    );
    
    if (strlen(user)==0) {
        p = p + String(
            "Accept: */*\r\n"
            "Connection: close\r\n"
            );
    }
    else {
        auth = base64::encode(String(user) + String(":") + psw);
        #ifdef Debug
        Serial.println(String(user) + String(":") + psw);
        #endif

        p = p + String("Authorization: Basic ");
        p = p + auth;
        p = p + String("\r\n");
    }
    p = p + String("\r\n");
    print(p);
    #ifdef Debug
    Serial.println(p);
    #endif
    unsigned long timeout = millis();
    while (available() == 0) {
        if (millis() - timeout > 20000) {
            Serial.println("Client Timeout !");
            return false;
        }
        delay(10);
    }
    char buffer[50];
    readLine(buffer,sizeof(buffer));
    if(strncmp((char*)buffer,"ICY 200 OK",10))
    {
      Serial.print((char*)buffer);
      return false;
    }
    return true;
}
bool NTRIPClient::reqRaw(char* host,int &port,char* mntpnt)
{
    return reqRaw(host,port,mntpnt,"","");
}
int NTRIPClient::readLine(char* _buffer,int size)
{
  int len = 0;
  while(available()) {
    _buffer[len] = read();
    len++;
    if(_buffer[len-1] == '\n' || len >= size) break;
  }
  _buffer[len]='\0';

  return len;
}
