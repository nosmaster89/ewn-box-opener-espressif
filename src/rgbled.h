#ifndef RGB_LED_H
#define RGB_LED_H

#ifdef RGBPIN // Only include if RGBLED build flag is defined

#include <Adafruit_NeoPixel.h> // Include NeoPixel library

void setupRGBLED();
void updateRGBLED();
void lightPixels(int color);
void blinkPixels(int color);

#endif // RGBLED
#endif // RGB_LED_H