# AikyaNova Labs Embedded Systems - ESP32 QC Testing

This repository provides AikyaNova Lab’s standardized Quality Control (QC) test firmware for ESP32 development boards. These tests are used internally by AikyaNova Labs during hardware validation and incoming quality control (IQC), and are also shared with customers so they can independently verify the ESP32 hardware they receive.

The QC process is divided into Phase One (Physical) and Phase Two (Functional/Firmware).

## What's Included

- `ESP32_QC_USB_Serial_Check`: USB-to-Serial communication test.
- `Blink_LED`: LED and basic GPIO checks.
- `ESP32_QC_WIFI_AP`: Wi-Fi Access Point mode test.
- `ESP32_QC_WIFI_STA`: Wi-Fi Station mode test.
- `ESP32_QC_Bluetooth_Android`: Bluetooth validation by Android pairing.
- `ESP32_QC_Bluetooth_IOS`: Bluetooth validation by iOS pairing.

## 

## Prerequisites

- Arduino IDE installed and configured.
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
