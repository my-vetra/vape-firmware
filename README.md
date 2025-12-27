<div align="center">

# Vetra — ESP32-C3 Vape Device Firmware

Built by Adeesh Devanand

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32C3-orange)](https://platformio.org/)
[![Arduino](https://img.shields.io/badge/Framework-Arduino-blue)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

</div>

## Overview
Production firmware for an ESP32-C3-based device focused on reliability, safety, and real-time interaction. It provides BLE control and telemetry, robust puff counting via a state machine, safe interrupt handling for input capture, deep sleep for ultra-low power consumption, and utilizes flash memory for durable state persistence across resets and sleep cycles.

The project emphasizes embedded best practices: minimal ISR work, debounced inputs, watchdog-friendly loops, and clear module boundaries.

![Architecture Diagram](docs/architecture-diagram.svg)

> Note: Diagram is a placeholder and will be replaced with final visuals.

## Key Features
- **BLE service:** Control and telemetry (timers, state, logs) via a custom BLE service.
- **Puff counting:** Debounced edge detection and a **finite state machine** governing valid phases and lockdown behavior.
- **Safe interrupts:** ISRs set lightweight flags; all heavy work happens in the main loop to avoid watchdog resets.
- **Deep sleep:** Configured wake sources with persisted timestamps so the device maintains continuity.
- **Persistence:** Records last epoch and critical parameters with `PersistenceManager`.
- **Logging pipeline:** `Logger` + `LogBuffer` stream events over BLE and/or serial without blocking.
- **Unit tests:** Core modules covered with PlatformIO’s `unity` framework.

## Getting Started (Windows)
### Prerequisites
- **VS Code** with the **PlatformIO** extension
- **ESP32-C3 DevKitM-1** (board `esp32-c3-devkitm-1`)
- USB cable and driver (CP210x/CH340 if applicable)

### Clone
```cmd
git clone https://github.com/<your-username>/vape-firmware.git
cd vape-firmware
```

### Configure
- Open the project in VS Code.
- Review `platformio.ini` (environments: `vetra-dev`, `vetra-release`).
- Adjust pins in `src/Device.h` to match your hardware (`BUTTON_PIN`, `HEAT_PIN`, coil control pin).

### Build
```cmd
pio run -e vetra-dev
```
Alternatively, using the PlatformIO Core path on Windows:
```cmd
%USERPROFILE%\.platformio\penv\Scripts\platformio.exe run --environment vetra-dev
```

### Flash
```cmd
pio run --target upload -e vetra-dev
```

### Monitor
```cmd
pio run --target monitor -e vetra-dev
```
Default baud rates are configured in `platformio.ini` (`upload_speed` / `monitor_speed`).

## How It Works
- **Interrupts:** Pin changes on `HEAT_PIN` trigger a tiny ISR that sets `volatile` flags (e.g., `s_puff_rising_pending`, `s_puff_falling_pending`). No logging or complex work occurs in the ISR.
- **Main loop:** In `App::loop()`, flags are consumed atomically, invoking `handlePuffCountRising()` / `handlePuffCountFalling()` and advancing the state machine.
- **Debounce:** `DebounceManager` smooths contact chatter; active decisions run in task context, not inside ISRs.
- **Deep sleep:** When BLE is inactive beyond a timeout, the firmware persists the current epoch, configures wake sources, and enters deep sleep.
- **BLE:** A lightweight service exposes control and telemetry characteristics. Logs are streamed via `BLEManager::pumpLogs()`.

![BLE Flow](docs/ble-flow.svg)

> Note: BLE flow diagram is a placeholder.

## Project Structure
- `src/` — Main application logic (`main.cpp`, `App.cpp/.h`, `Device.cpp/.h`)
- `lib/BLE/` — BLE communication (`BLEManager.cpp/.h`)
- `lib/Logger/` — Logging and buffered output (`Logger.cpp/.h`, `LogBuffer.cpp/.h`)
- `lib/StateMachine/` — Puff counting + lockdown FSM (`StateMachine.cpp/.h`)
- `lib/Utils/` — Utilities (`Debounce.cpp/.h`, `PersistenceManager.cpp/.h`, `Timer.cpp/.h`)
- `include/` — Shared headers
- `test/` — Unit tests (BLE, device, sleep manager, state machine)
- `docs/` — Diagrams and technical documentation (placeholders included)

## Configuration
- **Build flags:** See `platformio.ini` for runtime parameters like `MAX_PUFFS`, `PHASE_DURATION_SECONDS`, `NUM_PHASES`, and `MIN_PUFF_DURATION_MILLISECONDS`.
- **Pins:** Update `Device.h` to match your board and wiring.
- **Log level:** Tune `LOG_LEVEL` via build flags; production defaults to concise info.

## Usage Tips
- Keep ISRs minimal; avoid logging or singleton calls inside ISRs.
- If modifying debounce behavior, prefer processing in `loop()` or timer callbacks.
- Use the BLE logs for telemetry during development; fall back to serial monitor if BLE is off.

## Testing
Run unit tests per environment:
```cmd
pio test -e vetra-dev
```
Focus on specific modules by filtering tests or leveraging `test_ignore` in `platformio.ini`.

## Troubleshooting
- **Watchdog resets:** Remove heavy work from ISRs; do logging in `loop()`.
- **BLE not advertising:** Ensure `BLEManager::startService()` runs after interrupts attach; verify board power and reset.
- **No wake from deep sleep:** Confirm wake source in `setup()` and the correct `BUTTON_PIN` wiring.

## Roadmap
- Finalize BLE characteristics documentation and public schema.
- Add richer telemetry and OTA update support.
- Replace placeholder diagrams with final architecture visuals.

## License
This project is released under the MIT License. See `LICENSE`.

## Credits
Built by **Adeesh Devanand**.

If you use or evaluate this firmware, consider linking back to this repository on your project page.