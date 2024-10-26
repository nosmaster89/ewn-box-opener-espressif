
#include <ArduinoJson.h>
#include <time.h>
#include <WiFi.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <AsyncWebSocket.h>
#include <param.h>
#include <websockets.h>

AsyncWebSocket ws("/ws");

// Websocket functions
void logToWebSocket(const String &logLine)
{
    // Send the log data to all connected WebSocket clients
    ws.textAll(logLine);

    // Optionally, also print to the Serial Monitor
    Serial.println(logLine);
}

bool shouldSendUpdate()
{
    // Logic to determine when to send updates (e.g., every few seconds)
    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();

    if (currentTime - lastUpdateTime > 5000)
    { // 5-second interval
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        // logToWebSocket("Client connected");
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        // logToWebSocket("Client disconnected");
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            // the whole message is in a single frame and we got all of it's data
            // logToWebSocket("Received message with length " + String(len) + "\n");
            if (info->opcode == WS_TEXT)
            {
                data[len] = 0;
                // logToWebSocket("Received text: " + String((char *)data) + "\n");
            }
            else
            {
                // logToWebSocket("Received binary data\n");
            }
        }
        // Handle incoming messages if needed
    }
}
void sendStatsToClients()
{
    // Create a JSON object to hold the stats
    JsonDocument jsonDoc;

    // Fill in the data
    jsonDoc["DevNet"] = devnet;
    jsonDoc["IP"] = WiFi.localIP().toString();
    jsonDoc["Uptime"] = millis() / 1000 / 60 / 60;           // Convert milliseconds to hours
    jsonDoc["LastRequestTime"] = mktime(&lastSubmittedTime); // Use the timestamp
    jsonDoc["LastRequestStatus"] = LastGuessAccepted;
    jsonDoc["failedGuesses"] = failedGuesses;
    jsonDoc["sucessfulGuesses"] = sucessfulGuesses;
    jsonDoc["APIKey"] = apiKey;
    jsonDoc["Version"] = version;
    jsonDoc["RSSI"] = WiFi.RSSI();
    jsonDoc["waitTime"] = sleepTime;
    tm timeinfo;
    // logToWebSocket("    local time");
    if (getLocalTime(&timeinfo))
    {
        jsonDoc["current_time"] = mktime(&timeinfo);
    }

    // Serialize the JSON data to a string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Broadcast the stats to all connected WebSocket clients
    ws.textAll(jsonString);
}