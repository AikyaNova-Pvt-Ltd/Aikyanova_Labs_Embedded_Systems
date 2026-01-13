# AikyaNova Labs Embedded Systems - ESP32 QC Testing

This repository contains AikyaNova's internal quality control (QC) firmware used
to validate ESP32-based boards during manufacturing. Each folder targets a
specific QC test (LED, Wi-Fi AP/STA, Bluetooth, etc.). The goal is to provide
repeatable bring-up checks for hardware verification.

## What's Included

- `Blink_LED`: LED and basic GPIO sanity checks.
- `ESP32_QC_WIFI_AP`: Wi-Fi Access Point mode tests.
- `ESP32_QC_WIFI_STA`: Wi-Fi Station mode tests.
- `ESP32_QC_Bluetooth_Android`: Bluetooth validation for Android pairing.
- `ESP32_QC_Bluetooth_IOS`: Bluetooth validation for iOS pairing.

## Prerequisites

- ESP-IDF installed and configured.
- USB drivers for your ESP32 board.
- A supported ESP32 development board connected via USB.

## Quick Start

1. Choose the QC test folder you want to run.
2. Build and flash using ESP-IDF:

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

3. Follow any on-screen prompts in the serial monitor.

## QC Workflow (Suggested)

1. Power-on and verify serial output.
2. Run `Blink_LED` to validate GPIO and basic firmware loading.
3. Run Wi-Fi AP/STA tests to verify radio and antenna performance.
4. Run Bluetooth tests on both Android and iOS if applicable.
5. Record pass/fail results per board.

## Notes

- These tests are designed for manufacturing QC, not production firmware.
- Adjust GPIO pins and credentials as required for your specific hardware.

## Brand and License

All materials are under the AikyaNova™ brand and are licensed for
non-commercial use only. See `LICENSE` for details.
