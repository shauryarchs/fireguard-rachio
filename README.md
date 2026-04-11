# FireGuard – Arduino UNO R4 WiFi + Rachio Integration

FireGuard is an Arduino UNO R4 WiFi project that combines fire detection sensors with a smart sprinkler system for early wildfire prevention.

Sensor data is uploaded to [EmberSensor](https://embersensor.com) every 5 seconds, which computes a cloud-side risk index. When the risk index exceeds the trigger threshold, FireGuard automatically starts a configured Rachio sprinkler zone via the Rachio Cloud API and alerts locally via LCD and buzzer.

---

## How It Works

```
Sensors (flame, smoke, temp)
    ↓  every 5s
sendToCloud()  →  POST embersensor.com/api/update
    ↓  2.5s later (after cloud has time to process)
fetchRiskIndexFromCloud()  →  GET embersensor.com/api/status
    ↓
riskIndex > 7?
    ↓  yes
rachioStartZone()  →  PUT api.rach.io/1/public/zone/start
    + LCD alert + buzzer
```

The fire/no-fire decision is made by the EmberSensor cloud service, not locally. The Arduino collects and uploads sensor readings and acts on the returned risk index.

---

## Features

- **Multi-sensor detection**
  - Flame sensor (digital, active-low) with 5-read debounce — rejects sunlight glints and transient spikes
  - Smoke sensor (analog, A1) with 5-sample ADC averaging — reduces WiFi-induced ADC noise
  - TMP36 temperature sensor (analog, A0) with 5-sample ADC averaging

- **Cloud-based risk engine**
  - Sensor data POSTed to EmberSensor every 5 seconds
  - Risk index fetched 2.5 seconds later, giving the cloud time to process
  - Sprinkler fires only when `riskIndex > 7`

- **Automatic sprinkler activation**
  - Triggers Rachio zone via HTTPS when risk threshold is exceeded
  - 10-second cooldown between triggers

- **Local alerts**
  - 16x2 I2C LCD shows `All Clear / System Ready` or `Fire Detected! / Sprinklers ON`
  - Buzzer pulses on each fire detection event

- **Manual override** via Serial Monitor (115200 baud)
  - `s` — start sprinkler zone manually
  - `x` — stop all watering

---

## Project Structure

```
FireGuard/
├── FireGuard.ino     # Main loop: sensor polling, fire decision, LCD, serial commands
├── Config.h          # All tunable constants (WiFi, Rachio credentials, timing, thresholds)
├── Sensors.h/.cpp    # Sensor reading with debounce and ADC averaging; buzzer control
├── Rachio.h/.cpp     # WiFi connect, TLS time sync, Rachio API calls
└── Cloud.h/.cpp      # EmberSensor HTTP client: sendToCloud, fetchRiskIndex, local /status server
```

---

## Hardware Requirements

| Component | Detail |
|---|---|
| Arduino UNO R4 WiFi | Primary microcontroller |
| Flame sensor | Digital output, wired to D6 (active-low) |
| Smoke sensor | Analog output, wired to A1 (MQ-x or similar) |
| TMP36 | Analog temperature sensor, wired to A0 |
| I2C LCD | 16x2 display, address `0x27` (SDA/SCL) |
| Passive buzzer | Wired to D5 |
| Rachio sprinkler system | Cloud API access required |

---

## Setup Instructions

### 1. Install Arduino IDE

- Select board: **Arduino UNO R4 WiFi**
- Install libraries:
  - `WiFiS3` (built-in for UNO R4)
  - `LiquidCrystal_I2C`
  - `ArduinoJson`

### 2. Add TLS Certificate for Rachio

The Rachio API uses HTTPS. The Arduino's SSL stack requires the server certificate to be pre-loaded:

- In Arduino IDE: **Tools → WiFi Firmware Updater**
- Update firmware if needed
- Under **Upload Certificates**, add `api.rach.io`

### 3. Configure Config.h

Open `Config.h` and fill in your credentials and settings:

```cpp
#pragma once
#define WIFI_SSID             "your-wifi-name"
#define WIFI_PASS             "your-wifi-password"
#define RACHIO_API_TOKEN      "your-rachio-api-token"
#define RACHIO_ZONE_ID        "your-zone-uuid"
#define RACHIO_DEVICE_ID      "your-device-uuid"

#define DURATION_SECONDS          10      // how long sprinkler runs per trigger
#define SENSOR_LOOP_INTERVAL_MS   5000    // how often sensors are read and uploaded
#define FETCH_OFFSET_MS           2500    // delay between upload and risk index fetch
#define TRIGGER_COOLDOWN_MS       10000UL // min time between Rachio triggers

#define FLAME_CONFIRM_READS       5       // consecutive LOW reads required to confirm flame
#define FLAME_CONFIRM_DELAY_MS    10      // ms between flame confirmation reads
#define ADC_SAMPLE_COUNT          5       // samples averaged per analog read
#define ADC_SAMPLE_DELAY_MS       2       // ms between ADC samples
```

### 4. Wire the Hardware

| Sensor | Pin |
|---|---|
| Flame sensor | D6 |
| Smoke sensor | A1 |
| TMP36 | A0 |
| Buzzer | D5 |
| LCD SDA/SCL | I2C bus |

### 5. Upload and Run

- Compile and upload via Arduino IDE
- Open Serial Monitor at **115200 baud**

---

## Example Serial Output

```
IP Address: 192.168.1.42
HTTP server started
Zone ID: 22b157d1-...
Device ID: 47fbcc2c-...
Run time: 10 seconds
Checking Rachio API...
Rachio connection OK
System Ready

Sensor Temp=72.4F  Flame=1 Smoke=312
☁️ riskIndex from cloud = 2
No Fire Risk Detected...

Sensor Temp=74.1F  Flame=0 Smoke=587
☁️ riskIndex from cloud = 9
Fire Risk Detected!

Activating sprinkler zone 22b157d1-... for 10 seconds
Sprinkler started
```

---

## Security Note

`Config.h` contains sensitive credentials (WiFi password, Rachio API token). Do not commit this file to a public repository. For open-source sharing, provide a `Config.example.h` with placeholder values and add `Config.h` to `.gitignore`.
