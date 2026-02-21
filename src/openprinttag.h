#ifndef OPENPRINTTAG_H
#define OPENPRINTTAG_H

#include <Arduino.h>
#include <ArduinoJson.h>

// OpenPrintTag field keys (from main_fields.yaml)
enum OPTFieldKey : uint8_t {
    OPT_INSTANCE_UUID           = 0,
    OPT_PACKAGE_UUID            = 1,
    OPT_MATERIAL_UUID           = 2,
    OPT_BRAND_UUID              = 3,
    OPT_GTIN                    = 4,
    OPT_BRAND_SPECIFIC_INSTANCE = 5,
    OPT_BRAND_SPECIFIC_PACKAGE  = 6,
    OPT_BRAND_SPECIFIC_MATERIAL = 7,
    OPT_MATERIAL_CLASS          = 8,
    OPT_MATERIAL_TYPE           = 9,
    OPT_MATERIAL_NAME           = 10,
    OPT_BRAND_NAME              = 11,
    OPT_WRITE_PROTECTION        = 13,
    OPT_MANUFACTURED_DATE       = 14,
    OPT_EXPIRATION_DATE         = 15,
    OPT_NOMINAL_NETTO_WEIGHT    = 16,
    OPT_ACTUAL_NETTO_WEIGHT     = 17,
    OPT_EMPTY_CONTAINER_WEIGHT  = 18,
    OPT_PRIMARY_COLOR           = 19,
    OPT_SECONDARY_COLOR_0       = 20,
    OPT_TRANSMISSION_DISTANCE   = 27,
    OPT_TAGS                    = 28,
    OPT_DENSITY                 = 29,
    OPT_FILAMENT_DIAMETER       = 30,
    OPT_MIN_NOZZLE_DIAMETER     = 33,
    OPT_MIN_PRINT_TEMP          = 34,
    OPT_MAX_PRINT_TEMP          = 35,
    OPT_PREHEAT_TEMP            = 36,
    OPT_MIN_BED_TEMP            = 37,
    OPT_MAX_BED_TEMP            = 38,
    OPT_MATERIAL_ABBREVIATION   = 52,
    OPT_NOMINAL_FULL_LENGTH     = 53,
    OPT_ACTUAL_FULL_LENGTH      = 54,
    OPT_COUNTRY_OF_ORIGIN       = 55,
    OPT_CERTIFICATIONS          = 56,
    OPT_DRYING_TEMPERATURE      = 57,
    OPT_DRYING_TIME             = 58,
};

// OpenPrintTag auxiliary field keys (from aux_fields.yaml)
enum OPTAuxFieldKey : uint8_t {
    OPT_AUX_CONSUMED_WEIGHT     = 0,
    OPT_AUX_WORKGROUP           = 1,
    OPT_AUX_GP_RANGE_USER       = 2,
};

// Material class enum values
enum OPTMaterialClass : uint8_t {
    OPT_MC_FFF = 0,
    OPT_MC_SLA = 1,
};

// Common FFF material types (subset from material_type_enum.yaml)
// Full list has 100+ entries; we map the most common ones
enum OPTMaterialType : uint8_t {
    OPT_MT_PLA      = 0,
    OPT_MT_PETG     = 1,
    OPT_MT_ABS      = 2,
    OPT_MT_ASA      = 3,
    OPT_MT_PA       = 4,   // Nylon
    OPT_MT_PC       = 5,
    OPT_MT_TPU      = 6,
    OPT_MT_PVA      = 7,
    OPT_MT_HIPS     = 8,
    OPT_MT_PP       = 9,
    OPT_MT_PET      = 10,
    OPT_MT_PEEK     = 11,
    OPT_MT_PEI      = 12,
    OPT_MT_PA6      = 13,
    OPT_MT_PA12     = 14,
    OPT_MT_PCCF     = 15,  // PC Carbon Fiber
    OPT_MT_PACF     = 16,  // PA Carbon Fiber
    OPT_MT_PETGCF   = 17,  // PETG Carbon Fiber
    OPT_MT_PLACF    = 18,  // PLA Carbon Fiber
    OPT_MT_UNKNOWN  = 255,
};

// Parsed OpenPrintTag data structure
struct OpenPrintTagData {
    // Identity
    String instanceUuid;
    String brandUuid;
    uint64_t gtin;

    // Material info
    OPTMaterialClass materialClass;
    OPTMaterialType materialType;
    String materialName;        // max 31 chars
    String materialAbbreviation; // max 7 chars
    String brandName;           // max 31 chars

    // Weights (grams)
    float nominalNettoWeight;
    float actualNettoWeight;
    float emptyContainerWeight;

    // Length (mm)
    float nominalFullLength;
    float actualFullLength;

    // Colors (RGBA, 3 or 4 bytes)
    uint32_t primaryColor;     // 0xRRGGBBAA
    bool hasPrimaryColor;

    // Temperatures (°C)
    int16_t minPrintTemp;
    int16_t maxPrintTemp;
    int16_t preheatTemp;
    int16_t minBedTemp;
    int16_t maxBedTemp;

    // Physical properties
    float density;              // g/cm³
    float filamentDiameter;     // mm, default 1.75

    // Drying
    int16_t dryingTemperature;  // °C
    int16_t dryingTime;         // minutes

    // Auxiliary (writable section)
    float consumedWeight;       // grams

    // Parse status
    bool valid;
    String parseError;

    OpenPrintTagData() :
        gtin(0),
        materialClass(OPT_MC_FFF),
        materialType(OPT_MT_UNKNOWN),
        nominalNettoWeight(0), actualNettoWeight(0), emptyContainerWeight(0),
        nominalFullLength(0), actualFullLength(0),
        primaryColor(0), hasPrimaryColor(false),
        minPrintTemp(-1), maxPrintTemp(-1), preheatTemp(-1),
        minBedTemp(-1), maxBedTemp(-1),
        density(0), filamentDiameter(1.75f),
        dryingTemperature(-1), dryingTime(-1),
        consumedWeight(0),
        valid(false) {}
};

// NFC tag format detection result
enum NfcTagFormat {
    TAG_FORMAT_UNKNOWN,
    TAG_FORMAT_OPENSPOOL,      // JSON NDEF with "protocol":"openspool"
    TAG_FORMAT_OPENPRINTTAG,   // Binary TLV NDEF (OpenPrintTag)
    TAG_FORMAT_RAW_SPOOL_ID,   // Raw ASCII digits on pages 5-8 (Tool-Aware-NFC-Reader)
};

// Detect NFC tag format from NDEF payload
NfcTagFormat detectTagFormat(const uint8_t* ndefPayload, uint16_t payloadLength, uint8_t typeLength, const uint8_t* recordType);

// Parse OpenPrintTag binary TLV data from NDEF payload
bool parseOpenPrintTag(const uint8_t* payload, uint16_t payloadLength, OpenPrintTagData& data);

// Convert OpenPrintTagData to JSON (for web UI display and Spoolman mapping)
String openPrintTagToJson(const OpenPrintTagData& data);

// Convert OpenPrintTagData to OpenSpool-compatible JSON
String openPrintTagToOpenSpoolJson(const OpenPrintTagData& data);

// Map OpenPrintTag material type enum to string name
const char* optMaterialTypeToString(OPTMaterialType type);

// Map string material name to OpenPrintTag material type enum
OPTMaterialType stringToOptMaterialType(const char* name);

// Convert primary color (RGBA uint32) to hex string (e.g., "FF5733")
String optColorToHexString(uint32_t color);

// Convert hex color string to RGBA uint32
uint32_t hexStringToOptColor(const char* hex);

// Encode OpenPrintTagData to binary TLV for writing to tag
// Returns allocated buffer (caller must free) and sets bufferLength
uint8_t* encodeOpenPrintTag(const OpenPrintTagData& data, uint16_t& bufferLength);

// Build complete NDEF message with OpenPrintTag payload for writing
// Returns allocated buffer (caller must free) and sets bufferLength
uint8_t* buildOpenPrintTagNdefMessage(const OpenPrintTagData& data, uint16_t& bufferLength);

#endif
