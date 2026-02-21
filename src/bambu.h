#ifndef BAMBU_H
#define BAMBU_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct TrayData {
    uint8_t id;
    String tray_info_idx;
    String tray_type;
    String tray_sub_brands;
    String tray_color;
    int nozzle_temp_min;
    int nozzle_temp_max;
    String setting_id;
    String cali_idx;
};

struct BambuCredentials {
    String ip;
    String serial;
    String accesscode;
    bool autosend_enable;
    int autosend_time;
};

#define MAX_AMS 17  // 16 normal AMS + 1 external spool
extern String amsJsonData;  // For prepared JSON data

struct AMSData {
    uint8_t ams_id;
    TrayData trays[4]; // Assumption: Maximum 4 trays per AMS
};

extern bool bambu_connected;

extern int ams_count;
extern AMSData ams_data[MAX_AMS];
//extern bool autoSendToBambu;
extern uint16_t autoSetToBambuSpoolId;
extern bool bambuDisabled;
extern BambuCredentials bambuCredentials;

bool removeBambuCredentials();
bool loadBambuCredentials();
bool saveBambuCredentials(const String& bambu_ip, const String& bambu_serialnr, const String& bambu_accesscode, const bool autoSend, const String& autoSendTime);
bool setupMqtt();
void mqtt_loop(void * parameter);
bool setBambuSpool(String payload);
void bambu_restart();

extern TaskHandle_t BambuMqttTask;
#endif
