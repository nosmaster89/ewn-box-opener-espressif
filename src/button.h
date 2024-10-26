#ifdef BUTTONPIN
#ifndef BUTTON_H
#define BUTTON_H
#include <Arduino.h>
const int debounceTime = 200;
const int longPressTime = 1000;
const int doubleClickTime = 500;
long lastPressTime = millis();
#endif
#endif