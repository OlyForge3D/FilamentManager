#include "openprinttag.h"

// ============================================================================
// OpenPrintTag Binary TLV Parser/Encoder
//
// Implements the OpenPrintTag specification for NFC spool metadata.
// Binary TLV format: each field is [key(1)][length(1-2)][value(N)]
// See: https://github.com/OpenPrintTag/openprinttag-specification
// ============================================================================

// --- Material type name mapping ---

struct MaterialTypeEntry {
    OPTMaterialType type;
    const char* name;
};

static const MaterialTypeEntry materialTypeMap[] = {
    { OPT_MT_PLA,     "PLA" },
    { OPT_MT_PETG,    "PETG" },
    { OPT_MT_ABS,     "ABS" },
    { OPT_MT_ASA,     "ASA" },
    { OPT_MT_PA,      "PA" },
    { OPT_MT_PC,      "PC" },
    { OPT_MT_TPU,     "TPU" },
    { OPT_MT_PVA,     "PVA" },
    { OPT_MT_HIPS,    "HIPS" },
    { OPT_MT_PP,      "PP" },
    { OPT_MT_PET,     "PET" },
    { OPT_MT_PEEK,    "PEEK" },
    { OPT_MT_PEI,     "PEI" },
    { OPT_MT_PA6,     "PA6" },
    { OPT_MT_PA12,    "PA12" },
    { OPT_MT_PCCF,    "PC-CF" },
    { OPT_MT_PACF,    "PA-CF" },
    { OPT_MT_PETGCF,  "PETG-CF" },
    { OPT_MT_PLACF,   "PLA-CF" },
};

const char* optMaterialTypeToString(OPTMaterialType type) {
    for (const auto& entry : materialTypeMap) {
        if (entry.type == type) return entry.name;
    }
    return "Unknown";
}

OPTMaterialType stringToOptMaterialType(const char* name) {
    if (!name) return OPT_MT_UNKNOWN;
    String upper = String(name);
    upper.toUpperCase();
    upper.replace(" ", "");
    upper.replace("_", "-");

    for (const auto& entry : materialTypeMap) {
        if (upper == entry.name) return entry.type;
    }

    // Fuzzy matching for common variants
    if (upper.startsWith("PLA")) return OPT_MT_PLA;
    if (upper.startsWith("PETG")) return OPT_MT_PETG;
    if (upper.startsWith("ABS")) return OPT_MT_ABS;
    if (upper.startsWith("ASA")) return OPT_MT_ASA;
    if (upper.startsWith("TPU") || upper.startsWith("FLEX")) return OPT_MT_TPU;
    if (upper.startsWith("NYLON") || upper.startsWith("PA")) return OPT_MT_PA;
    if (upper.startsWith("PC")) return OPT_MT_PC;

    return OPT_MT_UNKNOWN;
}

String optColorToHexString(uint32_t color) {
    char hex[7];
    snprintf(hex, sizeof(hex), "%02X%02X%02X",
             (uint8_t)((color >> 24) & 0xFF),
             (uint8_t)((color >> 16) & 0xFF),
             (uint8_t)((color >> 8) & 0xFF));
    return String(hex);
}

uint32_t hexStringToOptColor(const char* hex) {
    if (!hex) return 0;
    // Skip leading '#' if present
    if (hex[0] == '#') hex++;

    uint32_t val = strtoul(hex, nullptr, 16);
    size_t len = strlen(hex);
    if (len <= 6) {
        // RGB -> RGBA with full opacity
        return (val << 8) | 0xFF;
    }
    return val;
}

// --- TLV Helper: read a variable-length integer ---

static uint16_t readTlvLength(const uint8_t* buf, uint16_t offset, uint16_t maxLen, uint16_t& bytesConsumed) {
    if (offset >= maxLen) { bytesConsumed = 0; return 0; }
    uint8_t first = buf[offset];
    if (first < 0x80) {
        bytesConsumed = 1;
        return first;
    }
    // Extended length: first byte is 0x80 | numLenBytes
    uint8_t numBytes = first & 0x7F;
    if (numBytes == 0 || numBytes > 2 || offset + 1 + numBytes > maxLen) {
        bytesConsumed = 0;
        return 0;
    }
    uint16_t length = 0;
    for (uint8_t i = 0; i < numBytes; i++) {
        length = (length << 8) | buf[offset + 1 + i];
    }
    bytesConsumed = 1 + numBytes;
    return length;
}

// Read a UUID (16 bytes) as a formatted string
static String readUuid(const uint8_t* data, uint16_t len) {
    if (len < 16) return "";
    char buf[37];
    snprintf(buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        data[0], data[1], data[2], data[3],
        data[4], data[5], data[6], data[7],
        data[8], data[9], data[10], data[11],
        data[12], data[13], data[14], data[15]);
    return String(buf);
}

// Read a 16-bit unsigned integer (big-endian)
static uint16_t readUint16BE(const uint8_t* data) {
    return (data[0] << 8) | data[1];
}

// Read a 32-bit unsigned integer (big-endian)
static uint32_t readUint32BE(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

// Read a float encoded as fixed-point or IEEE 754
static float readFixedFloat(const uint8_t* data, uint16_t len) {
    if (len == 2) {
        // 16-bit: treat as integer value (weight in grams, temp in °C)
        return (float)readUint16BE(data);
    } else if (len == 4) {
        // 32-bit IEEE 754 float
        uint32_t raw = readUint32BE(data);
        float result;
        memcpy(&result, &raw, sizeof(float));
        return result;
    }
    return 0.0f;
}

// --- Format Detection ---

NfcTagFormat detectTagFormat(const uint8_t* ndefPayload, uint16_t payloadLength,
                             uint8_t typeLength, const uint8_t* recordType) {
    if (!ndefPayload || payloadLength == 0) return TAG_FORMAT_UNKNOWN;

    // Check NDEF record type for JSON MIME type
    if (typeLength > 0 && recordType) {
        String type = "";
        for (uint8_t i = 0; i < typeLength; i++) {
            type += (char)recordType[i];
        }

        // OpenSpool uses "application/json" MIME type
        if (type == "application/json") {
            // Quick check for openspool protocol field
            if (payloadLength >= 20) {
                String preview = "";
                for (uint16_t i = 0; i < min((uint16_t)100, payloadLength); i++) {
                    if (ndefPayload[i] >= 32 && ndefPayload[i] <= 126) {
                        preview += (char)ndefPayload[i];
                    }
                }
                if (preview.indexOf("\"protocol\"") >= 0 &&
                    preview.indexOf("openspool") >= 0) {
                    return TAG_FORMAT_OPENSPOOL;
                }
            }
            // JSON but not openspool — could still be FilaMan format
            return TAG_FORMAT_OPENSPOOL;
        }

        // OpenPrintTag uses a specific MIME or external type
        // The spec may use "application/vnd.openprinttag" or similar
        if (type.indexOf("openprinttag") >= 0 || type.indexOf("opt") >= 0) {
            return TAG_FORMAT_OPENPRINTTAG;
        }
    }

    // Heuristic: check if payload looks like binary TLV (OpenPrintTag)
    // OpenPrintTag TLV: field key byte followed by length, then data
    // If the first few fields have valid key+length patterns, it's likely OpenPrintTag
    if (payloadLength >= 4) {
        uint8_t key0 = ndefPayload[0];
        // Valid main field keys are 0-58
        if (key0 <= 58) {
            uint16_t bytesConsumed = 0;
            uint16_t len0 = readTlvLength(ndefPayload, 1, payloadLength, bytesConsumed);
            if (bytesConsumed > 0 && len0 > 0 && (1 + bytesConsumed + len0) <= payloadLength) {
                // First field looks valid. Check if second field also looks valid.
                uint16_t nextOffset = 1 + bytesConsumed + len0;
                if (nextOffset < payloadLength) {
                    uint8_t key1 = ndefPayload[nextOffset];
                    if (key1 <= 58 && key1 != key0) {
                        return TAG_FORMAT_OPENPRINTTAG;
                    }
                }
                // Single valid field — still likely OpenPrintTag if not JSON
                if (ndefPayload[0] != '{') {
                    return TAG_FORMAT_OPENPRINTTAG;
                }
            }
        }
    }

    // Check for raw spool ID (ASCII digits on raw pages, no NDEF structure)
    bool allDigits = true;
    for (uint16_t i = 0; i < payloadLength && i < 16; i++) {
        if (ndefPayload[i] == 0x00) break;
        if (ndefPayload[i] < '0' || ndefPayload[i] > '9') {
            allDigits = false;
            break;
        }
    }
    if (allDigits && payloadLength > 0 && ndefPayload[0] >= '0' && ndefPayload[0] <= '9') {
        return TAG_FORMAT_RAW_SPOOL_ID;
    }

    return TAG_FORMAT_UNKNOWN;
}

// --- TLV Parser ---

bool parseOpenPrintTag(const uint8_t* payload, uint16_t payloadLength, OpenPrintTagData& data) {
    if (!payload || payloadLength < 3) {
        data.parseError = "Payload too short";
        return false;
    }

    data.valid = false;
    uint16_t offset = 0;

    while (offset < payloadLength) {
        // Read field key
        uint8_t key = payload[offset];
        offset++;

        if (offset >= payloadLength) break;

        // Read field length
        uint16_t bytesConsumed = 0;
        uint16_t fieldLen = readTlvLength(payload, offset, payloadLength, bytesConsumed);
        if (bytesConsumed == 0) {
            data.parseError = "Invalid TLV length at offset " + String(offset);
            return false;
        }
        offset += bytesConsumed;

        if (offset + fieldLen > payloadLength) {
            data.parseError = "Field extends beyond payload at key " + String(key);
            return false;
        }

        const uint8_t* fieldData = &payload[offset];

        switch (key) {
            case OPT_INSTANCE_UUID:
                data.instanceUuid = readUuid(fieldData, fieldLen);
                break;

            case OPT_BRAND_UUID:
                data.brandUuid = readUuid(fieldData, fieldLen);
                break;

            case OPT_GTIN:
                if (fieldLen == 8) {
                    data.gtin = ((uint64_t)readUint32BE(fieldData) << 32) |
                                 readUint32BE(fieldData + 4);
                } else if (fieldLen == 4) {
                    data.gtin = readUint32BE(fieldData);
                }
                break;

            case OPT_MATERIAL_CLASS:
                if (fieldLen >= 1) {
                    data.materialClass = (OPTMaterialClass)fieldData[0];
                }
                break;

            case OPT_MATERIAL_TYPE:
                if (fieldLen >= 1) {
                    data.materialType = (OPTMaterialType)fieldData[0];
                }
                break;

            case OPT_MATERIAL_NAME:
                data.materialName = "";
                for (uint16_t i = 0; i < fieldLen && i < 31; i++) {
                    if (fieldData[i] == 0) break;
                    data.materialName += (char)fieldData[i];
                }
                break;

            case OPT_BRAND_NAME:
                data.brandName = "";
                for (uint16_t i = 0; i < fieldLen && i < 31; i++) {
                    if (fieldData[i] == 0) break;
                    data.brandName += (char)fieldData[i];
                }
                break;

            case OPT_MATERIAL_ABBREVIATION:
                data.materialAbbreviation = "";
                for (uint16_t i = 0; i < fieldLen && i < 7; i++) {
                    if (fieldData[i] == 0) break;
                    data.materialAbbreviation += (char)fieldData[i];
                }
                break;

            case OPT_NOMINAL_NETTO_WEIGHT:
                data.nominalNettoWeight = readFixedFloat(fieldData, fieldLen);
                break;

            case OPT_ACTUAL_NETTO_WEIGHT:
                data.actualNettoWeight = readFixedFloat(fieldData, fieldLen);
                break;

            case OPT_EMPTY_CONTAINER_WEIGHT:
                data.emptyContainerWeight = readFixedFloat(fieldData, fieldLen);
                break;

            case OPT_NOMINAL_FULL_LENGTH:
                data.nominalFullLength = readFixedFloat(fieldData, fieldLen);
                break;

            case OPT_ACTUAL_FULL_LENGTH:
                data.actualFullLength = readFixedFloat(fieldData, fieldLen);
                break;

            case OPT_PRIMARY_COLOR:
                if (fieldLen >= 3) {
                    data.primaryColor = ((uint32_t)fieldData[0] << 24) |
                                        ((uint32_t)fieldData[1] << 16) |
                                        ((uint32_t)fieldData[2] << 8);
                    if (fieldLen >= 4) {
                        data.primaryColor |= fieldData[3];
                    } else {
                        data.primaryColor |= 0xFF; // full opacity
                    }
                    data.hasPrimaryColor = true;
                }
                break;

            case OPT_MIN_PRINT_TEMP:
                data.minPrintTemp = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            case OPT_MAX_PRINT_TEMP:
                data.maxPrintTemp = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            case OPT_PREHEAT_TEMP:
                data.preheatTemp = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            case OPT_MIN_BED_TEMP:
                data.minBedTemp = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            case OPT_MAX_BED_TEMP:
                data.maxBedTemp = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            case OPT_DENSITY:
                data.density = readFixedFloat(fieldData, fieldLen);
                if (data.density > 100.0f) {
                    // Likely encoded as fixed-point * 100
                    data.density /= 100.0f;
                }
                break;

            case OPT_FILAMENT_DIAMETER:
                data.filamentDiameter = readFixedFloat(fieldData, fieldLen);
                if (data.filamentDiameter > 100.0f) {
                    data.filamentDiameter /= 100.0f;
                }
                break;

            case OPT_DRYING_TEMPERATURE:
                data.dryingTemperature = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            case OPT_DRYING_TIME:
                data.dryingTime = (fieldLen >= 2) ? (int16_t)readUint16BE(fieldData) : fieldData[0];
                break;

            default:
                // Skip unknown fields
                Serial.printf("OpenPrintTag: Skipping unknown field key=%d len=%d\n", key, fieldLen);
                break;
        }

        offset += fieldLen;
    }

    data.valid = true;
    return true;
}

// --- JSON Conversion ---

String openPrintTagToJson(const OpenPrintTagData& data) {
    JsonDocument doc;

    doc["format"] = "openprinttag";
    if (data.instanceUuid.length() > 0) doc["instance_uuid"] = data.instanceUuid;
    if (data.brandUuid.length() > 0) doc["brand_uuid"] = data.brandUuid;
    if (data.gtin > 0) doc["gtin"] = data.gtin;

    doc["material_class"] = (data.materialClass == OPT_MC_FFF) ? "FFF" : "SLA";
    doc["material_type"] = optMaterialTypeToString(data.materialType);
    if (data.materialName.length() > 0) doc["material_name"] = data.materialName;
    if (data.materialAbbreviation.length() > 0) doc["material_abbreviation"] = data.materialAbbreviation;
    if (data.brandName.length() > 0) doc["brand_name"] = data.brandName;

    if (data.nominalNettoWeight > 0) doc["nominal_weight_g"] = data.nominalNettoWeight;
    if (data.actualNettoWeight > 0) doc["actual_weight_g"] = data.actualNettoWeight;
    if (data.emptyContainerWeight > 0) doc["spool_weight_g"] = data.emptyContainerWeight;

    if (data.nominalFullLength > 0) doc["nominal_length_mm"] = data.nominalFullLength;
    if (data.actualFullLength > 0) doc["actual_length_mm"] = data.actualFullLength;

    if (data.hasPrimaryColor) {
        doc["color_hex"] = optColorToHexString(data.primaryColor);
    }

    if (data.minPrintTemp >= 0) doc["min_print_temp"] = data.minPrintTemp;
    if (data.maxPrintTemp >= 0) doc["max_print_temp"] = data.maxPrintTemp;
    if (data.preheatTemp >= 0) doc["preheat_temp"] = data.preheatTemp;
    if (data.minBedTemp >= 0) doc["min_bed_temp"] = data.minBedTemp;
    if (data.maxBedTemp >= 0) doc["max_bed_temp"] = data.maxBedTemp;

    if (data.density > 0) doc["density"] = data.density;
    if (data.filamentDiameter > 0) doc["filament_diameter"] = data.filamentDiameter;

    if (data.dryingTemperature >= 0) doc["drying_temp"] = data.dryingTemperature;
    if (data.dryingTime >= 0) doc["drying_time_min"] = data.dryingTime;

    if (data.consumedWeight > 0) doc["consumed_weight_g"] = data.consumedWeight;

    String result;
    serializeJson(doc, result);
    return result;
}

String openPrintTagToOpenSpoolJson(const OpenPrintTagData& data) {
    // Convert to OpenSpool-compatible JSON for systems that only support OpenSpool
    JsonDocument doc;

    doc["protocol"] = "openspool";
    doc["version"] = "1.0";

    // Map material type to OpenSpool "type" field
    if (data.materialAbbreviation.length() > 0) {
        doc["type"] = data.materialAbbreviation;
    } else {
        doc["type"] = optMaterialTypeToString(data.materialType);
    }

    if (data.hasPrimaryColor) {
        doc["color_hex"] = optColorToHexString(data.primaryColor);
    }

    if (data.brandName.length() > 0) {
        doc["brand"] = data.brandName;
    }

    if (data.minPrintTemp >= 0) doc["min_temp"] = String(data.minPrintTemp);
    if (data.maxPrintTemp >= 0) doc["max_temp"] = String(data.maxPrintTemp);

    String result;
    serializeJson(doc, result);
    return result;
}

// --- TLV Encoder ---

// Write a TLV field to buffer. Returns bytes written.
static uint16_t writeTlvField(uint8_t* buf, uint16_t maxLen, uint8_t key,
                               const uint8_t* value, uint16_t valueLen) {
    uint16_t headerSize = (valueLen < 0x80) ? 2 : 3;
    if (headerSize + valueLen > maxLen) return 0;

    buf[0] = key;
    if (valueLen < 0x80) {
        buf[1] = (uint8_t)valueLen;
    } else {
        buf[1] = 0x82; // 2-byte extended length
        buf[2] = (uint8_t)(valueLen >> 8);
        buf[3] = (uint8_t)(valueLen & 0xFF);
        headerSize = 4;
    }
    memcpy(buf + headerSize, value, valueLen);
    return headerSize + valueLen;
}

static uint16_t writeTlvUint8(uint8_t* buf, uint16_t maxLen, uint8_t key, uint8_t value) {
    return writeTlvField(buf, maxLen, key, &value, 1);
}

static uint16_t writeTlvUint16(uint8_t* buf, uint16_t maxLen, uint8_t key, uint16_t value) {
    uint8_t data[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    return writeTlvField(buf, maxLen, key, data, 2);
}

static uint16_t writeTlvString(uint8_t* buf, uint16_t maxLen, uint8_t key,
                                const String& value, uint8_t maxStrLen) {
    uint16_t len = min((uint16_t)value.length(), (uint16_t)maxStrLen);
    return writeTlvField(buf, maxLen, key, (const uint8_t*)value.c_str(), len);
}

static uint16_t writeTlvUuid(uint8_t* buf, uint16_t maxLen, uint8_t key, const String& uuid) {
    if (uuid.length() < 32) return 0;

    // Parse UUID string "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" to 16 bytes
    uint8_t data[16];
    String hex = uuid;
    hex.replace("-", "");
    if (hex.length() < 32) return 0;

    for (int i = 0; i < 16; i++) {
        data[i] = (uint8_t)strtoul(hex.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
    }
    return writeTlvField(buf, maxLen, key, data, 16);
}

uint8_t* encodeOpenPrintTag(const OpenPrintTagData& data, uint16_t& bufferLength) {
    // Estimate max size: 512 bytes should cover all fields
    uint16_t maxSize = 512;
    uint8_t* buf = (uint8_t*)malloc(maxSize);
    if (!buf) { bufferLength = 0; return nullptr; }
    memset(buf, 0, maxSize);

    uint16_t offset = 0;
    uint16_t written;

    // Required: material_class
    written = writeTlvUint8(buf + offset, maxSize - offset, OPT_MATERIAL_CLASS, (uint8_t)data.materialClass);
    offset += written;

    // Identity fields
    if (data.instanceUuid.length() >= 32) {
        written = writeTlvUuid(buf + offset, maxSize - offset, OPT_INSTANCE_UUID, data.instanceUuid);
        offset += written;
    }
    if (data.brandUuid.length() >= 32) {
        written = writeTlvUuid(buf + offset, maxSize - offset, OPT_BRAND_UUID, data.brandUuid);
        offset += written;
    }

    // Material info
    if (data.materialType != OPT_MT_UNKNOWN) {
        written = writeTlvUint8(buf + offset, maxSize - offset, OPT_MATERIAL_TYPE, (uint8_t)data.materialType);
        offset += written;
    }
    if (data.materialName.length() > 0) {
        written = writeTlvString(buf + offset, maxSize - offset, OPT_MATERIAL_NAME, data.materialName, 31);
        offset += written;
    }
    if (data.materialAbbreviation.length() > 0) {
        written = writeTlvString(buf + offset, maxSize - offset, OPT_MATERIAL_ABBREVIATION, data.materialAbbreviation, 7);
        offset += written;
    }
    if (data.brandName.length() > 0) {
        written = writeTlvString(buf + offset, maxSize - offset, OPT_BRAND_NAME, data.brandName, 31);
        offset += written;
    }

    // Weights
    if (data.nominalNettoWeight > 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_NOMINAL_NETTO_WEIGHT, (uint16_t)data.nominalNettoWeight);
        offset += written;
    }
    if (data.actualNettoWeight > 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_ACTUAL_NETTO_WEIGHT, (uint16_t)data.actualNettoWeight);
        offset += written;
    }
    if (data.emptyContainerWeight > 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_EMPTY_CONTAINER_WEIGHT, (uint16_t)data.emptyContainerWeight);
        offset += written;
    }

    // Color
    if (data.hasPrimaryColor) {
        uint8_t colorBytes[4] = {
            (uint8_t)((data.primaryColor >> 24) & 0xFF),
            (uint8_t)((data.primaryColor >> 16) & 0xFF),
            (uint8_t)((data.primaryColor >> 8) & 0xFF),
            (uint8_t)(data.primaryColor & 0xFF)
        };
        written = writeTlvField(buf + offset, maxSize - offset, OPT_PRIMARY_COLOR, colorBytes, 4);
        offset += written;
    }

    // Temperatures
    if (data.minPrintTemp >= 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_MIN_PRINT_TEMP, (uint16_t)data.minPrintTemp);
        offset += written;
    }
    if (data.maxPrintTemp >= 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_MAX_PRINT_TEMP, (uint16_t)data.maxPrintTemp);
        offset += written;
    }
    if (data.minBedTemp >= 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_MIN_BED_TEMP, (uint16_t)data.minBedTemp);
        offset += written;
    }
    if (data.maxBedTemp >= 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_MAX_BED_TEMP, (uint16_t)data.maxBedTemp);
        offset += written;
    }

    // Physical
    if (data.density > 0) {
        uint16_t densityFixed = (uint16_t)(data.density * 100);
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_DENSITY, densityFixed);
        offset += written;
    }
    if (data.filamentDiameter > 0 && data.filamentDiameter != 1.75f) {
        uint16_t diameterFixed = (uint16_t)(data.filamentDiameter * 100);
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_FILAMENT_DIAMETER, diameterFixed);
        offset += written;
    }

    // Drying
    if (data.dryingTemperature >= 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_DRYING_TEMPERATURE, (uint16_t)data.dryingTemperature);
        offset += written;
    }
    if (data.dryingTime >= 0) {
        written = writeTlvUint16(buf + offset, maxSize - offset, OPT_DRYING_TIME, (uint16_t)data.dryingTime);
        offset += written;
    }

    bufferLength = offset;
    return buf;
}

uint8_t* buildOpenPrintTagNdefMessage(const OpenPrintTagData& data, uint16_t& bufferLength) {
    // First, encode the TLV payload
    uint16_t tlvLen = 0;
    uint8_t* tlvData = encodeOpenPrintTag(data, tlvLen);
    if (!tlvData || tlvLen == 0) {
        bufferLength = 0;
        return nullptr;
    }

    // NDEF MIME type for OpenPrintTag
    const char* mimeType = "application/vnd.openprinttag";
    uint8_t mimeLen = strlen(mimeType);

    // Calculate NDEF record size
    // Header(1) + TypeLen(1) + PayloadLen(1 or 4) + Type(N) + Payload(N)
    bool shortRecord = (tlvLen < 256);
    uint8_t payloadLenBytes = shortRecord ? 1 : 4;
    uint16_t ndefRecordLen = 1 + 1 + payloadLenBytes + mimeLen + tlvLen;

    // NDEF TLV wrapper: 0x03 + length(1 or 3) + record + 0xFE terminator
    bool extendedTlv = (ndefRecordLen > 254);
    uint16_t tlvHeaderLen = extendedTlv ? 4 : 2;
    uint16_t totalLen = tlvHeaderLen + ndefRecordLen + 1; // +1 for 0xFE terminator

    uint8_t* msg = (uint8_t*)malloc(totalLen);
    if (!msg) {
        free(tlvData);
        bufferLength = 0;
        return nullptr;
    }
    memset(msg, 0, totalLen);

    uint16_t offset = 0;

    // NDEF TLV header
    msg[offset++] = 0x03; // NDEF Message TLV
    if (extendedTlv) {
        msg[offset++] = 0xFF;
        msg[offset++] = (uint8_t)(ndefRecordLen >> 8);
        msg[offset++] = (uint8_t)(ndefRecordLen & 0xFF);
    } else {
        msg[offset++] = (uint8_t)ndefRecordLen;
    }

    // NDEF Record Header
    // MB=1, ME=1, CF=0, SR=shortRecord, IL=0, TNF=0x02 (MIME media)
    uint8_t header = 0xC0 | (shortRecord ? 0x10 : 0x00) | 0x02;
    msg[offset++] = header;
    msg[offset++] = mimeLen;

    if (shortRecord) {
        msg[offset++] = (uint8_t)tlvLen;
    } else {
        msg[offset++] = (uint8_t)((tlvLen >> 24) & 0xFF);
        msg[offset++] = (uint8_t)((tlvLen >> 16) & 0xFF);
        msg[offset++] = (uint8_t)((tlvLen >> 8) & 0xFF);
        msg[offset++] = (uint8_t)(tlvLen & 0xFF);
    }

    // MIME type
    memcpy(msg + offset, mimeType, mimeLen);
    offset += mimeLen;

    // TLV payload
    memcpy(msg + offset, tlvData, tlvLen);
    offset += tlvLen;

    // Terminator
    msg[offset++] = 0xFE;

    free(tlvData);
    bufferLength = offset;
    return msg;
}
