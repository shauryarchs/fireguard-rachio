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
- Sprinkler trigger gated only by `TRIGGER_COOLDOWN_MS` (10 s); serial `x` stops all watering

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

## Shared EmberSensor rules

> This section is identical in every EmberSensor repository. Update all four copies together.

### Related repositories
All under `github.com/shauryarchs`:
- `embersensor-site` — website (`docs/`) and Cloudflare Worker API (`worker/`). The hub: every device and client talks only to the Worker. **Issues for all repositories are tracked here.**
- `embersensor-ios` — iOS app; reads `/api/status`, `/api/fires`, `/api/calfire-fires`.
- `sprinkler-slide-pan-tilt` — Arduino Nano ESP32 firmware for the sprinkler slider and pan/tilt arm; polls `/api/motor/command`, pushes `/api/motor/state`.
- `fireguard-rachio` — Arduino UNO R4 WiFi sensor node; POSTs readings to `/api/update`, reads `riskIndex` from `/api/status`, and starts the Rachio sprinkler zone when `riskIndex > 7`.

Changing a Worker endpoint or response field can break the website, the iOS app, and both firmware projects — check all consumers.

### Work workflow (required for all new work)
1. **Create an issue first** in `embersensor-site` describing the task, scope, and acceptance criteria — even if the work is in another repository.
2. **Before making changes, create a branch** in each repository involved, named `shaurya/<issue-number>_<issue-title>` using the `embersensor-site` issue number and a short, lowercase, hyphenated title. Example: `shaurya/42_add-sprinkler-controls`.
3. Complete the work on those branches and run the relevant checks.
4. **Open a pull request in each affected repository**, referencing the issue (e.g. `shauryarchs/embersensor-site#42`) and summarizing the changes and validation.
5. **Do not merge until the repository owner explicitly approves.**
6. After approval, merge each PR into its `main` branch. Close the issue once all work covered by it has been merged.

### No AI attribution
Do not include any reference to "Claude" (or other AI-tool attribution) in code, comments, documentation, filenames, commit messages, branch names, issues, pull requests, or other project artifacts. This includes `Co-Authored-By` trailers and "Generated with …" footers. Commit messages describe the actual change only. Before committing or pushing, check that no such attribution or generated signature has been added.

The one intentional exception is this instructions file (`CLAUDE.md`), which stays tracked so every contributor shares the same instructions.

### Safety
- Keep any **new automatic sprinkler activation or motor movement disabled** until it has been reviewed and validated with the repository owner.
- `flame` is **active-low**: `0` means flame detected. It forces `riskIndex = 10`, which makes FireGuard start the real sprinkler zone. Test or "reset" payloads sent to `/api/update` must use `"flame": 1` unless deliberately simulating a fire.
- Never commit or expose secrets, tokens, Wi-Fi credentials, or access codes.

### Hardware work
- Confirm the exact part model and interface before implementing (ask for a product link or photo if ambiguous).
- Verify compatibility with the actual board — voltage levels, power, memory, and existing pin assignments — using the manufacturer's documentation.
- PRs for hardware changes include: a wiring table (component pin → board pin), power requirements and extra components, mounting guidance, required libraries and configuration, and a step-by-step initial test with expected results.
- Clearly separate checks done in software from tests that require the physical hardware, and document assumptions and limitations.

### Reviews and findings
- Support findings with file paths and function names.
- Distinguish what the code confirms from what is inferred. Documentation and comments alone are not proof that something is implemented.
