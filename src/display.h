#include <TFT_eSPI.h>
#include <PNGdec.h>
// ifdef BOARD =S3
#ifdef S3
#include <s3-display.h>
#endif
#ifndef DISPLAY_H
#define DISPLAY_H
// define the display
extern PNG png;
extern TFT_eSPI display;
extern TFT_eSprite Ksprite;
extern TFT_eSprite TEXTsprite;
extern TFT_eSprite DATEsprite;

extern bool backlight;
extern int displayPage;
extern int lastPage;
extern int maxPage;
extern int rotation;
void DisplayGfx(int code, String response);
void drawBmp(const char *filename, int16_t x, int16_t y);
void loadCatInBox();
void screenStats();
void displaySplash();
void displayInit();

#endif