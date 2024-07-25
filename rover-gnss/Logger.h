// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include "Config.h"

void logMessage(int level, const char* message);
void logMessage(int level, const char* message, int value);
void logMessage(int level, const char* message, const char* value);
void logMessage(int level, const char* message, float value, int decimals = 2);
void logMessage(int level, const char* message, int32_t value);
void logMessage(int level, const char* message, uint32_t value);

#endif // LOGGER_H
