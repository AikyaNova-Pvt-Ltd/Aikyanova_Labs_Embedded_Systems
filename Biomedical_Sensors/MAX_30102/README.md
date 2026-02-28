# 🔴 AikyaNova Labs Embedded Systems - MAX30102 Sensor QC Testing

This directory contains the Quality Control (QC) firmware and demonstration codes for testing the MAX30102 Pulse Oximeter and Heart-Rate Sensor using the ESP32 development board.

These scripts are designed for hardware validation, educational purposes, and developing advanced biomedical algorithms including photoplethysmography (PPG) waveform visualization, Heart Rate (BPM) calculation, and Blood Oxygen Saturation (SpO2) estimation.

## ❓ What is the MAX30102 Sensor? 

### 🧠 Overview
The MAX30102 is an integrated pulse oximetry and heart-rate monitor biosensor module. It includes internal LEDs (Red and Infrared), photodetectors, optical elements, and low-noise electronics with ambient light rejection. It operates via an I2C interface, making it easy to connect to microcontrollers like the ESP32.

### <ins>📊 MAX30102 Technical Specifications</ins>
- **Communication:** I2C Bus (Default speed up to 400kHz)
- **Sensing Elements:** Red and Infrared (IR) LEDs
- **Measurement Metrics:** Heart Rate (BPM) and Blood Oxygen Saturation (SpO2)
- **Operating Voltage:** 3.3V (Typically supplied via VIN)

## 📍 MAX30102 Pinout (ESP32 Reference)

Ensure the following connections are made for proper I2C communication:

| MAX30102 Pin | ESP32 DevKit Pin | Function / Notes |
| :--- | :--- | :--- |
| **VIN** | 3.3V | Power supply |
| **GND** | GND | Ground |
| **SDA** | GPIO 21 | I2C Data |
| **SCL** | GPIO 22 | I2C Clock |

## ⚙️ Software Dependencies

To run these sketches, you will need to install the **SparkFun MAX3010x Pulse and Proximity Sensor Library**.

1. Open the Arduino IDE.
2. Go to **Sketch** > **Include Library** > **Manage Libraries...**
3. Search for `MAX30105` (The library supports the MAX30102).
4. Install the library provided by SparkFun.

## 📂 Included Test Firmware

This folder contains a progressive series of scripts, from basic hardware validation to complex biometric calculations.

- `test_1.ino`: 💡 **Hardware Validation:** A basic sanity check to ensure the ESP32 board is functioning correctly. It configures the built-in LED (GPIO 2) as an output and blinks it ON and OFF every 1 second.
- `raw_values.ino`: 🔢 **Sensor Communication Test:** Initializes the MAX30102 at 400kHz. Sets up basic PPG settings (LED brightness, sample averaging, pulse width) and outputs the raw Infrared (IR) and RED LED values to the Serial Monitor at ~50 Hz.
- `raw_plot.ino`: 📈 **Waveform Visualization:** Reads IR values and applies a fast baseline tracking algorithm (DC removal shifting by `>>4`) to isolate the AC components, visualizing your heartbeat on the Serial Plotter.
- `BPM.ino`: 💓 **Heart Rate Extraction:** Applies DC removal, extracts the AC signal, and uses adaptive thresholding to detect local maxima (peaks) representing heartbeats. Outputs filtered waveform, threshold, and scaled BPM (x10) to the Serial Plotter. Includes a 300ms refractory period and Exponential Moving Average smoothing.
- `SpO2.ino`: 🩸 **Full Biometric Extraction:** Calculates both BPM and SpO2 using the "ratio-of-ratios" (R-ratio) formula for Red and IR wavelengths. Outputs IR waveform, threshold, BPM (x10), SpO2 (x10, clamped between 70%-100%), and the R-ratio (x1000) to the Serial Plotter.

## 🛠️ Troubleshooting Guide

 **🚫 "MAX30102 not found" Error**
  - Check your wiring. Ensure SDA is on GPIO 21 and SCL is on GPIO 22. 
  - Verify the sensor is receiving stable 3.3V power.

 **📉 Noisy or Erratic Readings**
  - **Motion Sensitivity:** The MAX30102 is highly sensitive to motion and ambient light. 
  - **Finger Placement:** Ensure your finger is placed gently but firmly on the sensor (pressing too hard restricts blood flow). 
  - Keep your hand completely still while measuring.

## ⚠️ Safety and Handling

  - **NOT FOR MEDICAL DIAGNOSIS:** These scripts and the MAX30102 module are developed for hardware validation and educational purposes only. They are not FDA-approved medical devices.

## 📝 Notes

- These tests are designed for hardware validation and algorithm development within the AikyaNova Labs ecosystem.

## 📜 Brand and License

All materials are under the AikyaNova™ brand and are licensed for non-commercial use only. See `LICENSE` for details.
