# Copilot Instructions for FilamentManager

## Project Overview

ESP32 Arduino firmware for NFC-based 3D printer filament management. Reads/writes NFC spool tags (OpenSpool JSON and OpenPrintTag binary TLV formats), integrates with Spoolman, Bambu Lab AMS (MQTT), Moonraker/Klipper, and PrintFarmer. Fork of [FilaMan](https://github.com/ManuelW77/Filaman).

## Build Commands

```bash
# Build firmware (default env: esp32-wroom-32d)
pio run -e esp32-wroom-32d

# Build LittleFS filesystem image (runs combine_html.py + gzip_files.py automatically)
pio run -t buildfs

# Upload firmware to ESP32
pio run --target upload

# Upload filesystem
pio run --target uploadfs

# Install dependencies
pio lib install
```

There are no unit tests or linters configured.

## Architecture

### Firmware (src/)

Single-threaded main loop in `main.cpp` with FreeRTOS tasks for NFC reading (`nfc.cpp`), MQTT/Bambu communication (`bambu.cpp`), and scale reading (`scale.cpp`). Task core and priority assignments are in `config.cpp`.

Key modules:
- **nfc.cpp/h** — PN532 NFC reader via software SPI (bit-banged GPIO, pins configurable at runtime via NVS). State machine (`nfcReaderStateType`: IDLE → READING → READ_SUCCESS/ERROR → WRITING → WRITE_SUCCESS/ERROR). Detects tag format automatically (OpenSpool JSON vs OpenPrintTag binary TLV vs raw spool ID).
- **openprinttag.cpp/h** — OpenPrintTag binary TLV encoder/decoder (Prusa's NFC standard). Field keys defined as `OPTFieldKey` enum.
- **api.cpp/h** — Spoolman REST API client, Moonraker/Klipper integration, PrintFarmer webhooks/heartbeat. State machine (`spoolmanApiStateType`: INIT → IDLE → TRANSMITTING).
- **bambu.cpp/h** — Bambu Lab AMS MQTT client with TLS (cert in `bambu_cert.h`). Auto-send spool data to AMS slots.
- **website.cpp/h** — ESPAsyncWebServer + WebSocket for real-time UI updates. Serves gzipped files from LittleFS.
- **config.h/cpp** — Pin definitions, NVS namespace/key constants, display constants, FreeRTOS task config.
- **commonFS.cpp/h** — LittleFS JSON file helpers (`saveJsonValue`, `loadJsonValue`).
- **scale.cpp/h** — HX711 load cell for spool weighing.
- **display.cpp/h** — SSD1306 OLED 128×64 display.

### Web UI (html/)

Static HTML/JS/CSS served from LittleFS. Pages communicate with firmware via WebSocket (`/ws`) and REST API (`/api/v1`).

### Build Pipeline (scripts/)

1. **combine_html.py** — Pre-build: injects `header.html` content between `<!-- head -->` markers in all HTML files. This is how nav links propagate across pages.
2. **gzip_files.py** — Compresses HTML/JS/CSS/PNG into `data/` for LittleFS. Exceptions: `spoolman.html` and `waage.html` are copied uncompressed.
3. **extra_script.py** — Additional PlatformIO build hooks.

### Persistent Storage

- **NVS (Non-Volatile Storage)** — Credentials and settings. Namespaces: `api` (Spoolman/Moonraker/PrintFarmer URLs and keys), `bambu` (printer credentials), `scale` (calibration).
- **LittleFS** — Web UI files and JSON config files (`bambu_credentials.json`, `spoolman_url.json`, etc.).

## Key Conventions

- **State machines over callbacks** — NFC reader and Spoolman API use explicit enum state machines with polling in the main loop.
- **Version from build flags** — `VERSION` macro is set in `platformio.ini` via `-DVERSION=\"${common.version}\"`. The `[common]` section is the single source of truth.
- **HTML header injection** — Never duplicate nav/head content across HTML files. Edit `html/header.html` and the build system propagates it via `<!-- head -->` markers.
- **Watchdog** — 10-second WDT is active on the main loop task. Long-running operations must reset it (`esp_task_wdt_reset()`).
- **Global state via externs** — Module state is shared through `extern` globals declared in headers (e.g., `nfcReaderState`, `spoolmanApiState`, `bambu_connected`).
- **NFC tag format detection** — `detectTagFormat()` returns `NfcTagFormat` enum. Always handle both OpenSpool and OpenPrintTag formats when working with NFC read/write logic.
- **Partition layout** — Custom `partitions.csv` with dual OTA partitions (app0/app1 at 1.875MB each) and 192KB SPIFFS/LittleFS. Firmware must stay under ~1.875MB.
