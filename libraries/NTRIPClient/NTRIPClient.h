#ifndef NTRIP_CLIENT
#define NTRIP_CLIENT

#include <WiFiClient.h>
#include <Arduino.h>
#include <base64.h>

class NTRIPClient : public WiFiClient {
public:
    bool reqSrcTbl(char* host, int &port);   // Request MountPoints List serviced by the NTRIP Caster 
    bool reqRaw(char* host, int &port, char* mntpnt, char* user, char* psw); // Request RAW data from Caster 
    bool reqRaw(char* host, int &port, char* mntpnt); // Non user
    int readLine(char* buffer, int size);
    void sendGGA(const char* ggaMessage, const char* host, int port, const char* user, const char* passwd, const char* mntpnt); // Send GGA message to caster
};

#endif

