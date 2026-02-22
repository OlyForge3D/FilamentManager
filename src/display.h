#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "config.h"

#ifndef DISABLE_DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;
#endif

extern bool wifiOn;

void setupDisplay();
void oledclearline();
void oledcleardata();
int oled_center_h(const String &text);
int oled_center_v(const String &text);

void oledShowProgressBar(const uint8_t step, const uint8_t numSteps, const char* largeText, const char* statusMessage);

void oledShowWeight(uint16_t weight);
void oledShowMessage(const String &message, uint8_t size = 2);
void oledShowTopRow();
void oledShowIcon(const char* icon);

#endif
