#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define BAMBU_DEFAULT_AUTOSEND_TIME         60

#define NVS_NAMESPACE_API                   "api"
#define NVS_KEY_SPOOLMAN_URL                "spoolmanUrl"
#define NVS_KEY_OCTOPRINT_ENABLED           "octoEnabled"
#define NVS_KEY_OCTOPRINT_URL               "octoUrl"
#define NVS_KEY_OCTOPRINT_TOKEN             "octoToken"

#define NVS_KEY_MOONRAKER_ENABLED           "moonEnabled"
#define NVS_KEY_MOONRAKER_URL               "moonUrl"
#define NVS_KEY_MOONRAKER_API_KEY           "moonApiKey"

#define NVS_KEY_PRINTFARMER_ENABLED         "pfEnabled"
#define NVS_KEY_PRINTFARMER_URL             "pfUrl"
#define NVS_KEY_PRINTFARMER_API_KEY         "pfApiKey"
#define NVS_KEY_PRINTFARMER_PRINTER_ID      "pfPrinterId"

#define NVS_NAMESPACE_BAMBU                 "bambu"
#define NVS_KEY_BAMBU_IP                    "bambuIp"
#define NVS_KEY_BAMBU_ACCESSCODE            "bambuCode"
#define NVS_KEY_BAMBU_SERIAL                "bambuSerial"
#define NVS_KEY_BAMBU_AUTOSEND_ENABLE       "autosendEnable"
#define NVS_KEY_BAMBU_AUTOSEND_TIME         "autosendTime"

#define NVS_NAMESPACE_SCALE                 "scale"
#define NVS_KEY_CALIBRATION                 "cal_value"
#define NVS_KEY_AUTOTARE                    "auto_tare"
#define SCALE_DEFAULT_CALIBRATION_VALUE     430.0f;

// ── Pin configuration NVS ──
#define NVS_NAMESPACE_PINS                  "pins"
#define NVS_KEY_PN532_SCK                   "pn532Sck"
#define NVS_KEY_PN532_MISO                  "pn532Miso"
#define NVS_KEY_PN532_MOSI                  "pn532Mosi"
#define NVS_KEY_PN532_SS                    "pn532Ss"
#define NVS_KEY_PN532_IRQ                   "pn532Irq"
#define NVS_KEY_PN532_RESET                 "pn532Rst"

// Board name set via build flag; fallback for legacy builds
#ifndef BOARD_NAME
  #define BOARD_NAME "ESP32-WROOM-32D"
#endif

// Board-specific default pins (set via build flags in platformio.ini)
#ifndef DEFAULT_PN532_SCK
  #define DEFAULT_PN532_SCK   18
#endif
#ifndef DEFAULT_PN532_MISO
  #define DEFAULT_PN532_MISO  19
#endif
#ifndef DEFAULT_PN532_MOSI
  #define DEFAULT_PN532_MOSI  23
#endif
#ifndef DEFAULT_PN532_SS
  #define DEFAULT_PN532_SS    5
#endif
#ifndef DEFAULT_PN532_IRQ
  #define DEFAULT_PN532_IRQ   4
#endif
#ifndef DEFAULT_PN532_RESET
  #define DEFAULT_PN532_RESET 27
#endif

// PN532 pin structure
struct Pn532Pins {
  uint8_t sck;
  uint8_t miso;
  uint8_t mosi;
  uint8_t ss;
  uint8_t irq;
  uint8_t reset;
};

extern Pn532Pins pn532Pins;

void loadPinConfig();
bool savePinConfig(const Pn532Pins &pins);

#define BAMBU_USERNAME                      "bblp"

#define OLED_RESET                          -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS                      0x3CU   // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH                        128U
#define SCREEN_HEIGHT                       64U
#define SCREEN_TOP_BAR_HEIGHT               16U
#define SCREEN_PROGRESS_BAR_HEIGHT          12U
#define DISPLAY_BOOT_TEXT                   "FilaMan"

#define WIFI_CHECK_INTERVAL                 60000U
#define DISPLAY_UPDATE_INTERVAL             1000U
#define SPOOLMAN_HEALTHCHECK_INTERVAL       60000U

extern const uint8_t LOADCELL_DOUT_PIN;
extern const uint8_t LOADCELL_SCK_PIN;
extern const uint8_t calVal_eepromAdress;
extern const uint16_t SCALE_LEVEL_WEIGHT;

extern const uint8_t TTP223_PIN;

extern const uint8_t OLED_TOP_START;
extern const uint8_t OLED_TOP_END;
extern const uint8_t OLED_DATA_START;
extern const uint8_t OLED_DATA_END;

extern const char* apiUrl;
extern const uint8_t webserverPort;



extern const unsigned char wifi_on[];
extern const unsigned char wifi_off[];
extern const unsigned char cloud_on[];
extern const unsigned char cloud_off[];

extern const unsigned char icon_failed[];
extern const unsigned char icon_success[];
extern const unsigned char icon_transfer[];
extern const unsigned char icon_loading[];

extern uint8_t rfidTaskCore;
extern uint8_t rfidTaskPrio;

extern uint8_t rfidWriteTaskPrio;

extern uint8_t mqttTaskCore;
extern uint8_t mqttTaskPrio;

extern uint8_t scaleTaskCore;
extern uint8_t scaleTaskPrio;

extern uint16_t defaultScaleCalibrationValue;
#endif