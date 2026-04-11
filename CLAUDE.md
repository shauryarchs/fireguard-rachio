# CLAUDE.md

This repository contains Arduino UNO R4 WiFi / embedded C++ code for the FireGuard / EmberSensor project: fire-detection sensors → cloud risk engine → automatic Rachio sprinkler activation.

## Architecture
- **FireGuard.ino** — main loop: sensor poll, cloud send, risk fetch, fire decision, Rachio trigger, LCD/buzzer, serial commands (`s`/`x`)
- **Sensors.h/.cpp** — flame (digital D6, active-low, 5-read debounce), smoke (analog A1, averaged), TMP36 (A0, averaged), buzzer (D5)
- **Cloud.h/.cpp** — EmberSensor HTTP client (`sendToCloud`, `fetchRiskIndexFromCloud`) + local `/status` HTTP server
- **Rachio.h/.cpp** — WiFi connect + RTC sync for TLS, HTTPS requests to `api.rach.io` (health check, start zone, stop all)
- **Config.h** — all tunables: credentials, timing, thresholds. Contains secrets — do not commit real values

## Control flow and timing
- Sensor read + `sendToCloud` runs every `SENSOR_LOOP_INTERVAL_MS` (5000 ms)
- `fetchRiskIndexFromCloud` runs on the same interval, offset by `FETCH_OFFSET_MS` (2500 ms) so the cloud has time to process the latest upload
- Fire decision is `getCurrentRiskIndex() > 7` — the decision is made in the cloud, not on-device
- Sprinkler trigger gated by `TRIGGER_COOLDOWN_MS` (10 s) and auto-trigger lockout after `MAX_AUTO_TRIGGERS` (3), releasable by time (`AUTO_TRIGGER_LOCKOUT_MS`, 1 hr) or serial `x`

## Key invariants (do not break without explicit request)
- Pins in `Sensors.h` (`PIN_FLAME=6`, `PIN_SMOKE=A1`, `PIN_BUZZER=5`, `PIN_TMP36=A0`) and LCD I2C address `0x27`
- Fire threshold `riskIndex > 7` in `FireGuard.ino`
- Flame debounce semantics: all `FLAME_CONFIRM_READS` must be LOW to report flame (`s.flame == 0`)
- TMP36 display conversion applies a `-80 °F` offset to compensate for sensor precision error — present in both `FireGuard.ino` and `Cloud.cpp` (`toDisplayTempF`); keep them consistent
- Cooldown + lockout logic in the fire branch of `loop()`
- `arduinoWiFiConnect()` must finish RTC sync before any HTTPS call, or Rachio TLS will fail

## Build / flash
- Arduino IDE, board: **Arduino UNO R4 WiFi**
- Libraries: `WiFiS3` (built-in), `LiquidCrystal_I2C`, `ArduinoJson`
- Rachio TLS requires `api.rach.io` certificate pre-loaded via **Tools → WiFi Firmware Updater → Upload Certificates**
- Serial monitor at **115200 baud**; `s` starts the zone, `x` stops all + clears lockout



## Goals
- Preserve existing behavior unless a change is explicitly requested
- Prioritize reliability, clarity, and low-risk edits
- Keep the code easy to debug on hardware
- Report all dead code, conflicting logic, race conditions and deadlocks

## Rules
- Do not change pin assignments unless explicitly asked
- Do not change threshold values or fire detection semantics unless explicitly asked
- Do not remove cooldown or safety logic unless explicitly asked
- Prefer small, surgical changes over large rewrites
- Preserve serial logging unless explicitly asked to reduce it
- Avoid dynamic memory usage where possible
- Avoid introducing heavy abstractions that make embedded debugging harder
- Keep loop behavior simple and non-blocking where possible
- When suggesting refactors, explain hardware/runtime risks first

## Code style
- Prefer clear function names
- Keep modules focused by responsibility
- Minimize hidden side effects
- Comment only where it improves maintainability

## When making changes
- First explain the proposed approach
- Then identify files to modify
- Then make the change
- Then summarize exactly what changed and any hardware behavior that could be affected
