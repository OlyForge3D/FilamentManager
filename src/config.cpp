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
static bool isValidPinConfig(const Pn532Pins &pins) {
  uint8_t arr[6] = { pins.sck, pins.miso, pins.mosi, pins.ss, pins.irq, pins.reset };
  for (int i = 0; i < 6; i++) {
    if (arr[i] > 48) return false;
  }
  for (int i = 0; i < 6; i++) {
    for (int j = i + 1; j < 6; j++) {
      if (arr[i] == arr[j]) return false;
    }
  }
  return true;
}

void loadPinConfig() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE_PINS, true); // read-only
  Pn532Pins loadedPins = {
    prefs.getUChar(NVS_KEY_PN532_SCK,   DEFAULT_PN532_SCK),
    prefs.getUChar(NVS_KEY_PN532_MISO,  DEFAULT_PN532_MISO),
    prefs.getUChar(NVS_KEY_PN532_MOSI,  DEFAULT_PN532_MOSI),
    prefs.getUChar(NVS_KEY_PN532_SS,    DEFAULT_PN532_SS),
    prefs.getUChar(NVS_KEY_PN532_IRQ,   DEFAULT_PN532_IRQ),
    prefs.getUChar(NVS_KEY_PN532_RESET, DEFAULT_PN532_RESET)
  };
  prefs.end();

  if (!isValidPinConfig(loadedPins)) {
    Serial.printf("Invalid PN532 pin config in NVS (SCK=%u MISO=%u MOSI=%u SS=%u IRQ=%u RST=%u). Using defaults.\n",
      loadedPins.sck, loadedPins.miso, loadedPins.mosi,
      loadedPins.ss, loadedPins.irq, loadedPins.reset);
    pn532Pins = {
      DEFAULT_PN532_SCK,
      DEFAULT_PN532_MISO,
      DEFAULT_PN532_MOSI,
      DEFAULT_PN532_SS,
      DEFAULT_PN532_IRQ,
      DEFAULT_PN532_RESET
    };
  } else {
    pn532Pins = loadedPins;
  }

  Serial.printf("PN532 pins: SCK=%u MISO=%u MOSI=%u SS=%u IRQ=%u RST=%u\n",
    pn532Pins.sck, pn532Pins.miso, pn532Pins.mosi,
    pn532Pins.ss, pn532Pins.irq, pn532Pins.reset);
}

bool savePinConfig(const Pn532Pins &pins) {
  if (!isValidPinConfig(pins)) return false;

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
