#include "rgbled.h"

#ifdef RGBPIN // Only include if RGBLED build flag is defined
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, RGBPIN, NEO_GRB + NEO_KHZ800);

void setupRGBLED()
{
    strip.begin(); // Initialize the NeoPixel strip
    strip.show();  // Turn off all LEDs initially
}

void updateRGBLED()
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 150, 0)); // Set LED to green as an example
    }
    strip.show(); // Apply the color changes
}
void lightPixels(int color)
{
    for (int i = 0; i < 1; i++)
    {
        strip.setPixelColor(i, color);
    }
    strip.show();
}
void blinkPixels(int color)

{
    for (int i = 0; i < 1; i++)
    {
        strip.setPixelColor(i, color);
        strip.show();
        delay(500);
        strip.clear();
        strip.show();
    }
}

#endif // RGBLED
