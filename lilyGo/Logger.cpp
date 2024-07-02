// Logger.cpp
#include "Logger.h"

void logMessage(int level, const char* message) {
    if (level <= LOG_LEVEL) {
        Serial.println(message);
    }
}

void logMessage(int level, const char* message, int value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void logMessage(int level, const char* message, const char* value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void logMessage(int level, const char* message, float value, int decimals) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value, decimals);
    }
}

void logMessage(int level, const char* message, int32_t value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}

void logMessage(int level, const char* message, uint32_t value) {
    if (level <= LOG_LEVEL) {
        Serial.print(message);
        Serial.println(value);
    }
}
