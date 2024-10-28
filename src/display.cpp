// #include <display.h>
#ifdef SCREEN
#include <websockets.h>
#include <param.h>
#include <TFT_eSPI.h>
#include <PNGdec.h>
#ifdef S3
#include <kitty.h>
#include <s3-display.h>
#endif

TFT_eSPI display = TFT_eSPI();
TFT_eSprite Ksprite = TFT_eSprite(&display);
TFT_eSprite TEXTsprite = TFT_eSprite(&display);
TFT_eSprite DATEsprite = TFT_eSprite(&display);

bool backlight = true;
int displayPage = 0;
int lastPage = 0;
int maxPage = 2;
int rotation = 1;

// Bodmer's BMP image rendering function
uint16_t read16(fs::File &f)
{
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}
uint32_t read32(fs::File &f)
{
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}
void drawBmp(const char *filename, int16_t x, int16_t y)
{
    Serial.println("Loading image '" + String(filename) + "'");
    if ((x >= display.width()) || (y >= display.height()))
    {

        Serial.println("Image is off screen");
        return;
    }

    fs::File bmpFS;

    // Open requested file on SD card
    bmpFS = SPIFFS.open(filename, "r");

    if (!bmpFS)
    {
        Serial.println("File not found");
        return;
    }

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b;

    uint32_t startTime = millis();

    Serial.println("trying to load bmp file");
    if (read16(bmpFS) == 0x4D42)
    {
        Serial.println("found BMP file!");
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
        {
            y += h - 1;

            bool oldSwapBytes = display.getSwapBytes();
            display.setSwapBytes(true);
            bmpFS.seek(seekOffset);

            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++)
            {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16-bit colours
                for (uint16_t col = 0; col < w; col++)
                {
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                display.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
            }
            display.setSwapBytes(oldSwapBytes);
            Serial.print("Loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        }
        else
            Serial.println("BMP format not recognized.");
    }
    Serial.println("File not loaded");
    bmpFS.close();
}

// page 1 is cat in a box
void loadCatInBox()
{
    // load in the background
    // turn off the blacklight
    digitalWrite(TFT_BL, LOW);
    drawBmp("/box.bmp", 0, 0);
    // load in the kitty from kitty.h
    // clear the sprite
    Ksprite.deleteSprite();
    Ksprite.createSprite(KITTY_WIDTH, KITTY_HEIGHT);
    Ksprite.pushImage(0, 0, KITTY_WIDTH, KITTY_HEIGHT, kitty);
    Ksprite.pushSprite(KITTY_X, KITTY_Y, TFT_BLACK);
    // turn on the backlight
    digitalWrite(TFT_BL, HIGH);
}
void DisplayCat()
{
    if (displayPage == 0)
    {
        Ksprite.deleteSprite();
        Ksprite.createSprite(KITTY_WIDTH, KITTY_HEIGHT);
        Ksprite.fillSprite(TFT_TRANSPARENT);
        Ksprite.pushImage(KITTY_X, KITTY_Y, KITTY_WIDTH, KITTY_HEIGHT, kitty);
        Ksprite.pushSprite(KITTY_X, KITTY_Y);
    }
}

// page 2 is stats
void screenStats()
{
    // wipe the screen
    display.fillScreen(TFT_BLACK);
    display.setCursor(0, 10);
    // print out the stats on a screen
    // print line connected
    display.setTextColor(TFT_WHITE);
    display.setTextSize(2);
    String text = WiFi.localIP().toString();
    display.println(text);
    display.println();
    display.println("RSSI: " + String(WiFi.RSSI()));
    display.println();
    display.println("Uptime: ");
    display.println(String(millis() / 1000 / 60 / 60) + " hours");
    display.println();
    display.println("Last Request: ");
    display.println(String(mktime(&lastSubmittedTime)));
    display.println();
    display.println("Last Request: ");
    display.println(LastGuessAccepted);
    display.println();
    display.println("DevNet: ");
    display.println(String(devnet ? "Enabled" : "Disabled"));
    display.println();
    display.println("Version: " + String(version));
}

void displaySplash()
{
    display.setRotation(1);
    // display the splash screen
    // display.fillScreen(TFT_BLACK);
    // read in labsmall.png from spiffs
    drawBmp("/labsmall.bmp", 0, 0);
    digitalWrite(TFT_BL, HIGH);
    delay(2000);
    display.setRotation(0);
    digitalWrite(TFT_BL, LOW);
}
void DisplayGfx(int code, String response)
{
    String text;

    switch (displayPage)
    {
    case 0:
        Ksprite.deleteSprite();
        Ksprite.createSprite(KITTY_WIDTH, KITTY_HEIGHT);
        Ksprite.pushImage(0, 0, KITTY_WIDTH, KITTY_HEIGHT, kitty);
        Ksprite.pushSprite(KITTY_X, KITTY_Y, TFT_BLACK);
        DATEsprite.deleteSprite();
        DATEsprite.createSprite(DATE_WIDTH, DATE_HEIGHT);
        DATEsprite.pushSprite(DATE_X, DATE_Y);
        DATEsprite.setTextColor(TFT_WHITE);
        DATEsprite.deleteSprite();
        DATEsprite.createSprite(DATE_WIDTH, DATE_HEIGHT);
        DATEsprite.setTextSize(1);
        DATEsprite.setTextColor(TFT_GREEN);
        // get the width of the text
        int16_t x1, y1;
        uint16_t w, h;
        // try to get NTP time
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            logToWebSocket("Failed to obtain time");
        }
        else
        {
            logToWebSocket("Time: " + String(asctime(&timeinfo)));
            // get the time
            // turn the time into a short string dd/mm/yyyy hh:mm:ss
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

            int x = display.textWidth(timeStr, 1); // Get the width of the text
            logToWebSocket("Time width: " + String(x));
            int h2 = display.fontHeight(1); // Get the height of the font
            logToWebSocket("Font height: " + String(h2));
            logToWebSocket("cursor: " + String((DATE_WIDTH - x) / 2) + " " + String((DATE_HEIGHT - h2) / 2));

            DATEsprite.setCursor((DATE_WIDTH - x) / 2, (DATE_HEIGHT - h2) / 2);
            DATEsprite.println(timeStr);
            DATEsprite.pushSprite(DATE_X, DATE_Y, TFT_BLACK);
        }

        // TEXTsprite.fillSprite(TFT_BLACK);
        // TEXTsprite.pushSprite(TXT_B_X, TXT_B_Y, TFT_BLACK);
        // TEXTsprite.deleteSprite();

        TEXTsprite.deleteSprite();
        TEXTsprite.createSprite(TXT_B_WIDTH, TXT_B_HEIGHT);
        // TEXTsprite.setTextColor(TFT_WHITE, TFT_BLACK, TFT_TRANSPARENT);
        // TEXTsprite.fillSprite(TFT_BLACK);
        TEXTsprite.pushSprite(TXT_B_X, TXT_B_Y);
        TEXTsprite.setTextSize(1);
        // create 2 random words string 1 and string 2
        TEXTsprite.setTextColor(TFT_RED);
        switch (code)
        {
        case 202:
            TEXTsprite.setTextColor(TFT_GREEN);
            text = "Accepted";
            break;
        case 404:
            text = "Box Opened";
            break;
        case 401:
            text = "Unauthorized";
            break;
        case 429:
            text = "Rate Limited";
            break;
        case 500:
            text = "Internal Server Error";
            break;
        default:
            text = "Unknown";
            break;
        }

        if (display.textWidth(text.c_str(), 1) > TXT_B_WIDTH)
        {
            TEXTsprite.setCursor(0, (TXT_B_HEIGHT - display.fontHeight(1)) / 2);
        }
        else
        {

            TEXTsprite.setCursor((TXT_B_WIDTH - display.textWidth(text.c_str(), 1)) / 2, (TXT_B_HEIGHT - display.fontHeight(1)) / 2);
        }
        TEXTsprite.println(text);

        // push the sprite to the screen
        TEXTsprite.pushSprite(TXT_B_X, TXT_B_Y, TFT_BLACK);
        break;
    case 1:
        // display the stats
        screenStats();
        break;
    default:
        break;
    }
    // is display != 0
    if (displayPage != 0)
    {
        return;
    }
    // load in the kitty from kitty.h
}
void displayInit()
{
    Serial.println("init Screen Setup");
    display.init();
    digitalWrite(TFT_BL, LOW);
    display.flush();
    display.fillScreen(TFT_BLACK);

    Serial.println("set sprite bytes");

    // TEXTsprite.init();
    // DATEsprite.init();

    // delay(3000);
    Ksprite.setSwapBytes(true);
    TEXTsprite.setSwapBytes(true);
    DATEsprite.setSwapBytes(true);
    // display.writecommand(ST7789_DISPON); // turn on lcd display
    logToWebSocket("finished Display setup");
}
#endif