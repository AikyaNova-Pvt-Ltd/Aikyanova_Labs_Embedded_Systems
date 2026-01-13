# AikyaNova Labs Embedded Systems - ESP32 QC Testing

This repository provides AikyaNova Lab’s standardized Quality Control (QC) test firmware for ESP32 development boards. These tests are used internally by AikyaNova Labs during hardware validation and incoming quality control (IQC), and are also shared with customers so they can independently verify the ESP32 hardware they receive.

The QC process is divided into Phase One (Physical) and Phase Two (Functional/Firmware).

## What is an ESP32? 

### Overview of ESP32 SoC
The ESP32 is a highly integrated, low-cost system-on-chip (SoC) developed by Espressif Systems, combining a dual-core 32-bit microcontroller with built-in 2.4 GHz Wi-Fi and Bluetooth (Classic + BLE) connectivity. Clocked up to 240 MHz, it offers strong processing capability, flexible GPIO multiplexing and a rich set of peripherals including ADC, DAC, PWM, UART, SPI, I²C and CAN. Multiple low-power modes enable energy-efficient designs, while hardware security features such as secure boot, flash encryption and cryptographic acceleration make the ESP32 suitable for both prototyping and production-grade embedded and IoT applications.

## ESP32 DevKit Overview

The ESP32 DevKit is a development and evaluation board built around the ESP32 SoC. It exposes the ESP32’s GPIOs, power rails, and communication interfaces through standard headers, enabling rapid prototyping, firmware development and hardware validation.
AikyaNova Labs uses ESP32 DevKit boards as the reference platform for QC testing, ensuring consistent and repeatable results across batches.

### ESP32 Technical Specifications

  Processor & Architecture
  - Dual-core 32-bit Xtensa LX6
  - Clock frequency up to 240 MHz
  - Harvard architecture with hardware accelerators

  Memory
  - ROM: 448 KB (dedicated for bootloader and firmware operation) 
  - SRAM: 520 KB (for instruction execution and runtime data)
  - Flash: External SPI flash (typically 4 MB on DevKit boards)

  Wireless Specifications

  - Wi-Fi: 2.4 GHz, IEEE 802.11 b/g/n
  - Bluetooth: Classic Bluetooth + Bluetooth Low Energy (BLE)
  - Integrated PCB or external antenna support (board dependent)

  Power Requirements

  - Input Power:
      + 5V via USB Type-C connector
      + 3.3 V regulated supply for MCU logic
  - Operating Supply Voltage: ~3.3 V logic level
  - Current on I/O Pins: 40 mA
  - Peak current during Wi-Fi transmission can exceed 500 mA

  Operating Conditions

  - Operating temperature: –40°C to +85°C
  - Designed for continuous operation in embedded environments

## ESP32 DevKit Pinout



## Quality Control (QC) Overview

- Phase 1: Physical and Power Checks
- Phase 2: Functional & Connectivity Checks

## What's Included

- `Blink_LED`: LED and basic GPIO checks.
- `ESP32_QC_WIFI_AP`: Wi-Fi Access Point mode test.
- `ESP32_QC_WIFI_STA`: Wi-Fi Station mode test.
- `ESP32_QC_Bluetooth_Android`: Bluetooth validation by Android pairing.
- `ESP32_QC_Bluetooth_IOS`: Bluetooth validation by iOS pairing.



## QC Phases Overview
  ### Phase 1: Physical and Power Checks
  - Board condition and enclosure inspection
  - USB port sturdiness & connectivity
  - Power-on LED behavior
  - GPIO pins validation
  - Buttons click test

  ### Phase 2: Functional & Connectivity Checks
  - Wi-Fi (Access Point & Station mode)
  - Bluetooth (Classic / BLE) testing using mobile devices

  To run the firmware tests (Phase Two), you must set up your software environment. We recommend the Arduino IDE due to its ease of use.

## Setting up the Arduino IDE

1. Install Arduino IDE
   - Download and install the latest Arduino IDE for your OS (Windows, macOS or Linux) from:
   https://www.arduino.cc/en/software

2. Add ESP32 Board Support
   - Open Arduino IDE and go to File > Preferences.
   - In the "Additional Boards Manager URLs" field, paste the following link:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

3. Install ESP32 Board Support
   - Go to Tools > Board > Boards Manager.
   - Search for "esp32".
   - Install the "esp32 by Espressif Systems" package
    
4. Select Board & Port
   - Board: Go to Tools > Board > ESP Arduino > ESP32 Dev Module
   - Port: Go to Tools > Port and select the COM/USB port where ESP32 is connected
   - Upload Speed: 921600 (recommended) or 115200
   
## USB Driver Installation
If your computer does not recognize the board (no COM port appears), you likely need to install the USB-to-UART bridge driver.
- CP210x Driver: Download from Silicon Labs - https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers (Common for DevKit V1/DOIT boards).
Tip: If you are unsure which chip your board uses, look at the small square black chip near the USB port. The text on it will usually say CP2102 or CP2104.

## QC Workflow

1. Power-on and verify serial output.
2. Run `Blink_LED` to validate GPIO and basic firmware loading.
3. Run Wi-Fi AP/STA tests to verify radio and antenna performance.
4. Run Bluetooth tests on both Android and iOS if applicable.
5. Record pass/fail results per board.

## Notes

- These tests are designed for hardware validation, not production firmware.

## Brand and License

All materials are under the AikyaNova™ brand and are licensed for
non-commercial use only. See `LICENSE` for details.
