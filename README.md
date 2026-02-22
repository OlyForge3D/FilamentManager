# FilamentManager

> **An ESP32 NFC filament management system for 3D printing.**
> Read and write NFC spool tags, track filament usage, and integrate with Spoolman, Moonraker/Klipper, Bambu Lab AMS, and [PrintFarmer](https://github.com/jpapiez/PrintFarmer).

## Attribution

FilamentManager is derived from [FilaMan](https://github.com/ManuelW77/Filaman) by Manuel Weiser.

## What FilamentManager Adds

- OpenPrintTag binary TLV support in addition to OpenSpool JSON
- Multi-backend printer integrations (Moonraker/Klipper and PrintFarmer)
- Fleet management hooks (heartbeat + scan webhooks) for PrintFarmer

## Features

### NFC Tag Support

- **OpenSpool** — JSON NDEF format (`application/json`), the community standard
- **OpenPrintTag** — Binary TLV NDEF format (`application/vnd.openprinttag`), Prusa's standard
- **Auto-detection** — format is detected automatically on scan
- **Read & write** — both formats can be read from and written to NTAG213/215/216 tags
- **Spoolman mapping** — scanned tag data maps to Spoolman spool entries; creates new entries when no match is found

### Printer Backend Integrations

- **Bambu Lab AMS** — MQTT-based AMS slot assignment and monitoring
- **Moonraker/Klipper** — sets active spool via `POST /server/spoolman/spool_id` on scan; Moonraker's Spoolman proxy can replace direct Spoolman access
- **PrintFarmer** — reports spool scans and device health to a PrintFarmer server for centralized farm management

### PrintFarmer Integration

- **Heartbeat** — WiFi RSSI, NFC reader status, firmware version, free heap reported every 60 seconds
- **Scan webhook** — each NFC scan sends spool ID, tag format, material type, and brand
- **Auto-registration** — the PrintFarmer server creates the device record on first heartbeat
- **Companion page** — PrintFarmer includes a React NFC Devices page showing device status, scan history, and associated printers

### Spoolman Integration

- List, filter, and select filament spools
- Update spool weights automatically via scale
- Track NFC tag assignments
- Manufacturer tag auto-import
- Supports Spoolman OctoPrint Plugin

### Hardware Features

- **NFC read/write** — PN532 module via software SPI
- **WiFi** — WiFiManager captive portal for network setup
- **OTA updates** — firmware and filesystem updates via web UI
- **Optional peripherals** — weight measurement (HX711), OLED display (SSD1306), touch tare button — see [Optional Features](OPTIONAL_FEATURES.md)

## Manufacturer Tags Support

FilamentManager supports **Manufacturer Tags** — NFC tags that come pre-programmed directly from filament manufacturers.

### RecyclingFabrik Partnership

[**RecyclingFabrik**](https://www.recyclingfabrik.com) is the first manufacturer to support FilamentManager-compatible NFC tags on their spools. When scanned, these tags automatically create Spoolman entries with manufacturer-verified specifications.

Supported manufacturer-tag types (brief):
- Pre-programmed **RecyclingFabrik** tags
- Generic manufacturer tags that follow the compact JSON schema (`b`, `an`, `t`, `c`, `cn`, `et`, `bt`, `di`, `de`, `sw`, optional `mc`, `mcd`, `u`)

For the full format, examples, and implementation details: [Manufacturer Tags Documentation](manufacturer_tags.md)

## Hardware Requirements

### Core Components (Affiliate Links)
- **ESP32 Development Board** — Any ESP32 variant (WROOM-32D, C3, C6)
[Amazon Link](https://amzn.to/3FHea6D)
- **PN532 NFC Module** — V3 with SPI support
[Amazon Link](https://amzn.eu/d/gy9vaBX)
- **NFC Tags** — NTAG213, NTAG215, or NTAG216
[Amazon Link](https://amzn.to/3E071xO)

For optional hardware (scale, OLED display, touch sensor), see [Optional Features](OPTIONAL_FEATURES.md).


### Pin Configuration (PN532)

Default PN532 wiring uses **software SPI** (bit-banged GPIO) and varies by firmware target. Any GPIO pin can be used — defaults can be changed via the Hardware page without reflashing.

#### Power Connections (all boards)

| ESP32 | PN532 |
|:---:|:---:|
| 3.3V | VCC |
| GND | GND |

#### SPI Signal Connections

| Firmware Target | SCK | MISO | MOSI | SS | IRQ | RESET |
|---|:---:|:---:|:---:|:---:|:---:|:---:|
| ESP32-WROOM-32D | 18 | 19 | 23 | 5 | 4 | 27 |
| ESP32-C3-Mini | 4 | 5 | 6 | 7 | 10 | 3 |
| ESP32-C3-SuperMini | 4 | 5 | 6 | 7 | 10 | 3 |
| ESP32-C6-DevKit-1 | 6 | 2 | 7 | 18 | 19 | 20 |

**Important:** Set the PN532 DIP switches to SPI mode.

For optional hardware pin configurations (scale, display, touch sensor), see [Optional Features](OPTIONAL_FEATURES.md).

## Software Dependencies

### ESP32 Libraries
- `WiFiManager`: Network configuration
- `ESPAsyncWebServer`: Web server functionality
- `ArduinoJson`: JSON parsing and creation
- `PubSubClient`: MQTT communication
- `Adafruit_PN532`: NFC functionality

### Installation

## Prerequisites
- **Software:**
  - [PlatformIO](https://platformio.org/) in VS Code (for building from source)
  - [Spoolman](https://github.com/Donkie/Spoolman) instance
- **Hardware:**
  - ESP32 Development Board
  - PN532 NFC Module
  - NFC tags (NTAG213/215/216)
  - Connecting wires

## Important Note
You have to activate Spoolman in debug mode, because you are not able to set CORS Domains in Spoolman yet.

```
# Enable debug mode
# If enabled, the client will accept requests from any host
# This can be useful when developing, but is also a security risk
# Default: FALSE
#SPOOLMAN_DEBUG_MODE=TRUE
```


## Step-by-Step Installation
### Easy Installation
1. **Go to [FilamentManager Web Installer](https://olyforge3d.github.io/FilamentManager/installer/)**

2. **Plug you device in and push Connect button**

3. **Select your Device Port and push Intall**

4. **Initial Setup:**
    - Connect to the device WiFi access point (default SSID: "FilaMan").
    - Configure WiFi settings through the captive portal.
    - Access the web interface at `http://filaman.local` (default hostname) or the IP address.

### Compile by yourself
1. **Clone the Repository:**
    ```bash
    git clone https://github.com/OlyForge3D/FilamentManager.git
    cd FilamentManager
    ```
2. **Install Dependencies:**
    ```bash
    pio lib install
    ```
3. **Flash the ESP32:**
    ```bash
    pio run --target upload
    ```
4. **Initial Setup:**
    - Connect to the device WiFi access point (default SSID: "FilaMan").
    - Configure WiFi settings through the captive portal.
    - Access the web interface at `http://filaman.local` (default hostname) or the IP address.

## Documentation

### Relevant Links
- [PlatformIO Documentation](https://docs.platformio.org/)
- [Spoolman Documentation](https://github.com/Donkie/Spoolman)
- [Bambu Lab Printer Documentation](https://www.bambulab.com/)

### Tutorials and Examples
- [PlatformIO Getting Started](https://docs.platformio.org/en/latest/tutorials/espressif32/arduino_debugging_unit_testing.html)
- [ESP32 Web Server Tutorial](https://randomnerdtutorials.com/esp32-web-server-arduino-ide/)

## License

This project is licensed under the MIT License. See the [LICENSE.txt](LICENSE.txt) file for details.

## Materials

### Useful Resources
- [ESP32 Official Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Arduino Libraries](https://www.arduino.cc/en/Reference/Libraries)
- [NFC Tag Information](https://learn.adafruit.com/adafruit-pn532-rfid-nfc/overview)

### Community and Support
- [PlatformIO Community](https://community.platformio.org/)
- [Arduino Forum](https://forum.arduino.cc/)
- [ESP32 Forum](https://www.esp32.com/)

## Availability

- **FilamentManager**: [github.com/OlyForge3D/FilamentManager](https://github.com/OlyForge3D/FilamentManager)
- **PrintFarmer**: [github.com/jpapiez/PrintFarmer](https://github.com/jpapiez/PrintFarmer)
