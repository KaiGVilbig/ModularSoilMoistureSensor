# Modular Soil Moisture Sensor

A modular, battery-powered soil moisture monitoring system: a BLE sensor node reports readings to a Raspberry Pi bridge over MQTT, with an Android app for live readings, configuration, and threshold alerts.

## Components

- [`firmware/`](firmware/) — Seeed XIAO nRF52840 sensor node. Reads capacitive soil moisture sensors, sends readings over BLE, accepts config over BLE.
- [`pi-bridge/`](pi-bridge/) — Raspberry Pi 3B+ Python service. BLE central that republishes readings to a local MQTT broker and evaluates alert thresholds.
- [`android/`](android/) — Android app. MQTT client for live readings, sensor/threshold configuration, and local notifications.

## Status

Early scaffolding — architecture and BLE protocol are designed, no code implemented yet. See [CLAUDE.md](CLAUDE.md) for full architecture, hardware details, and build order.

## Hardware

- Seeed XIAO nRF52840 (BLE only, no WiFi)
- Capacitive soil moisture sensors, up to 6 per node (one per ADC pin)
- Single MOSFET-gated power rail shared across sensors
- 3.7V 1000mAh LiPo battery, charged via onboard USB-C
- Raspberry Pi 3B+ as BLE-to-MQTT bridge

## License

See [LICENSE](LICENSE).
