# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repo state

Pre-code. `/firmware`, `/pi-bridge`, `/android` exist as empty scaffolding (placeholder READMEs only) — no actual firmware, Pi bridge, or Android sources yet, so there are no build/lint/test commands to document. The architecture below reflects the design from a prior scaffolding session and is the intended layout once code lands. Update this section (and add real commands) once the first code is committed.

## Project overview

A modular soil moisture sensor system with three components:

1. **Sensor node firmware** (Seeed XIAO nRF52840) — reads capacitive soil moisture sensors, sends readings over BLE, accepts config over BLE
2. **Home server bridge** (Raspberry Pi 3B+, Python) — BLE central that connects to the sensor node, republishes readings to a local MQTT broker, evaluates thresholds, triggers alerts
3. **Android app** — MQTT client showing live readings, lets the user configure sensor pins/calibration/thresholds, shows local notifications on threshold breach

Solo hobby project. Repo: `KaiGVilbig/ModularSoilMoistureSensor`.

## Hardware

- **MCU**: Seeed XIAO nRF52840 (not the Sense variant unless noted) — 6 analog pins (A0–A5) usable as ADC, so **6 is the hard ceiling on simultaneous sensors**
- **Sensors**: capacitive soil moisture sensors (read HIGH when dry, LOW when wet — opposite of resistive sensors)
- **Power switching**: all sensors share one MOSFET/transistor gated by a single GPIO (`SENSOR_POWER_PIN`), so sensors are only powered briefly during a read — this matters for long-term corrosion resistance, don't wire sensors to always-on power
- **Battery**: 3.7V 1000mAh LiPo into the XIAO's BAT+/BAT- pads, charged via onboard USB-C (BQ25101 charger)
- **Bridge**: Raspberry Pi 3B+ — has built-in WiFi + Bluetooth 4.2 (BLE), no USB dongle needed
- **No WiFi on the sensor node** — the XIAO nRF52840 is BLE-only, hence the Pi as a bridge to get data onto the local network / MQTT

## Firmware architecture (`/firmware`)

Modular by design — adding/reconfiguring a sensor should never require a firmware recompile, only a config push over BLE.

- `Config.h` — pin map, constants, `SensorConfig` struct (per-slot: enabled, pin index, name, calibration raw_dry/raw_wet, threshold)
- `SoilSensor.h/.cpp` — one instance per slot; reads its pin, oversamples, converts raw ADC → moisture %
- `SensorManager.h/.cpp` — owns the array of `SoilSensor`s, handles shared power-gating, persists config to flash (LittleFS), builds/parses JSON payloads
- `BLEHandler.h/.cpp` — Nordic UART Service (BLE UART) setup, routes incoming JSON commands to `SensorManager`
- Main `.ino` — setup/loop, sample-and-send cycle

**BLE protocol** (newline-terminated JSON over Nordic UART Service):

Device → Pi (periodic + on-demand):
```json
{"type":"reading","uptime_ms":123456,"batt_mv":3850,
 "sensors":[{"slot":0,"name":"Tomato Bed","raw":512,"pct":45.2,"low":false}]}
```

Pi → Device commands:
```json
{"cmd":"get_config"}
{"cmd":"set_config","sensors":[{"slot":0,"enabled":true,"name":"...","raw_dry":800,"raw_wet":300,"low_threshold":20}]}
{"cmd":"read_now"}
```

**Known gaps** (see firmware README for details — flag these if working nearby):
- Sleep loop is currently a placeholder busy-wait, not real nRF52 low-power sleep — needs replacing before battery-life claims are trustworthy
- Battery voltage reading (`readBatteryMillivolts()`) is a best-effort stub; the read-enable pin and divider ratio differ across XIAO board revisions — verify against Seeed's current wiki before trusting it
- No BLE pairing/bonding — open BLE, acceptable for a home LAN device but worth knowing

## Pi bridge architecture (`/pi-bridge`)

- Python service using `bleak` (not `bluepy` — unmaintained) as BLE central, connects to the XIAO, subscribes to its UART TX characteristic
- Republishes readings to a local **Mosquitto** MQTT broker, one topic per sensor
- Threshold evaluation lives on the Pi (source of truth), not the device — the device's own `low` flag in readings is redundant/best-effort only
- Pi holds the "desired" sensor config and pushes it to the node via `set_config` on connect or on change (Pi is the source of truth for config, synced down to the device)
- Reconnection handling needed — BLE central should auto-retry if the node drops out of range or resets

## Android app architecture (`/android`)

- MQTT client (subscribes to the Pi's broker) for live readings
- Config screens for pin assignment, calibration (guided dry/wet flow), and thresholds — writes go back through the Pi, not directly to the device
- Local notifications while app is running/backgrounded on threshold-crossing MQTT messages
- Anywhere/background notifications (app fully closed, off local network) are a stretch goal — would need Tailscale (Pi reachable off-LAN) or FCM push triggered by the Pi, not yet built

## Project tracking

Work is tracked as GitHub Issues + Milestones (6 milestones = epics: Sensor Node Firmware, Home Server Bridge, Notifications, Android App, Hardware & Power, Integration & Testing) on a GitHub Project board, with a numeric `Priority` field reflecting recommended build order. Issue titles follow a prefix convention: `SN-#` (firmware), `PB-#` (Pi bridge), `NT-#` (notifications), `APP-#` (Android), `HW-#` (hardware), `QA-#` (testing).

**Recommended build order** (thin end-to-end slice first, then widen):
1. Single sensor read + calibrate (`SN-1`, `SN-2`) → wire it (`HW-1`) → BLE send (`SN-6`, `SN-7`) → Pi receives (`PB-1`, `PB-2`) → smoke test (`QA-1`)
2. Multi-sensor + config sync (`SN-3`, `SN-8`, `SN-5`, `PB-7`, `QA-2`)
3. Notifications (`PB-4`–`PB-6`, `NT-1`, `QA-3`)
4. App (`APP-1`–`APP-5`)
5. Power/hardening/stretch (`SN-4`, `SN-9`, `SN-10`, `HW-2`–`HW-4`, `PB-3`, `QA-4`, `QA-5`, stretch-labeled issues last)

When picking up work, check the Project board's Priority-sorted table for what's next rather than assuming epic order.

## Conventions

- User is writing all application code themselves (this was scaffolded by Claude in a prior chat, then handed off) — favor reviewing/discussing over rewriting large chunks unless asked
- Capacitive sensor calibration is per-physical-unit (raw ADC values vary between individual sensors) — don't hardcode calibration constants as if they're universal
- Config is always Pi-owned / pushed down to the device, not device-initiated — keep that direction consistent if extending the protocol