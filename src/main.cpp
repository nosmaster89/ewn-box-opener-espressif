/*
 * ------------------------------------------------------------------------
 * Project Name: esp32-ewn-box-opener
 * Description: Esp32 port of the Box Opener client for mining EWN tokens.
 * Author: Crey
 * Repository: https://github.com/cr3you/esp32-ewn-box-opener/
 * Date: 2024.10.04
 * Version: 1.0.1
 * License: MIT
 * ------------------------------------------------------------------------
 */

#include "bip39/bip39.h"
#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>
#include <HTTPUpdate.h>
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "esp_ota_ops.h"
#include <ESPmDNS.h>
#include <rgbled.h>
#include <led.h>

#define NUMPIXELS 1
#define DELAYVAL 500
#define BRIGHTNESS 120
#ifdef RGBpin
int rgbpin = RGBpin;
#endif
const char *apSSID = "ESP32_Setup";
const char *apPassword = "123456789";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;
long lastUpdatCheck = 0;
const char *version = "0.0.2";
// every 5min
const long updateCheckInterval = 300000;
// ledpin
bool devnet = false;
bool opener_running = true;
//=====wifi setup
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
// Handle WebSocket events
void logToWebSocket(const String &logLine)
{
  // Send the log data to all connected WebSocket clients
  ws.textAll(logLine);

  // Optionally, also print to the Serial Monitor
  Serial.print(logLine);
}
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    logToWebSocket("Client connected");
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    logToWebSocket("Client disconnected");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      // the whole message is in a single frame and we got all of it's data
      logToWebSocket("Received message with length " + String(len) + "\n");
      if (info->opcode == WS_TEXT)
      {
        data[len] = 0;
        logToWebSocket("Received text: " + String((char *)data) + "\n");
      }
      else
      {
        logToWebSocket("Received binary data\n");
      }
    }
    // Handle incoming messages if needed
  }
}
tm lastSubmittedTime;
unsigned long lastRequestTime;
const char *apiUrl = "https://api.erwin.lol/";        // mainnet
const char *devUrl = "https://devnet-api.erwin.lol/"; // devnet
String apiKey = "";                                   // <---------------------- SET THIS !!!
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);
const byte DNS_PORT = 53;
Adafruit_NeoPixel pixels;
const int numGuesses = 50;
String mnemonics[numGuesses]; // bip39 mnemonic table
int sleepTime = 10000;        // default sleep time in ms
String LastGuessAccepted = "undefined";
String processHeader(String header)
{
  int rssi = WiFi.RSSI();
  header.replace("{{rssi}}", String(rssi));

  return header;
}
String processTemplate(String html, bool devnet, String LastGuess, String apiKey, tm lastTime, String version)
{
  logToWebSocket("Processing template");
  logToWebSocket("LastGuess: " + LastGuess);
  logToWebSocket("LastGuessAccepted: " + LastGuessAccepted);
  logToWebSocket("API Key: " + apiKey);
  logToWebSocket("Version: " + version);
  logToWebSocket("DevNet: " + String(devnet));
  logToWebSocket("IP: " + WiFi.localIP().toString());
  logToWebSocket("Uptime: " + String(millis() / 1000 / 60 / 60));
  logToWebSocket("LastRequestTime: " + String(mktime(&lastTime)));

  html.replace("{{DevNet}}", String(devnet ? "Enabled" : "Disabled"));
  html.replace("{{IP}}", WiFi.localIP().toString());
  html.replace("{{Uptime}}", String(millis() / 1000 / 60 / 60));
  html.replace("{{ssid}}", preferences.getString("ssid"));
  html.replace("{{password}}", preferences.getString("password"));
  html.replace("{{apiKey}}", apiKey);
  html.replace("{{rgbpin}}", String(preferences.getInt("rgbpin")));
  html.replace("{{devnet}}", devnet ? "true" : "false");
  int32_t rssi = WiFi.RSSI();
  logToWebSocket("RSSI: " + String(rssi));
  html.replace("{{rssi}}", String(rssi));
  Serial.println("opener running:" + opener_running);
  html.replace("{{OPENER_STATUS}}", opener_running ? "<h2 class='bg-success'>Opener is running</h2>" : "<h2 class='bg-danger'>Opener is stopped</h2>");
  ;

  // turn lasttime into a utcTimestamp
  time_t utc_timestamp = mktime(&lastTime);
  html.replace("{{LastRequestTime}}", String(utc_timestamp));

  html.replace("{{LastRequestStatus}}", LastGuess);
  html.replace("{{apiKey}}", apiKey);
  html.replace("{{Version}}", version);
  return html;
}
void handleLogger(AsyncWebServerRequest *request)
{
  logToWebSocket("Logger requested");

  request->send(SPIFFS, "/logger.html", String(), false);
}
void handleRoot(AsyncWebServerRequest *request)
{

  File file = SPIFFS.open("/index.html", "r");
  if (!file)
  {
    request->send(500, "text/plain", "File not found");
    return;
  }
  String html = processTemplate(file.readString(), devnet, LastGuessAccepted, apiKey, lastSubmittedTime, version);
  // Read the HTML file into a String
  file.close();

  request->send(200, "text/html", html);
}
void handleUpdate(AsyncWebServerRequest *request)
{
  // serial log the size current size of the spiffs in human readable format
  // Serial.printf("Free space: %u\n", SPIFFS.totalBytes() - SPIFFS.usedBytes());

  // Serial log the size of the uploaded file in human readable format
  Serial.printf("File size: %u\n", request->contentLength());
  // serial log the remaining size in spiffs in human readable format
  Serial.printf("Free space: %u\n", SPIFFS.totalBytes() - SPIFFS.usedBytes());

  // check for a file called upload.bin in the spiffs
  // if it exists delete it
  if (SPIFFS.exists("/upload.bin"))
  {
    SPIFFS.remove("/upload.bin");
  }
  // create a file called upload.bin
  // File file = SPIFFS.open("/upload.bin", "w");
  // // if the file is not created
  // if (!file)
  // {
  //   // send a 500 error
  //   request->send(500, "text/plain", "Error creating file");
  //   return;
  // }
  // // stream in the file from the post request to the file
  // if (request->hasParam("file", true, true))
  // {
  //   AsyncWebParameter *p = request->getParam("file", true, true);
  //   file.write(reinterpret_cast<const uint8_t *>(p->value().c_str()), p->value().length());
  // }
  // file.close();
  // calculate the hash of the file
}
void handleUpdater(AsyncWebServerRequest *request)
{

  logToWebSocket("Updater requested");
  request->send(SPIFFS, "/update.html", String(), false);
}
void handleReboot(AsyncWebServerRequest *request)
{
  logToWebSocket("Reboot requested");
  // use spiffs to return the reboot page
  request->send(SPIFFS, "/reboot.html", String(), false);
}
void handleSave(AsyncWebServerRequest *request)
{
  String ssid = request->arg("ssid");
  String password = request->arg("password");
  String apiKey = request->arg("apiKey");
  uint8_t rgbpin = request->arg("rgbpin").toInt();
  bool dev = request->arg("devnet") == "on";
  preferences.putInt("rgbpin", rgbpin);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("apiKey", apiKey);
  preferences.putBool("devnet", dev);
  // if wifi is in ap mode
  if (WiFi.getMode() == WIFI_AP)
  {
    // restart the device
    request->send(200, "text/html", "Saved! Restarting... Please check the serial monitor for the new IP address");
    delay(1000);
    ESP.restart();
  }
  else
  {
    const char *html = "<!DOCTYPE html>"
                       "<html lang=\"en\">"
                       "<head>"
                       "<meta charset=\"UTF-8\">"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                       "<title>Rebooting</title>"
                       "<script>"
                       "setTimeout(function() { window.location.href = '/stats'; }, 10000);"
                       "</script>"
                       "</head>"
                       "<body>"
                       "<h1>Rebooting...</h1>"
                       "</body>"
                       "</html>";
    request->send(200, "text/html", html);
    delay(1000);
    ESP.restart(); // Restart ESP32 to apply changes
  }
  // craft some html to send restarting and triger the browser to redirect to the /stats in 10 seconds

  // String html = "<html><body>Saved! Restarting...</body></html>";
}
void handleNoContent(AsyncWebServerRequest *request)
{
  request->send(204, "text/plain", "");
}
void handleSettings(AsyncWebServerRequest *request)
{
  File file = SPIFFS.open("/settings.html", "r");
  if (!file)
  {
    request->send(500, "text/plain", "File not found");
    return;
  }

  // Read the HTML file into a String
  String html = file.readString();
  file.close();

  // Replace placeholders in the HTML
  html = processTemplate(html, devnet, LastGuessAccepted, apiKey, lastSubmittedTime, version);

  // Send the modified HTML page
  request->send(200, "text/html", html);
}
void handleStats(AsyncWebServerRequest *request)
{

  File file = SPIFFS.open("/stats.html", "r");
  if (!file)
  {
    request->send(500, "text/plain", "File not found");
    return;
  }

  // Read the HTML file into a String
  String html = file.readString();
  file.close();

  // Example dynamic data

  // Replace placeholders in the HTML
  html = processTemplate(html, devnet, LastGuessAccepted, apiKey, lastSubmittedTime, version);

  // Send the modified HTML page
  request->send(200, "text/html", html);
};
void HandleConfirmReboot(AsyncWebServerRequest *request)
{
  // send back a themed page to confirm the reboot
  request->send(SPIFFS, "/confirmreboot.html", String(), false);
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
  jsonDoc["APIKey"] = apiKey;
  jsonDoc["Version"] = version;

  // Serialize the JSON data to a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Broadcast the stats to all connected WebSocket clients
  ws.textAll(jsonString);
}
void triggerSucess()
{
#ifdef LEDPIN
  blinkLED();
#endif
#ifdef RGBPIN
  lightPixels(pixels.Color(0, BRIGHTNESS, 0));
  delay(50);
  lightPixels(pixels.Color(0, 0, 0));
#endif
}
void triggerError()
{
#ifdef RGBPIN
  lightPixels(pixels.Color(BRIGHTNESS, 0, 0));
  delay(50);
  lightPixels(pixels.Color(0, 0, 0));
#endif
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
// handle wildcard css lookup
void handleCss(AsyncWebServerRequest *request)
{
  String path = request->url();

  // Ensure the file is opened in read mode
  File file = SPIFFS.open(path, "r");
  if (!file)
  {
    request->send(404, "text/plain", "File Not Found");
    return;
  }

  // Set the content type based on the file extension
  String contentType = "text/css";
  Serial.println("Sending CSS: " + path);
  // print out file
  // logToWebSocket(file.readString());

  // Send the file content with the correct content type
  request->send(file, contentType);
  file.close();
}

// handle wildcard js lookup
void handleJs(AsyncWebServerRequest *request)
{
  String path = request->url();
  File file = SPIFFS.open(path, "r");
  if (!file)
  {
    request->send(404, "text/plain", "File Not Found");
    return;
  }
  String contentType = "text/javascript";
  Serial.println("Sending JS");
  request->send(file, contentType);
  file.close();
};

void setup()
{
  esp_ota_mark_app_valid_cancel_rollback();

  Serial.begin(115200);

  logToWebSocket("trying to load preferences");
  preferences.begin("wifiCreds", false);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    logToWebSocket("An Error has occurred while mounting SPIFFS");
    return;
  }

  const esp_partition_t *partition;
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  String key = preferences.getString("apiKey", "");
  Serial.println("TESING");
  devnet = preferences.getBool("devnet", false);

//
// if d == "true" then devnet = true else false
// devnet =
#ifdef RGBPIN
  setupRGBLED();
#endif
#ifdef LEDPIN
  setupLED();
#endif

  // lightPixels(pixels.Color(BRIGHTNESS, 0, 0));

  if (ssid == "" || password == "")
  {

    // No credentials, start in Access Point mode
    WiFi.softAP(apSSID, apPassword);
    logToWebSocket("Started Access Point");
    logToWebSocket("connect to " + WiFi.softAPIP().toString());
    // lightPixels(pixels.Color(102, 0, 255));
    // Start the web server
    dnsServer.start(DNS_PORT, "*", apIP);
    server.on("/generate_204", handleNoContent);       // Android Captive Portal
    server.on("/hotspot-detect.html", handleSettings); // iOS Captive Portal
    server.on("/captive.apple.com", handleSettings);   // Another iOS URL
  }

  else
  {
    // Connect to WiFi

    WiFi.begin(ssid.c_str(), password.c_str());

    if (!MDNS.begin("erwin"))
    { // Set the hostname to "esp32.local"
      Serial.println("Error setting up MDNS responder!");
      while (1)
      {
        delay(1000);
      }
    }
    logToWebSocket("mDNS responder started");

    logToWebSocket("Connecting to WiFi...");
    logToWebSocket("ssid: " + ssid + "\n");
    logToWebSocket("password: " + password + "\n");
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      logToWebSocket("WiFi Failed! Starting AP mode");
      WiFi.softAP(apSSID, apPassword);
#ifdef RGBPIN // Only include if RGBLED build flag is defined
      lightPixels(pixels.Color(0, 0, 255));
#endif
    }
    else
    {
      logToWebSocket("WiFi Connected!");
      // You can now use the API key stored
      logToWebSocket("API Key: " + apiKey);
    }
  }
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/stats", handleStats);
  server.on("/css/*", handleCss);
  server.on("/js/*", handleJs);
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });
  server.on("/settings", handleSettings);
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/favicon.ico", "image/x-icon"); });
  server.on("/image", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Sending image");
              request->send(SPIFFS, "/labsmall.png", "image/png"); });
  server.on("/logs", handleLogger);
  server.on("/header", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // get the header file from spiffs and process it before sending
              File file = SPIFFS.open("/header.html", "r");
              if (!file)
              {
                request->send(500, "text/plain", "File not found");
                return;
              }
              String html = processHeader(file.readString());
              file.close();
              request->send(200, "text/html", html); });
  server.on("/header.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // get the header file from spiffs and process it before sending
              File file = SPIFFS.open("/header.js", "r");
              if (!file)
              {
                request->send(500, "text/plain", "File not found");
                return;
              }
              String html = file.readString();
              file.close();
              request->send(200, "text/javascript", html); });
  server.on("/update", HTTP_GET, handleUpdater);
  server.on("/update", HTTP_POST, handleUpdate);
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "Not found"); });
  server.on("/reboot", handleReboot);
  apiKey = key;
  // // apiKey = key; // <-- Make sure to assign the retrieved key to the global apiKey

  logToWebSocket("===============\n");
  logToWebSocket("Box-opener started\n");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  Serial.println("Free heap before server begin: " + String(ESP.getFreeHeap()));

  server.begin();
  Serial.println("Free heap after server begin: " + String(ESP.getFreeHeap()));
}

//===== generate table of 50 bip39 12-word mnemonics
void generateMnemonics(String *mnemonics)
{
  for (int i = 0; i < numGuesses; i++)
  {
    std::vector<uint8_t> entropy(16); // 128-bit entropy -> 12 words
    for (size_t j = 0; j < entropy.size(); j++)
    {
      entropy[j] = esp_random() & 0xFF; // extract 1 random byte
    }
    BIP39::word_list mnemonic = BIP39::create_mnemonic(entropy);
    std::string mnemonicStr = mnemonic.to_string();
    mnemonics[i] = String(mnemonicStr.c_str());
    // logToWebSocket("Generated mnemonic: " + mnemonics[i] + "\n");
  }
}
//===== submit mnemonics to Oracle
bool submitGuesses(String *mnemonics, const String &apiUrl, const String &apiKey)
{
  bool ret = false;
  JsonDocument jsonDoc; // max size of the json output, to verify! (4kB was not enough)

  for (int i = 0; i < numGuesses; i++)
  {
    jsonDoc.add(mnemonics[i]);
  }

  logToWebSocket("🔑️ Generated %u guesses" + String(numGuesses) + "\n");
  logToWebSocket("➡️ Submitting to oracle\n");
  String apistring = "🔑️ API Key: " + String(apiKey) + "\n";
  logToWebSocket(apistring);

  logToWebSocket("webserver IP: " + WiFi.localIP().toString() + "\n");

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  HTTPClient http;
  http.begin(apiUrl + "/submit_guesses");
  http.addHeader("x-api-key", apiKey);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // increase default 5s to 15s

  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    if (httpResponseCode == 202)
    {
      logToWebSocket("✅ Guesses accepted");
      // make the pixels green
      triggerSucess();
      LastGuessAccepted = "Accepted";
      ret = false;
    }
    else if (httpResponseCode == 404) // "Closed Box Not Found"
    {
      logToWebSocket("❌ Guesses rejected :" + String(response.c_str()) + "\n");
      ret = false;
      triggerError();
      LastGuessAccepted = response.c_str();
    }
    else // other errors
    {
      logToWebSocket("❌ Guesses rejected :" + String(response.c_str()) + "\n");
      ret = true;
      triggerError();
      LastGuessAccepted = response.c_str();
    }
  }
  else // even more other errors :V maybe do a reconnect?
  {
    logToWebSocket("❌ Error in HTTP request: " + http.errorToString(httpResponseCode) + "\n");
    ret = true;
    triggerError();
    LastGuessAccepted = http.errorToString(httpResponseCode).c_str();
  }

  http.end();
  return ret;
}
void update_started()
{
  logToWebSocket("CALLBACK:  HTTP update process started");
}

void update_finished()
{
  logToWebSocket("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total)
{
  // logToWebSocket("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err)
{
  logToWebSocket("CALLBACK:  HTTP update fatal error code " + String(err) + "\n");
}
//====== main loop ====
void loop()
{
  // server.handleClient();
  //
  //--- reconnect wifi if it is not connected by some reason
  ws.cleanupClients();
  if (shouldSendUpdate())
  {
    sendStatsToClients();
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    // check the last time we made a request if its been more than sleepTime then make another request
    if (millis() - lastUpdatCheck > updateCheckInterval || lastUpdatCheck == 0)
    {

      logToWebSocket("Version: " + String(version) + "\n");
      httpUpdate.rebootOnUpdate(false); // remove automatic update
      // get the next update partition
      const esp_partition_t *next_update_partition = esp_ota_get_next_update_partition(NULL);
      httpUpdate.onStart(update_started);
      httpUpdate.onEnd(update_finished);
      // httpUpdate.onProgress(update_progress);
      httpUpdate.onError(update_error);
      // find the currently running partition

      lastUpdatCheck = millis();
      WiFiClient client;
      logToWebSocket("Checking for updates...");
      // get the json file from the server http://172.16.0.101:3000/firmware.json
      HTTPClient http;
      http.begin("http://172.16.0.101:3000/firmwarewatcher?id=n16r8");

      int httpCode = http.GET();
      if (httpCode > 0)
      {
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = http.getString();
          logToWebSocket(payload);
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, payload);
          if (error)
          {
            Serial.print(F("deserializeJson() failed: "));
            logToWebSocket(error.c_str());
            return;
          }
          const char *newvers = doc["version"];
          logToWebSocket("version: " + String(newvers) + "\n");
          logToWebSocket("current version: " + String(version) + "\n");
          // if version set copyver to it or set to VERSION
          String copyver = version;
          if (version == NULL)
          {
            copyver = version;
          }
          if (strcmp(newvers, copyver.c_str()) != 0)
          {
            logToWebSocket("New version available");
            // get the length of the update bin
            t_httpUpdate_return ret = httpUpdate.update(client, "172.16.0.101", 3000, "/firmware?id=n16r8&version=%s", newvers);
            // print the byte length of the update bin
            // get the length of the update bin
            switch (ret)
            {
            case HTTP_UPDATE_FAILED:
              logToWebSocket("HTTP_UPDATE_FAILD Error (" + String(httpUpdate.getLastError()) + "): " + httpUpdate.getLastErrorString() + "\n");
              break;
            case HTTP_UPDATE_NO_UPDATES:
              logToWebSocket("HTTP_UPDATE_NO_UPDATES");
              break;
            case HTTP_UPDATE_OK:
              logToWebSocket("HTTP_UPDATE_OK");
              // read out the otadata partition

              if (next_update_partition != NULL)
              {
                logToWebSocket("Next update partition: " + String(next_update_partition->label) + "\n");
                // serial dump the content of the next update partition

                // Set the next boot partition to the new OTA partition
                esp_err_t err = esp_ota_set_boot_partition(next_update_partition);

                if (err == ESP_OK)
                {
                  logToWebSocket("Successfully set the next boot partition!");
                  // sprint the next boot partition
                  const esp_partition_t *new_boot_partition = esp_ota_get_boot_partition();
                  logToWebSocket("new boot partition: " + String(new_boot_partition->label) + "\n");
                  logToWebSocket("Rebooting to the new partition...");
                  delay(1000); // Give time for the serial output to finish
                  preferences.putString("version", newvers);
                  esp_restart();
                }
                else
                {
                  logToWebSocket("Failed to set the next boot partition!");
                }
              }
              else
              {
                logToWebSocket("No valid OTA update partition found!");
              }

              break;
            }
          }
        }
      }
    }
    if (millis() - lastRequestTime > sleepTime)
    {
      Serial.println("API Key: " + apiKey);
      // if apikey is not set then return
      if (apiKey == "" || opener_running == false)
      {
        // logToWebSocket("API Key not set, Opener shutdown Please set the API Key");
        Serial.println("API Key not set, Opener shutdown Please set the API Key");
        opener_running = false;
      }
      Serial.println("im in the loop");
      Serial.println("opener running:" + String(opener_running));
      if (opener_running == true)
      {
        logToWebSocket("⚙️ Generating guesses...");
        generateMnemonics(mnemonics);
        // create a pointer to  the corrent api url looking at devnet to check if we are on devnet

        const char *currentApiUrl = devnet ? devUrl : apiUrl;

        bool rateLimited = submitGuesses(mnemonics, currentApiUrl, apiKey);

        if (rateLimited)
        {
          sleepTime += 10000;
        }
        else
        {
          sleepTime -= 1000;
        }

        if (sleepTime < 10000)
        {
          sleepTime = 10000;
        }
        if (sleepTime > 71000) // if sleep for more than a minute limit it to one minute
        {
          sleepTime = 71000;
        }

        logToWebSocket("waiting " + String(sleepTime / 1000) + "s for next batch...\n");
        pixels.clear();
        // save the last request time to preferences
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
          logToWebSocket("Failed to obtain time");
        }
        else
        {
          lastSubmittedTime = timeinfo;
          logToWebSocket("time: " + String(lastRequestTime));
          // log the new time
        }
        lastRequestTime = millis();
        // delay(sleepTime - DELAYVAL);
      }
    }
  }
  else
  {
    // Serial.println("im in the loop");

    dnsServer.processNextRequest();
  };
  (10);
}
