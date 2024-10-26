#ifndef PARAM_H
#define PARAM_H
#include <time.h>
#include <Arduino.h>

extern const char *apSSID;
extern const char *apPassword;
extern const char *ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;
extern long lastUpdatCheck;
extern const char *version;
extern const long updateCheckInterval;
extern bool devnet;
extern bool opener_running;
extern long sucessfulGuesses;
extern long failedGuesses;
extern tm lastSubmittedTime;
extern unsigned long lastRequestTime;
extern const char *apiUrl;
extern const char *devUrl;
extern String apiKey;
extern IPAddress apIP;
extern const byte DNS_PORT;
extern int sleepTime;
extern String LastGuessAccepted;

#endif // PARAM_H