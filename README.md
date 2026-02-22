# FilamentManager

> **An ESP32 NFC filament management system for 3D printing.**
> Read and write NFC spool tags, track filament usage, and integrate with Spoolman, Moonraker/Klipper, Bambu Lab AMS, and [PrintFarmer](https://github.com/jpapiez/PrintFarmer).

## Attribution

FilamentManager is derived from [FilaMan](https://github.com/ManuelW77/Filaman) by Manuel Weiser.
To satisfy MIT license requirements, the original copyright and permission notice are retained in [LICENSE.txt](LICENSE.txt).

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

- **Weight measurement** — HX711 load cell for precise spool weighing
- **NFC read/write** — PN532 module via I2C
- **OLED display** — 128×64 SSD1306 for status display
- **WiFi** — WiFiManager captive portal for network setup
- **OTA updates** — firmware and filesystem updates via web UI

## Manufacturer Tags Support

FilamentManager supports **Manufacturer Tags** — NFC tags that come pre-programmed directly from filament manufacturers.

### RecyclingFabrik Partnership

[**RecyclingFabrik**](https://www.recyclingfabrik.com) is the first manufacturer to support FilamentManager-compatible NFC tags on their spools. When scanned, these tags automatically create Spoolman entries with manufacturer-verified specifications.

For technical details: [Manufacturer Tags Documentation](README_ManufacturerTags_EN.md)

## Hardware Requirements

### Components (Affiliate Links)
- **ESP32 Development Board:** Any ESP32 variant.
[Amazon Link](https://amzn.to/3FHea6D)
- **HX711 5kg Load Cell Amplifier:** For weight measurement.
[Amazon Link](https://amzn.to/4ja1KTe)
- **OLED 0.96 Zoll I2C white/yellow Display:** 128x64 SSD1306.
[Amazon Link](https://amzn.to/445aaa9)
- **PN532 NFC NXP RFID-Modul V3:** For NFC tag operations.
[Amazon Link](https://amzn.eu/d/gy9vaBX)
- **NFC Tags NTAG213 NTAG215:** RFID Tag
[Amazon Link](https://amzn.to/3E071xO)
- **TTP223 Touch Sensor (optional):** For reTARE per Button/Touch
[Amazon Link](https://amzn.to/4hTChMK)


### Pin Configuration
| Component          | ESP32 Pin |
|-------------------|-----------|
| HX711 DOUT        | 16        |
| HX711 SCK         | 17        |
| OLED SDA          | 21        |
| OLED SCL          | 22        |
| PN532 IRQ         | 32        |
| PN532 RESET       | 33        |
| PN532 SDA         | 21        |
| PN532 SCL         | 22        |
| TTP223 I/O        | 25        |

**!! Make sure that the DIP switches on the PN532 are set to I2C**  
**Use the 3V pin from the ESP for the touch sensor**

![Wiring](./img/Schaltplan.png)

![myWiring](./img/IMG_2589.jpeg)
![myWiring](./img/IMG_2590.jpeg)

*The load cell is connected to most HX711 modules as follows:  
E+ red  
E- black  
A- white  
A+ green*

## Software Dependencies

### ESP32 Libraries
- `WiFiManager`: Network configuration
- `ESPAsyncWebServer`: Web server functionality
- `ArduinoJson`: JSON parsing and creation
- `PubSubClient`: MQTT communication
- `Adafruit_PN532`: NFC functionality
- `Adafruit_SSD1306`: OLED display control
- `HX711`: Load cell communication

### Installation

## Prerequisites
- **Software:**
  - [PlatformIO](https://platformio.org/) in VS Code
  - [Spoolman](https://github.com/Donkie/Spoolman) instance
- **Hardware:**
  - ESP32 Development Board
  - HX711 Load Cell Amplifier
  - Load Cell (weight sensor)
  - OLED Display (128x64 SSD1306)
  - PN532 NFC Module
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
