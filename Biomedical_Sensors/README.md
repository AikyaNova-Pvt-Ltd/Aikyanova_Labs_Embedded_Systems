# 🩸 AikyaNova Labs Embedded Systems - Biomedical Sensors

This directory provides AikyaNova Lab’s standardized firmware, example sketches, and documentation for integrating various biomedical sensors into embedded systems projects. These modules are primarily utilized for health, fitness monitoring, and physiological data acquisition applications, including heart rate, blood oxygen saturation (SpO2), and electrocardiogram (ECG) tracking.

## ❓ What are Biomedical Sensors? 

### 🧠 Overview
Biomedical sensors are highly specialized electronic components designed to detect and measure physiological signals from the human body. By converting biological responses (such as the optical absorption of blood or the electrical activity of the heart) into readable electrical signals, these sensors allow microcontrollers like the ESP32 to process, analyze, and transmit health data in real-time. 

## 🎛️ Supported Sensor Modules Overview

AikyaNova Labs uses specific reference sensors for reliable health-data acquisition. Below are the primary modules supported in this directory:

### <ins>🔴 MAX30102 (Pulse Oximeter and Heart-Rate Sensor)</ins>
The MAX30102 is an integrated pulse oximetry and heart-rate monitor module. It includes internal LEDs (Red and IR), photodetectors, optical elements, and low-noise electronics with ambient light rejection. It communicates with the host microcontroller via a standard I²C compatible interface.

**Technical Specifications:**
- **Operating Voltage:** 1.8V to 3.3V 
- **Communication:** I²C Bus
- **Measurement Metrics:** Heart Rate (BPM) and SpO2 (%)

### <ins>⚡ AD8232 (Single-Lead Heart Rate Monitor)</ins>
The AD8232 is an integrated signal conditioning block for ECG and other biopotential measurement applications. It is designed to extract, amplify, and filter small biopotential signals in the presence of noisy conditions, producing an analog output that is easily read by an ADC.

**Technical Specifications:**
- **Operating Voltage:** 2.0V to 3.5V
- **Communication:** Analog Output (requires an ADC pin)
- **Measurement Metrics:** Electrocardiogram (ECG) waveform

## 📍 Typical Sensor Pinouts (ESP32 Reference)

Below is a quick reference for connecting these sensors to an ESP32 DevKit:

| Sensor | Pin Name | ESP32 Connection (Default) | Function |
| :--- | :--- | :--- | :--- |
| **MAX30102** | VIN | 3.3V | Power Supply |
| | GND | GND | Ground |
| | SDA | GPIO 21 | I²C Data |
| | SCL | GPIO 22 | I²C Clock |
| **AD8232** | 3.3V | 3.3V | Power Supply |
| | GND | GND | Ground |
| | OUTPUT | GPIO 34 / 35 (ADC1) | Analog Signal Output |
| | LO+ | GPIO / Digital Pin | Leads-Off Detection |
| | LO- | GPIO / Digital Pin | Leads-Off Detection |

> **⚠️ Important Sensor Notes:**
> 1. **ADC Selection for AD8232:** Always use ADC1 pins (like GPIO 34 or 35) on the ESP32 if you plan to use Wi-Fi simultaneously, as the ESP32's ADC2 is disabled when Wi-Fi is active.
> 2. **I²C Pull-ups:** Ensure your MAX30102 module has appropriate I²C pull-up resistors, or enable internal pull-ups if communication fails.
> 3. **Voltage Levels:** Always verify your specific sensor breakout board's logic levels. Supplying 5V to a strictly 3.3V-rated sensor will permanently damage it.

## 📂 Included Sensor Firmware

- `MAX_30102/BPM.ino`: 💓 Calculates and outputs Heart Rate (Beats Per Minute).
- `MAX_30102/SpO2.ino`: 🩸 Calculates and outputs Blood Oxygen Saturation.
- `MAX_30102/raw_plot.ino`: 📈 Outputs raw Red/IR optical data formatted for the Serial Plotter.
- `MAX_30102/raw_values.ino`: 🔢 Outputs raw sensor values, useful for debugging.
- `MAX_30102/test_1.ino`: 🛠️ Basic hardware verification sketch.
- `AD_8232/`: ⚡ Contains code and independent documentation for the Analog Front-End ECG sensor.

## ⚙️ Getting Started

To test any of these sensors:
1. Navigate to the specific sensor's sub-directory.
2. Review the dedicated `README.md` inside that folder for specific wiring diagrams and library dependencies.
3. Open the provided `.ino` files in your Arduino IDE (refer to the main QC testing guide for IDE setup instructions).
4. Select your board and port, then upload the sketch.
5. Open the **Serial Monitor** (or **Serial Plotter** for waveforms) to view the incoming data.

## 🛠️ Troubleshooting Guide

 **🚫 I²C Device Not Found (MAX30102)**
  - Check jumper wires and ensure SDA/SCL are not swapped.
  - Run an I²C Scanner sketch to verify the sensor's hardware address.
  - Ensure the board is receiving stable 3.3V power.

 **📉 Noisy/Inaccurate Readings**
  - **MAX30102:** Ensure consistent finger pressure. Pressing too light or too hard will skew optical readings. Shield the sensor from strong ambient light (like direct sunlight).
  - **AD8232:** Ensure electrode pads have good skin contact. Minimize body movement during testing to reduce muscle noise (EMG interference).

## ⚠️ Safety and Handling

  - **NOT FOR MEDICAL DIAGNOSIS:** These sensors and the provided code are for educational, prototyping, and hardware validation purposes only. They are not FDA-approved medical devices.
  - **ELECTRICAL SAFETY:** Ensure you are electrically isolated from mains power when connecting electrodes directly to the body. **Always run your microcontroller off battery power (like a laptop on battery or a USB power bank)** when using the AD8232 ECG sensor.

## 📝 Notes

- These sketches are designed for hardware integration and validation within the AikyaNova Labs ecosystem.

## 📜 Brand and License

All materials are under the AikyaNova™ brand and are licensed for non-commercial use only. See `LICENSE` for details.
