#ifndef WS_H
#define WS_H
#include <ESPAsyncWebServer.h>
void logToWebSocket(const String &logLine);
bool shouldSendUpdate();
void sendStatsToClients();
// show ws
extern AsyncWebSocket ws; // Declare ws as an external variable
#endif
