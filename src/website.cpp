#include "website.h"
#include "commonFS.h"
#include "api.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "bambu.h"
#include "nfc.h"
#include "openprinttag.h"
#include "scale.h"
#include "esp_task_wdt.h"
#include <Update.h>
#include "display.h"
#include "ota.h"
#include "config.h"
#include "debug.h"


#ifndef VERSION
  #define VERSION "1.1.0"
#endif

// Define Cache-Control header
#define CACHE_CONTROL "max-age=604800" // Cache for 1 week

AsyncWebServer server(webserverPort);
AsyncWebSocket ws("/ws");

uint8_t lastSuccess = 0;
nfcReaderStateType lastnfcReaderState = NFC_IDLE;


void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    HEAP_DEBUG_MESSAGE("onWsEvent begin");
    if (type == WS_EVT_CONNECT) {
        Serial.println("New client connected!");
        // Send AMS data to the new client
        if (!bambuDisabled) sendAmsData(client);
        sendNfcData();
        foundNfcTag(client, 0);
        sendWriteResult(client, 3);

        // Clean up dead connections
        (*server).cleanupClients();
        Serial.println("Currently connected number of clients: " + String((*server).getClients().size()));
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("Client disconnected.");
    } else if (type == WS_EVT_ERROR) {
        Serial.printf("WebSocket Client #%u error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
    } else if (type == WS_EVT_PONG) {
        Serial.printf("WebSocket Client #%u pong\n", client->id());
    } else if (type == WS_EVT_DATA) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (char*)data, len);
        //String message = String((char*)data);
        //deserializeJson(doc, message);

        if (error) {
            Serial.println("JSON deserialization failed: " + String(error.c_str()));
            return;
        }

        if (doc["type"] == "heartbeat") {
            // Send heartbeat response
            ws.text(client->id(), "{"
                "\"type\":\"heartbeat\","
                "\"freeHeap\":" + String(ESP.getFreeHeap()/1024) + ","
                "\"bambu_connected\":" + String(bambu_connected) + ","
                "\"spoolman_connected\":" + String(spoolmanConnected) + ""
                "}");
        }

        else if (doc["type"] == "writeNfcTag") {
            if (doc["payload"].is<JsonObject>()) {
                // Attempt to write NFC data
                String payloadString;
                serializeJson(doc["payload"], payloadString);

                startWriteJsonToTag((doc["tagType"] == "spool") ? true : false, payloadString.c_str());
            }
        }

        else if (doc["type"] == "writeOpenPrintTag") {
            if (doc["payload"].is<JsonObject>()) {
                String payloadString;
                serializeJson(doc["payload"], payloadString);
                startWriteOpenPrintTagToTag(payloadString.c_str());
            }
        }

        else if (doc["type"] == "scale") {
            uint8_t success = 0;
            if (doc["payload"] == "tare") {
                scaleTareRequest = true;
                success = 1;
                //success = tareScale();
            }

            if (doc["payload"] == "calibrate") {
                success = calibrate_scale();
            }

            if (doc["payload"] == "setAutoTare") {
                success = setAutoTare(doc["enabled"].as<bool>());
            }

            if (success) {
                ws.textAll("{\"type\":\"scale\",\"payload\":\"success\"}");
            } else {
                ws.textAll("{\"type\":\"scale\",\"payload\":\"error\"}");
            }
        }

        else if (doc["type"] == "reconnect") {
            if (doc["payload"] == "bambu") {
                bambu_restart();
            }

            if (doc["payload"] == "spoolman") {
                initSpoolman();
            }
        }

        else if (doc["type"] == "setBambuSpool") {
            Serial.println(doc["payload"].as<String>());
            setBambuSpool(doc["payload"]);
        }

        else if (doc["type"] == "setSpoolmanSettings") {
            Serial.println(doc["payload"].as<String>());
            if (updateSpoolBambuData(doc["payload"].as<String>())) {
                ws.textAll("{\"type\":\"setSpoolmanSettings\",\"payload\":\"success\"}");
            } else {
                ws.textAll("{\"type\":\"setSpoolmanSettings\",\"payload\":\"error\"}");
            }
        }

        else if (doc["type"] == "saveMoonrakerSettings") {
            String url = doc["payload"]["url"].as<String>();
            String apiKey = doc["payload"]["apiKey"].as<String>();
            saveMoonrakerSettings(url, apiKey);
            ws.textAll("{\"type\":\"saveMoonrakerSettings\",\"payload\":\"success\"}");
        }

        else if (doc["type"] == "savePrintFarmerSettings") {
            String url = doc["payload"]["url"].as<String>();
            String apiKey = doc["payload"]["apiKey"].as<String>();
            String printerId = doc["payload"]["printerId"].as<String>();
            savePrintFarmerSettings(url, apiKey, printerId);
            ws.textAll("{\"type\":\"savePrintFarmerSettings\",\"payload\":\"success\"}");
        }

        else {
            Serial.println("Unknown WebSocket type: " + doc["type"].as<String>());
        }
        doc.clear();
    }
    HEAP_DEBUG_MESSAGE("onWsEvent end");
}

// Function for loading and replacing the header in an HTML file
String loadHtmlWithHeader(const char* filename) {
    Serial.println("Loading HTML file: " + String(filename));
    if (!LittleFS.exists(filename)) {
        Serial.println("Error: File not found!");
        return "Error: File not found!";
    }

    File file = LittleFS.open(filename, "r");
    String html = file.readString();
    file.close();

    return html;
}

void sendWriteResult(AsyncWebSocketClient *client, uint8_t success) {
    // Send success/failure to all clients
    String response = "{\"type\":\"writeNfcTag\",\"success\":" + String(success ? "1" : "0") + "}";
    ws.textAll(response);
}

void foundNfcTag(AsyncWebSocketClient *client, uint8_t success) {
    if (success == lastSuccess) return;
    ws.textAll("{\"type\":\"nfcTag\", \"payload\":{\"found\": " + String(success) + "}}");
    sendNfcData();
    lastSuccess = success;
}

void sendNfcData() {
    if (lastnfcReaderState == nfcReaderState) return;
    // TBD: Why is there no status for reading the tag?
    switch(nfcReaderState){
        case NFC_IDLE:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":{}}");
            break;
        case NFC_READ_SUCCESS:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":" + nfcJsonData + "}");
            break;
        case NFC_READ_ERROR:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":{\"error\":\"Empty Tag or Data not readable\"}}");
            break;
        case NFC_WRITING:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":{\"info\":\"Writing tag...\"}}");
            break;
        case NFC_WRITE_SUCCESS:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":{\"info\":\"Tag written successfully\"}}");
            break;
        case NFC_WRITE_ERROR:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":{\"error\":\"Error writing to Tag\"}}");
            break;
        case DEFAULT:
            ws.textAll("{\"type\":\"nfcData\", \"payload\":{\"error\":\"Something went wrong\"}}");
    }
    lastnfcReaderState = nfcReaderState;
}

void sendAmsData(AsyncWebSocketClient *client) {
    if (ams_count > 0) {
        ws.textAll("{\"type\":\"amsData\",\"payload\":" + amsJsonData + "}");
    }
}

void setupWebserver(AsyncWebServer &server) {
    oledShowProgressBar(2, 7, DISPLAY_BOOT_TEXT, "Webserver init");
    // Disable all debug output
    Serial.setDebugOutput(false);
    
    // WebSocket optimizations
    ws.onEvent(onWsEvent);
    ws.enable(true);

    // Configure server for large uploads
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){});
    server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){});

    // Load the Spoolman URL at boot
    spoolmanUrl = loadSpoolmanUrl();
    Serial.print("Loaded Spoolman URL: ");
    Serial.println(spoolmanUrl);

    // Load Bamb credentials:
    loadBambuCredentials();

    // Route for about
    server.on("/about", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /about received");
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
    });

    // Route for scale
    server.on("/waage", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /waage received");
        //AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/waage.html.gz", "text/html");
        //response->addHeader("Content-Encoding", "gzip");
        //response->addHeader("Cache-Control", CACHE_CONTROL);

        String html = loadHtmlWithHeader("/waage.html");
        html.replace("{{autoTare}}", (autoTare) ? "checked" : "");

        request->send(200, "text/html", html);
    });

    // Route for RFID
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /rfid received");
        
        String page = (bambuDisabled) ? "/rfid.html.gz" : "/rfid_bambu.html.gz";
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, page, "text/html");
        
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("RFID page sent");
    });

    server.on("/api/url", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("API call: /api/url");
        String jsonResponse = "{\"spoolman_url\": \"" + String(spoolmanUrl) + "\"}";
        request->send(200, "application/json", jsonResponse);
    });

    // Route for WiFi
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /wifi received");
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/wifi.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
    });

    // Route for Spoolman settings
    server.on("/spoolman", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /spoolman received");
        String html = loadHtmlWithHeader("/spoolman.html");
        html.replace("{{spoolmanUrl}}", (spoolmanUrl != "") ? spoolmanUrl : "");
        html.replace("{{spoolmanOctoEnabled}}", octoEnabled ? "checked" : "");
        html.replace("{{spoolmanOctoUrl}}", (octoUrl != "") ? octoUrl : "");
        html.replace("{{spoolmanOctoToken}}", (octoToken != "") ? octoToken : "");

        html.replace("{{bambuIp}}", bambuCredentials.ip);            
        html.replace("{{bambuSerial}}", bambuCredentials.serial);
        html.replace("{{bambuCode}}", bambuCredentials.accesscode ? bambuCredentials.accesscode : "");
        html.replace("{{autoSendToBambu}}", bambuCredentials.autosend_enable ? "checked" : "");
        html.replace("{{autoSendTime}}", (bambuCredentials.autosend_time != 0) ? String(bambuCredentials.autosend_time) : String(BAMBU_DEFAULT_AUTOSEND_TIME));

        request->send(200, "text/html", html);
    });

    // Route for checking Spoolman instance
    server.on("/api/checkSpoolman", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!request->hasParam("url")) {
            request->send(400, "application/json", "{\"success\": false, \"error\": \"Missing URL parameter\"}");
            return;
        }

        if (request->getParam("octoEnabled")->value() == "true" && (!request->hasParam("octoUrl") || !request->hasParam("octoToken"))) {
            request->send(400, "application/json", "{\"success\": false, \"error\": \"Missing OctoPrint URL or Token parameter\"}");
            return;
        }

        String url = request->getParam("url")->value();
        if (url.indexOf("http://") == -1 && url.indexOf("https://") == -1) {
            url = "http://" + url;
        }
        // Remove trailing slash if exists
        if (url.length() > 0 && url.charAt(url.length()-1) == '/') {
            url = url.substring(0, url.length()-1);
        }
        
        bool octoEnabled = (request->getParam("octoEnabled")->value() == "true") ? true : false;
        String octoUrl = request->getParam("octoUrl")->value();
        String octoToken = (request->getParam("octoToken")->value() != "") ? request->getParam("octoToken")->value() : "";

        url.trim();
        octoUrl.trim();
        octoToken.trim();
        
        bool healthy = saveSpoolmanUrl(url, octoEnabled, octoUrl, octoToken);
        String jsonResponse = "{\"healthy\": " + String(healthy ? "true" : "false") + "}";

        request->send(200, "application/json", jsonResponse);
    });

    // Route for checking Bambu instance
    server.on("/api/bambu", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("remove")) {
            if (removeBambuCredentials()) {
                request->send(200, "application/json", "{\"success\": true}");
            } else {
                request->send(500, "application/json", "{\"success\": false, \"error\": \"Error deleting Bambu credentials\"}");
            }
            return;
        }

        if (!request->hasParam("bambu_ip") || !request->hasParam("bambu_serialnr") || !request->hasParam("bambu_accesscode")) {
            request->send(400, "application/json", "{\"success\": false, \"error\": \"Missing parameter\"}");
            return;
        }

        String bambu_ip = request->getParam("bambu_ip")->value();
        String bambu_serialnr = request->getParam("bambu_serialnr")->value();
        String bambu_accesscode = request->getParam("bambu_accesscode")->value();
        bool autoSend = (request->getParam("autoSend")->value() == "true") ? true : false;
        String autoSendTime = request->getParam("autoSendTime")->value();
        
        bambu_ip.trim();
        bambu_serialnr.trim();
        bambu_accesscode.trim();
        autoSendTime.trim();

        if (bambu_ip.length() == 0 || bambu_serialnr.length() == 0 || bambu_accesscode.length() == 0) {
            request->send(400, "application/json", "{\"success\": false, \"error\": \"Empty parameter\"}");
            return;
        }

        bool success = saveBambuCredentials(bambu_ip, bambu_serialnr, bambu_accesscode, autoSend, autoSendTime);

        request->send(200, "application/json", "{\"healthy\": " + String(success ? "true" : "false") + "}");
    });

    // Route for checking Spoolman instance
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        ESP.restart();
    });

    // Route for loading CSS file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Loading style.css");
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/style.css.gz", "text/css");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("style.css sent");
    });

    // Route for logo
    server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/logo.png.gz", "image/png");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("logo.png sent");
    });

    // Route for favicon
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/favicon.ico", "image/x-icon");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("favicon.ico sent");
    });

    // Route for spool_in.png
    server.on("/spool_in.png", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/spool_in.png.gz", "image/png");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("spool_in.png sent");
    });

    // Route for set_spoolman.png
    server.on("/set_spoolman.png", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/set_spoolman.png.gz", "image/png");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("set_spoolman.png sent");
    });

    // Route for JavaScript files
    server.on("/spoolman.js", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /spoolman.js received");
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/spoolman.js.gz", "text/javascript");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("Spoolman.js sent");
    });

    server.on("/rfid.js", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /rfid.js received");
        AsyncWebServerResponse *response = request->beginResponse(LittleFS,"/rfid.js.gz", "text/javascript");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", CACHE_CONTROL);
        request->send(response);
        Serial.println("RFID.js sent");
    });

    // Simplified update handler
    server.on("/upgrade", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/upgrade.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    });

    // Register update handler
    handleUpdate(server);

    server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request){
        String fm_version = VERSION;
        String jsonResponse = "{\"version\": \""+ fm_version +"\"}";
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/api/backends", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["moonraker"]["enabled"] = moonrakerEnabled;
        doc["moonraker"]["url"] = moonrakerUrl;
        doc["moonraker"]["apiKey"] = moonrakerApiKey;
        doc["printfarmer"]["enabled"] = printFarmerEnabled;
        doc["printfarmer"]["url"] = printFarmerUrl;
        doc["printfarmer"]["apiKey"] = printFarmerApiKey;
        doc["printfarmer"]["printerId"] = printFarmerPrinterId;
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    // ── GET /api/v1/pins ──
    server.on("/api/v1/pins", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["board"] = BOARD_NAME;
        doc["current"]["sck"]   = pn532Pins.sck;
        doc["current"]["miso"]  = pn532Pins.miso;
        doc["current"]["mosi"]  = pn532Pins.mosi;
        doc["current"]["ss"]    = pn532Pins.ss;
        doc["current"]["irq"]   = pn532Pins.irq;
        doc["current"]["reset"] = pn532Pins.reset;
        doc["defaults"]["sck"]   = (uint8_t)DEFAULT_PN532_SCK;
        doc["defaults"]["miso"]  = (uint8_t)DEFAULT_PN532_MISO;
        doc["defaults"]["mosi"]  = (uint8_t)DEFAULT_PN532_MOSI;
        doc["defaults"]["ss"]    = (uint8_t)DEFAULT_PN532_SS;
        doc["defaults"]["irq"]   = (uint8_t)DEFAULT_PN532_IRQ;
        doc["defaults"]["reset"] = (uint8_t)DEFAULT_PN532_RESET;
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    // ── PUT /api/v1/pins (via GET with query params for simplicity) ──
    server.on("/api/v1/pins/update", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!request->hasParam("sck") || !request->hasParam("miso") ||
            !request->hasParam("mosi") || !request->hasParam("ss") ||
            !request->hasParam("irq") || !request->hasParam("reset")) {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing pin parameters\"}");
            return;
        }
        Pn532Pins newPins;
        newPins.sck   = request->getParam("sck")->value().toInt();
        newPins.miso  = request->getParam("miso")->value().toInt();
        newPins.mosi  = request->getParam("mosi")->value().toInt();
        newPins.ss    = request->getParam("ss")->value().toInt();
        newPins.irq   = request->getParam("irq")->value().toInt();
        newPins.reset = request->getParam("reset")->value().toInt();

        if (!savePinConfig(newPins)) {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"Validation failed – all pins must be unique\"}");
            return;
        }
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Pins saved. Reboot to apply.\"}");
    });

    // ── Hardware / Pin Mapping page ──
    server.on("/hardware", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Request for /hardware received");
        String html = loadHtmlWithHeader("/hardware.html");
        request->send(200, "text/html", html);
    });

    // Error handling for pages not found
    server.onNotFound([](AsyncWebServerRequest *request){
        Serial.print("404 - Not found: ");
        Serial.println(request->url());
        request->send(404, "text/plain", "Page not found");
    });

    // WebSocket route
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    ws.enable(true);

    // Start the webserver
    server.begin();
    Serial.println("Webserver started");
}
