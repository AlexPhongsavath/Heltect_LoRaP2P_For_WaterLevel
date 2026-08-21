# Heltec LoRa P2P — Water Level Monitor

Point-to-point LoRa wireless communication between two **Heltec WiFi LoRa 32 V2** (ESP32) boards for real-time water level monitoring — no internet, no cloud, no gateway needed.

One board acts as the **sensor node**: it reads distance from an ultrasonic sensor (HC-SR04 via Modbus or direct trigger), computes the water level, and transmits the value over LoRa. The other board acts as the **display node**: it receives the packet and shows the reading on the onboard OLED display.

## Hardware

| Component | Role |
|---|---|
| Heltec WiFi LoRa 32 V2 × 2 | Microcontroller + LoRa transceiver |
| Ultrasonic sensor (HC-SR04) | Distance / water level measurement |
| OLED 128×64 (SSD1306, built-in) | Live reading display |

## Dependencies

Managed via PlatformIO (`platformio.ini`):

| Library | Purpose |
|---|---|
| `heltecautomation/Heltec ESP32 Dev-Boards` | Board support, LoRa radio, OLED |
| `adafruit/Adafruit SSD1306` | OLED driver |
| `4-20ma/ModbusMaster` | Modbus RTU communication |
| `ericksimoes/Ultrasonic` | Ultrasonic sensor helper |

## Getting Started

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
2. Clone the repo and open the project folder
3. Connect your Heltec board via USB
4. Build and upload:

```bash
pio run --target upload
```

5. Open the serial monitor at 115200 baud:

```bash
pio device monitor
```

## Project Structure

```
├── platformio.ini      # board + library config
├── src/
│   └── main.ino        # main sketch (sender or receiver logic)
├── include/            # header files
├── lib/                # local libraries
└── test/               # unit tests
```

## Built With

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus&logoColor=white)
![PlatformIO](https://img.shields.io/badge/PlatformIO-F5822A?style=flat&logo=platformio&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat&logo=espressif&logoColor=white)
