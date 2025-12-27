<div align="center">

# Vetra Firmware (Internal)

Proprietary and confidential. Do not distribute.

</div>

## Overview
Internal firmware for ESP32-C3 device. Features BLE connectivity, timers, puff counting, deep sleep, and robust state persistence.

## Project Structure
- `src/` — Main application logic (`main.cpp`, `App.cpp/h`, `Device.cpp/h`)
- `lib/BLE/` — BLE communication (`BLEManager.cpp/h`)
- `lib/StateMachine/` — State machine for puff counting and lockdown (`StateMachine.cpp/h`)
<!-- - `lib/Memory/` — Persistent storage (`mem_sys.cpp/h`) -->
- `lib/Utils/` — Utilities (`GlobalVars.cpp/h`, `Logger.cpp/h`, `SleepManager.cpp/h`)
- `test/` — Unit tests for all modules

## Features
- BLE service for timer, NTP sync, and device state
- Persistent and coil timers
- Puff counting and lockdown state machine
- Deep sleep with state saved to NVRAM and RTC memory
- Power outage detection and recovery
- Modular logging via `Logger` interface
- Unit tests for all modules

## Build & Flash
1. Install PlatformIO
2. Build (debug): `pio run -e esp32-c3-devkitm-1`
3. Build (release): `pio run -e esp32-c3-devkitm-1-release`
4. Upload: `pio run --target upload -e esp32-c3-devkitm-1`

## Customization
- Update pin definitions in `Device.h`
- Extend logging in `Logger.cpp`
- Add new BLE characteristics in `BLEManager.cpp/h`