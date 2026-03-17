
# 🔥 FireGuard Modular – Arduino UNO R4 WiFi + Rachio Integration

FireGuard is an Arduino UNO R4 WiFi project that combines **fire detection sensors** with a **smart sprinkler system** for early wildfire prevention.

When flame, smoke, or high temperature is detected, FireGuard automatically starts a configured **Rachio sprinkler zone** via the Rachio Cloud API, while providing **local alerts** on an LCD and buzzer.

---

## ✨ Features

* **Multi-sensor detection**

  * Flame sensor
  * Smoke sensor (digital)
  * TMP36 temperature sensor

* **Automatic sprinkler activation**

  * Starts a Rachio zone immediately when fire is detected
  * Cooldown prevents repeated triggers within a short interval

* **Local alerts**

  * LCD screen shows status (“All Clear” / “Fire Detected”)
  * Buzzer alarm on fire detection

* **Manual override** via Serial Monitor

  * `s` → Start sprinkler zone
  * `x` → Stop all watering

* **Modular structure**

  * `Config.h` → Wi-Fi, Rachio API token, zone/device IDs, thresholds
  * `Sensors.h` / `Sensors.cpp` → sensor reading + buzzer alert
  * `Rachio.h` / `Rachio.cpp` → Wi-Fi, TLS, Rachio API calls
  * `FireGuard.ino` → main application loop & LCD UI

---

## 📂 Project Structure

```
FireGuard_Modular/
├── FireGuard.ino      # Main application logic
├── Config.h           # Wi-Fi & Rachio configuration
├── Sensors.h          # Sensor interface header
├── Sensors.cpp        # Sensor implementation
├── Rachio.h           # Rachio API interface header
├── Rachio.cpp         # Rachio API implementation
```

---

## ⚡ Hardware Requirements

* Arduino UNO R4 WiFi
* Flame sensor (digital output)
* Smoke sensor (digital output, e.g., MQ-x with comparator board)
* TMP36 analog temperature sensor
* I²C LCD display (16x2, address `0x27`)
* Passive buzzer
* Rachio sprinkler system (with API key, zone ID, and device ID)

---

## 🛠 Setup Instructions

1. **Install Arduino IDE** (latest version)

   * Select board: **Arduino UNO R4 WiFi**
   * Install the **WiFiS3** library (built-in for UNO R4)
   * Install the **LiquidCrystal_I2C** library

2. **Wi-Fi Module Setup**

   * In Arduino IDE, open **Tools → WiFi Firmware Updater**
   * Update firmware if needed
   * Add certificate for `api.rach.io` under **Upload Certificates**

3. **Configure FireGuard**

   * Open `Config.h`
   * Enter your Wi-Fi SSID & password
   * Enter your Rachio API token, Zone ID, and Device ID
   * Adjust thresholds (e.g., temperature trigger) as needed

```
#pragma once
#define WIFI_SSID        "wifi-name"
#define WIFI_PASS        "wifi-password"
#define RACHIO_API_TOKEN "RACHIO_API_TOKEN"
#define RACHIO_ZONE_ID   "RACHIO_ZONE_ID"
#define RACHIO_DEVICE_ID "RACHIO_DEVICE_ID"
#define DURATION_SECONDS   10
#define TEMP_THRESHOLD_C   65
#define WEATHER_LOOP_INTERVAL_MS   5000
#define SENSOR_LOOP_INTERVAL_MS   5000
#define TRIGGER_COOLDOWN_MS 10000UL
#define WEATHER_API_KEY "your-weather-key"
#define WEATHER_LAT 34.1064
#define WEATHER_LON -117.5931
```

4. **Wire the Sensors**

   * Flame sensor → D6
   * Smoke sensor → D7
   * Buzzer → D5
   * TMP36 → A0
   * LCD → I²C (SDA, SCL)

5. **Upload and Run**

   * Compile & upload the project
   * Open Serial Monitor at `115200` baud
   * Monitor logs for connectivity and sensor status

---

## 🖥 Example Serial Output

```
[ready] Zone ID: 32b157d1-1be0-4abb-8092-a4ce2bc046e8
[ready] Device ID: 47fbcc2c-ab76-49e5-a5f9-f160d0a52175
[rachio] Connectivity & auth check: GET /1/public/person/info
[tls] TLS connected.
[rachio] ✅ API reachable and token accepted.
[sens] flame=0 smoke=1 tempC=41.9 fireDetected=YES
[rachio] FIRE DETECTED → starting zone...
[rachio] ✅ Zone start accepted.
```

---

## 🔒 Security Note

* Keep your `RACHIO_API_TOKEN` private — don’t commit it directly to public repos.
* For open source release, use an `.example` config file instead.
