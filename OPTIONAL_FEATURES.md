# Optional Hardware Features

FilamentManager supports optional hardware peripherals beyond the core NFC reader. These features are **not required** for basic NFC tag reading/writing and Spoolman integration.

## Weight Measurement (HX711 Load Cell)

The HX711 load cell amplifier allows automatic spool weight measurement. When a tagged spool is placed on the scale, FilamentManager can update the spool weight in Spoolman automatically.

### Components

- **HX711 5kg Load Cell Amplifier** — [Amazon Link](https://amzn.to/4ja1KTe)
- **5kg Load Cell** — typically included with the HX711 module

### Pin Configuration

| Component | Default GPIO | Notes |
|---|---:|---|
| HX711 DOUT | 16 | Data output from load cell |
| HX711 SCK | 17 | Clock signal |

### Load Cell Wiring

Connect the load cell to the HX711 module:

| Wire Color | HX711 Terminal |
|---|---|
| Red | E+ |
| Black | E- |
| White | A- |
| Green | A+ |

### Calibration

1. Access the web UI at `http://filaman.local`
2. Navigate to the Scale page
3. Follow the on-screen calibration procedure with a known weight

### Usage

Once calibrated, place a spool on the scale and scan its NFC tag. After 3 seconds of stable weight, FilamentManager sends the weight to Spoolman.

---

## OLED Display (SSD1306)

A 128×64 OLED display shows real-time status including WiFi signal, NFC reader state, and spool weight.

### Components

- **OLED 0.96" I2C Display** — 128×64 SSD1306, white or yellow — [Amazon Link](https://amzn.to/445aaa9)

### Pin Configuration

The display uses the default I2C pins for your ESP32 variant:

| ESP32 Variant | SDA | SCL |
|---|---:|---:|
| ESP32-WROOM-32D | 21 | 22 |
| ESP32-C3-Mini | 8 | 9 |
| ESP32-C3-SuperMini | 8 | 9 |
| ESP32-C6-DevKit-1 | 6 | 7 |

### Display States

- **Weight** — current spool weight in grams
- **WiFi signal** — connection status and RSSI
- **NFC status** — reading, writing, success, or error icons
- **Messages** — calibration prompts, auto-send countdown

---

## Touch Sensor (TTP223)

An optional touch sensor provides a hardware tare button for the scale.

### Components

- **TTP223 Touch Sensor** — [Amazon Link](https://amzn.to/4hTChMK)

### Pin Configuration

| Component | Default GPIO | Notes |
|---|---:|---|
| TTP223 I/O | 25 | ESP32-WROOM-32D; varies by board |

### Wiring

- **VCC** — Connect to **3.3V** (not 5V)
- **GND** — Ground
- **I/O** — GPIO 25 (or configured pin)

### Usage

Touch the sensor to tare (zero) the scale. The sensor is automatically detected on boot.

---

## Build Variants

FilamentManager can be built with or without these optional features:

| Build Variant | NFC | Scale | Display | Touch |
|---|:---:|:---:|:---:|:---:|
| `esp32-wroom-32d` | ✓ | ✓ | ✓ | ✓ |
| `esp32-wroom-32d-lite` | ✓ | — | — | — |
| `esp32-c3-mini` | ✓ | ✓ | ✓ | ✓ |
| `esp32-c3-mini-lite` | ✓ | — | — | — |
| `esp32-c3-supermini` | ✓ | ✓ | ✓ | ✓ |
| `esp32-c3-supermini-lite` | ✓ | — | — | — |
| `esp32-c6-devkit` | ✓ | ✓ | ✓ | ✓ |
| `esp32-c6-devkit-lite` | ✓ | — | — | — |

*Note: ESP32-C6 variants are experimental.*

Lite variants exclude scale and display code, reducing firmware size and simplifying wiring to just the PN532 NFC module.

### Building a Lite Variant

```bash
pio run -e esp32-wroom-32d-lite
```

---

## Wiring Diagram

The full wiring diagram including all optional components:

![Wiring](./img/Schaltplan.png)

### Example Builds

![Build Example 1](./img/IMG_2589.jpeg)
![Build Example 2](./img/IMG_2590.jpeg)
