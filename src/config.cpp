#include "config.h"
#include <Preferences.h>

// ################### Config Bereich Start
// ***** PN532 (RFID) – runtime-configurable, see loadPinConfig()
Pn532Pins pn532Pins = {
  DEFAULT_PN532_SCK,
  DEFAULT_PN532_MISO,
  DEFAULT_PN532_MOSI,
  DEFAULT_PN532_SS,
  DEFAULT_PN532_IRQ,
  DEFAULT_PN532_RESET
};
// ***** PN532

// ***** HX711 (Waage)
// HX711 circuit wiring
const uint8_t LOADCELL_DOUT_PIN = 16; //16;
const uint8_t LOADCELL_SCK_PIN = 17; //17;
const uint8_t calVal_eepromAdress = 0;
const uint16_t SCALE_LEVEL_WEIGHT = 500;
// ***** HX711

// ***** TTP223 (Touch Sensor)
// TTP223 circuit wiring
const uint8_t TTP223_PIN = 25;
// ***** TTP223


// ***** Display
const uint8_t OLED_TOP_START = 0;
const uint8_t OLED_TOP_END = 16;
const uint8_t OLED_DATA_START = 17;
const uint8_t OLED_DATA_END = SCREEN_HEIGHT;

// ***** Display

// ***** Webserver
const uint8_t webserverPort = 80;
// ***** Webserver

// ***** API
const char* apiUrl = "/api/v1";
// ***** API

// ***** Bambu Auto Set Spool

// ***** Task Prios
// Single-core chips (ESP32-C3, C6) must pin everything to core 0
#if CONFIG_FREERTOS_UNICORE
uint8_t rfidTaskCore = 0;
#else
uint8_t rfidTaskCore = 1;
#endif
uint8_t rfidTaskPrio = 1;

uint8_t rfidWriteTaskPrio = 1;

#if CONFIG_FREERTOS_UNICORE
uint8_t mqttTaskCore = 0;
#else
uint8_t mqttTaskCore = 1;
#endif
uint8_t mqttTaskPrio = 1;

uint8_t scaleTaskCore = 0;
uint8_t scaleTaskPrio = 1;
// ***** Task Prios

// ── Pin configuration persistence ──
void loadPinConfig() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE_PINS, true); // read-only
  pn532Pins.sck   = prefs.getUChar(NVS_KEY_PN532_SCK,   DEFAULT_PN532_SCK);
  pn532Pins.miso  = prefs.getUChar(NVS_KEY_PN532_MISO,  DEFAULT_PN532_MISO);
  pn532Pins.mosi  = prefs.getUChar(NVS_KEY_PN532_MOSI,  DEFAULT_PN532_MOSI);
  pn532Pins.ss    = prefs.getUChar(NVS_KEY_PN532_SS,     DEFAULT_PN532_SS);
  pn532Pins.irq   = prefs.getUChar(NVS_KEY_PN532_IRQ,   DEFAULT_PN532_IRQ);
  pn532Pins.reset = prefs.getUChar(NVS_KEY_PN532_RESET,  DEFAULT_PN532_RESET);
  prefs.end();

  Serial.printf("PN532 pins: SCK=%u MISO=%u MOSI=%u SS=%u IRQ=%u RST=%u\n",
    pn532Pins.sck, pn532Pins.miso, pn532Pins.mosi,
    pn532Pins.ss, pn532Pins.irq, pn532Pins.reset);
}

bool savePinConfig(const Pn532Pins &pins) {
  // Basic validation: all six pins must be different
  uint8_t arr[6] = { pins.sck, pins.miso, pins.mosi, pins.ss, pins.irq, pins.reset };
  for (int i = 0; i < 6; i++) {
    for (int j = i + 1; j < 6; j++) {
      if (arr[i] == arr[j]) return false;
    }
  }

  Preferences prefs;
  prefs.begin(NVS_NAMESPACE_PINS, false);
  prefs.putUChar(NVS_KEY_PN532_SCK,   pins.sck);
  prefs.putUChar(NVS_KEY_PN532_MISO,  pins.miso);
  prefs.putUChar(NVS_KEY_PN532_MOSI,  pins.mosi);
  prefs.putUChar(NVS_KEY_PN532_SS,    pins.ss);
  prefs.putUChar(NVS_KEY_PN532_IRQ,   pins.irq);
  prefs.putUChar(NVS_KEY_PN532_RESET, pins.reset);
  prefs.end();

  pn532Pins = pins;
  return true;
}
