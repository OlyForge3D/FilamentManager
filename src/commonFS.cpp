#include "commonFS.h"
#include <LittleFS.h>

bool removeJsonValue(const char* filename) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return true;
    }
    file.close();
    if (!LittleFS.remove(filename)) {
        Serial.print("Error deleting file: ");
        Serial.println(filename);
        return false;
    }
    return true;
}

bool saveJsonValue(const char* filename, const JsonDocument& doc) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.print("Error opening file for writing: ");
        Serial.println(filename);
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Error serializing JSON.");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool loadJsonValue(const char* filename, JsonDocument& doc) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Error opening file for reading: ");
        Serial.println(filename);
        return false;
    }
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        Serial.print("Error deserializing JSON: ");
        Serial.println(error.f_str());
        return false;
    }
    return true;
}

void initializeFileSystem() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.printf("LittleFS Total: %u bytes\n", LittleFS.totalBytes());
    Serial.printf("LittleFS Used: %u bytes\n", LittleFS.usedBytes());
    Serial.printf("LittleFS Free: %u bytes\n", LittleFS.totalBytes() - LittleFS.usedBytes());
}