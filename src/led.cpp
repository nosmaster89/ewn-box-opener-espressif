#include <led.h>
#include <Arduino.h>

#ifdef LEDPIN
bool inverted = false;

// set pinmode to output
void setupLED()
{
#ifdef LEDINVERT
    inverted = true;
#endif
    pinMode(LEDPIN, OUTPUT);
    turnOffLED();
}
void turnOnLED()
{
    if (inverted)
    {
        digitalWrite(LEDPIN, LOW);
    }
    else
    {
        digitalWrite(LEDPIN, HIGH);
    }
}
void turnOffLED()
{
    if (inverted)
    {
        digitalWrite(LEDPIN, HIGH);
    }
    else
    {
        digitalWrite(LEDPIN, LOW);
    }
}
void blinkLED()
{
    turnOnLED();
    delay(50);
    turnOffLED();
}

#endif // LEDPIN