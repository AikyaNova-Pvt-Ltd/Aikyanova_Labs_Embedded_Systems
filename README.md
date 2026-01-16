# 🛠️ AikyaNova Labs Embedded Systems - ESP32 QC Testing

This repository provides AikyaNova Lab’s standardized Quality Control (QC) test firmware for ESP32 development boards. These tests are used internally by AikyaNova Labs during hardware validation and incoming quality control (IQC) and are also shared with customers so they can independently verify the ESP32 hardware they receive.

The QC process is divided into **Phase One** (Physical) and **Phase Two** (Functional/Firmware).

## ❓ What is an ESP32? 

### 🧠 Overview of ESP32 SoC
The ESP32 is a highly integrated, low-cost system-on-chip (SoC) developed by Espressif Systems, combining a dual-core 32-bit microcontroller with built-in 2.4 GHz Wi-Fi and Bluetooth (Classic + BLE) connectivity. Clocked up to 240 MHz, it offers strong processing capability, flexible GPIO multiplexing and a rich set of peripherals including ADC, DAC, PWM, UART, SPI, I²C and CAN. Multiple low-power modes enable energy-efficient designs, while hardware security features such as secure boot, flash encryption and cryptographic acceleration make the ESP32 suitable for both prototyping and production-grade embedded and IoT applications.

<p align="center">
  <img src="Images/ESP32.jpg" width="400" alt="ESP32 Components">
</p>

## 🎛️ ESP32 DevKit Overview

The ESP32 DevKit is a development and evaluation board built around the ESP32 SoC. It exposes the ESP32’s GPIOs, power rails and communication interfaces through standard headers, enabling rapid prototyping, firmware development and hardware validation.
AikyaNova Labs uses ESP32 DevKit boards as the reference platform for QC testing, ensuring consistent and repeatable results across batches.

<p align="center">
  <img src="Images/ESP32 DevKit.jpg" width="500" alt="Preferences">
</p>

### <ins>📊 ESP32 Technical Specifications</ins>

  **🧠 Processor & Architecture**
  - Dual-core 32-bit Xtensa LX6
  - Clock frequency up to 240 MHz
  - Harvard architecture with hardware accelerators

  **💾 Memory**
  - ROM: 448 KB (dedicated for bootloader and firmware operation) 
  - SRAM: 520 KB (for instruction execution and runtime data)
  - Flash: External SPI flash (typically 4 MB on DevKit boards)

  **📡 Wireless Specifications**
  - Wi-Fi: 2.4 GHz, IEEE 802.11 b/g/n
  - Bluetooth: Classic Bluetooth + Bluetooth Low Energy (BLE)
  - Integrated PCB or external antenna support (board dependent)

  **⚡ Power Requirements**
  - Input Power:
      + 5V via USB Type-C connector
      + 3.3 V regulated supply for MCU logic
  - Operating Supply Voltage: ~3.3 V logic level
  - Current on I/O Pins: 40 mA
  - Peak current during Wi-Fi transmission can exceed 500 mA

  **🌡️ Operating Conditions**
  - Operating temperature: –40°C to +85°C
  - Designed for continuous operation in embedded environments

## 📍 ESP32 DevKit Pinout

<p align="center">
  <img src="Images/ESP32 Pinout Diagram.png" width="600" alt="ESP32 Components">
</p>

The ESP32 DevKit V1 exposes a variety of GPIOs and special peripherals. Below is a quick reference for the most commonly used pins:

| Function | Description | Typical Pins (Default) |
| :--- | :--- | :--- |
| **Power** | Power Supply Rails | **VIN** (5V Input), **3.3V** (Output), **GND** |
| **ADC** | Analog to Digital Converter | GPIO 32, 33, 34, 35, 36 (VP), 39 (VN) |
| **DAC** | Digital to Analog Converter | **DAC1** (GPIO 25), **DAC2** (GPIO 26) |
| **I²C** | I2C Bus (Sensors/Displays) | **SDA** (GPIO 21), **SCL** (GPIO 22) |
| **SPI** | SPI Bus (SD Cards/Screens) | **MOSI** (GPIO 23), **MISO** (GPIO 19), **CLK** (GPIO 18), **CS** (GPIO 5) |
| **UART** | Serial Communication | **TX0** (GPIO 1), **RX0** (GPIO 3) |
| **PWM** | Pulse Width Modulation | All output GPIOs (Software configurable) |

> **⚠️ Important Pin Notes:**
> 1.  **Input Only GPIOs:** GPIOs **34, 35, 36 (VP), and 39 (VN)** do not have internal pull-ups and cannot be used as outputs.
> 2.  **ADC2 vs Wi-Fi:** Do not use ADC2 pins (GPIO 0, 2, 4, 12-15, 25-27) for analog readings when Wi-Fi is active; they will not work. Use ADC1 pins instead.
> 3.  **Boot Strapping:** GPIO **0, 2, 5, 12, and 15** determine boot mode. Avoid connecting external components that pull these pins High/Low during power-up to prevent boot failures.

## 📋 Quality Control (QC) Overview

AikyaNova Labs follows a repeatable and customer-verifiable QC process to ensure ESP32 boards meet electrical, functional and wireless performance expectations.

Incoming QC (IQC) verifies physical condition, connectors, power integrity and basic electrical behavior.
Functional QC validates firmware upload, USB-serial communication, GPIO operation, Wi-Fi and Bluetooth functionality using standardized test firmware.

- **Phase 1:** Physical and Power Checks 🔍
- **Phase 2:** Functional & Connectivity Checks 📶
  
To run the firmware tests (Phase 2), you must set up your software environment. We recommend the Arduino IDE due to its ease of use.

## ⚙️ Setting up the Arduino IDE

1. **Install Arduino IDE** 📥
   - Download and install the latest Arduino IDE for your OS (Windows, macOS or Linux) from:
   `https://www.arduino.cc/en/software
   - Agree to the terms and conditions and install in the default loaction. Click on Finish to complete the installation process.
   - Install the driver softwares if asked by the system.
  
<p align="center">
  <img src="Images/IDE Installation.png" width="600" alt="Installation">
</p>

2. **Add ESP32 Board Support** 🔗
   - Open Arduino IDE and go to File > Preferences.
   - In the "Additional Boards Manager URLs" field, paste the following link:
`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

<p align="center">
  <img src="Images/IDE Preferences.png" width="600" alt="Preferences">
</p>

3. **Install ESP32 Board Support** 📦
   - Go to Tools > Board > Boards Manager.
   - Search for "esp32".
   - Install the "esp32 by Espressif Systems" package Version: `2.0.17` as it the stable version.
  
<p align="center">
  <img src="Images/IDE Board Manager.png" width="600" alt="Board Manager">
    &nbsp; &nbsp;
  <img src="Images/IDE Board Installation.png" width="600" alt="Board Installation">
</p>
    
4. **Select Board & Port** 🔌
   - Board: Go to Tools > Board > ESP Arduino/esp3 > ESP32 Dev Module
     
<p align="center">
  <img src="Images/IDE Tools.png" width="600" alt="Tools">
</p>

   - Port: Go to Tools > Port and select the COM/USB port where ESP32 is connected

<p align="center">
  <img src="Images/IDE Port.png" width="600" alt="Port">
</p>
  
   - Upload Speed: 921600 (recommended) or 115200
   
## 🔌 USB Driver Installation
If your computer does not recognize the board (no COM port appears), you likely need to install the USB-to-UART bridge driver.
- **CP210x Driver:** Download from [Silicon Labs](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers) (Common for DevKit V1/DOIT boards).

<p align="center">
  <img src="Images/CP210x Driver Installation.png" width="600" alt="CP210x">
</p>

Tip: If you are unsure which chip your board uses, look at the small square black chip near the USB port. The text on it will usually say CP2102 or CP2104.

<p align="center">
  <img src="Images/CP 210x.png" width="300" alt="CP 210x">
</p>

- Connect the ESP32 to your PC/Laptop by a USB Type C Data Cable.
- Right click on the `Windows button` and go to `Device Manager > Ports`.

<p align="center">
  <img src="Images/Device Manager.png" width="600" alt="Device Manager">
</p>

- Select `CP2102 USB to UART Bridge Controller` > `Brower for drivers on your computer`, locate the driver file and click on `OK`.

<p align="center">
  <img src="Images/Browse Driver.png" width="600" alt="Driver File">
</p>

- Windows will update your drivers and you can see `Silicon Labs CP210x USB to UART Bridge (COMxx)` under `Ports (COM & LPT)`.
- Now go to Arduini IDE, `Tools > Port > COMxx`.

## 📂 Included Test Firmware

- `Blink_LED`: 💡 LED and basic GPIO checks.
- `ESP32_QC_WIFI_AP`: 📡 Wi-Fi Access Point mode test.
- `ESP32_QC_WIFI_STA`: 🌐 Wi-Fi Station mode test.
- `ESP32_QC_Bluetooth_Android`: 🤖 Bluetooth validation by Android pairing.
- `ESP32_QC_Bluetooth_IOS`: 🍎 Bluetooth validation by iOS pairing.

## 🚦 QC Phases

  - **Phase 1: Physical & Power Checks** 🔍
    + Board condition and enclosure inspection
    + USB port sturdiness & connectivity
    + Power-on validation
    + GPIO pins check
    + Buttons click test
      
  - **Phase 2: Functional & Firmware Checks** 💻
    + GPIO and USB-Serial Communication Validation (Blinking LED)
    + Wi-Fi Testing (Access Point & Station mode)
    + Bluetooth Testing (Classic/BLE testing using mobile devices)
   
## 🔄 QC Workflow

**Recommended Test Sequence:**
1. Power ON and verify USB-serial detection
2. Upload and run Blink_LED firmware
3. Execute Wi-Fi tests (AP → STA)
4. Execute Bluetooth tests (Android / iOS as applicable)

Record **PASS/FAIL** ✅/❌

## 🛠️ Troubleshooting Guide

 **🚫 Board Not Detected**
  - Check USB cable (data cable required)
  - Verify correct COM/USB port selection
  - Install appropriate USB-to-UART driver (CP210x / CH340)
  - Try a different USB port or system

 **❌ Upload Failures**
  - Ensure correct board is selected in Arduino IDE
  - Hold BOOT button (if present) during upload
  - Reduce upload speed if errors persist
  - Close other applications using the serial port

**📶 Wi-Fi / Bluetooth Failures**
  - Verify antenna is not damaged or obstructed
  - Avoid metal enclosures during testing
  - Ensure only one board is powered during RF tests
  - Check serial logs for connection errors

**⚠️ Common ESP32 Pitfalls**
  - Insufficient USB power causing brownouts
  - Using restricted GPIOs unintentionally
  - Expecting stable Wi-Fi performance in noisy RF environments

## ⚠️ Safety and Handling
  - Do not exceed recommended voltage levels
  - Avoid hot-plugging external peripherals
  - Use reliable USB power sources
  - Avoid repeated force on USB connector

## 📝 Notes

- These tests are designed for hardware validation, not production firmware.

## 📜 Brand and License

All materials are under the AikyaNova™ brand and are licensed for
non-commercial use only. See `LICENSE` for details.
