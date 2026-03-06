# 🔴 AikyaNova Labs Embedded Systems - MAX30102 Sensor QC Testing

This directory contains the demonstration codes for testing the MAX30102 Pulse Oximeter and Heart-Rate Sensor using the ESP32 development board.

These scripts are designed for hardware validation, educational purposes, and developing advanced biomedical algorithms including photoplethysmography (PPG) waveform visualization, Heart Rate (BPM) calculation and Blood Oxygen Saturation (SpO2) estimation.

## ❓ What is the MAX30102 Sensor? 

The MAX30102 is an integrated pulse oximetry and heart-rate monitor biosensor module. It includes internal LEDs (Red and Infrared), photodetectors, optical elements and low-noise electronics with ambient light rejection. It operates via an I2C interface, making it easy to connect to microcontrollers like the ESP32.

### 🧠 Hardware Overview

<p align="center">
  <img src="Images/Max30102_Module.png" width="400" alt="MAX30102 Biosensor Module">
</p>

At the core of this module is the MAX30102 IC (an Analog Devices / Maxim Integrated component). Serving as a robust upgrade to the older MAX30100, this sensor is engineered specifically for precise photoplethysmography (PPG). It is commonly found in commercial smartwatches and fitness trackers due to its compact footprint and high reliability.

The sensor architecture integrates three main components:
* **Dual-Wavelength Emitters:** Two specialized LEDs—one Red (660nm) and one Infrared (880nm)—that project light into the capillary bed of the skin.
* **High-Sensitivity Photodetector:** Measures the amount of light reflected back after being absorbed by pulsating arterial blood.
* **Advanced Signal Processing:** Built-in ambient light rejection and low-noise analog circuitry ensure clean, usable physiological data.

### ⚡ Power and Logic Management
Internally, the bare MAX30102 IC requires two distinct voltage rails: 1.8V for its internal digital logic and 3.3V to drive the LEDs. Fortunately, this breakout module includes onboard voltage regulators, meaning you only need to supply a single power source (typically 3.3V from the ESP32) and the board handles the rest.

The module is configured by default for **3.3V logic levels**, making it natively plug-and-play with microcontrollers like the ESP32 and Arduino. (Note: Some module variants include rear solder jumpers to switch down to 1.8V logic if required by specific low-voltage processors). 

### 🌡️ Integrated Temperature Compensation
Accurate SpO2 calculation is highly dependent on environmental stability, as the wavelength of the LEDs can drift with heat. To account for this, the MAX30102 features an integrated on-chip (die) temperature sensor. 

While it is not designed to measure human body temperature, it allows our algorithms to dynamically compensate for thermal variations in LED emission. This internal sensor is highly precise, maintaining an accuracy of **±1°C** across a harsh operating range of **-40°C to +85°C**.

### 📈 Understanding the Artery Signal Graph

<p align="center">
  <img src="Images/Max30102_Artery_Signal_Graph.png" width="400" alt="Pinout Diagram">
</p>

The waveform image above demonstrates the photoplethysmogram (PPG) output.

As your heart pumps blood (systole), the volume of blood in your finger's capillary bed increases. This surge of blood absorbs more of the emitted infrared light, altering the amount of light that reflects back to the photodetector.

* **The Peaks (Systolic Phase):** Every time your heart contracts, a surge of blood is pumped into your capillary bed. Because blood absorbs the light emitted by the sensor's LEDs, this sudden increase in blood volume results in a spike in light absorption, which appears as a distinct peak on the graph.
* **The Valleys (Diastolic Phase):** As blood flows back out of the capillaries between heartbeats, the volume of blood decreases, lowering the light absorption and creating the troughs in the waveform. 
* **The Dicrotic Notch:** On a clean signal, you can often see a slight, secondary bump on the downward slope of the wave. This corresponds to a brief change in pressure when the heart's aortic valve closes.
* **Signal Processing:** The raw data from the sensor includes a large "DC offset" (the baseline tissue and venous blood absorption). The `raw_plot.ino` script strips away this DC baseline to isolate the "AC component" (the pulsating arterial blood), giving you the clean, oscillating wave you see in the plotter. 

Our more advanced scripts (`BPM.ino` and `SpO2.ino`) take this exact waveform and apply mathematical algorithms—like dynamic thresholding and moving averages—to automatically count those peaks and calculate your heart rate.

## 📍 MAX30102 Pinout (ESP32 Reference)

<p align="center">
  <img src="Images/Max30102_Pinout.png" width="400" alt="Pinout Diagram">
</p>

Ensure the following connections are made for proper I2C communication:

| MAX30102 Pin | ESP32 DevKit Pin | Arduino Pin | Function |
| :--- | :--- | :--- | :--- |
| **VIN** | 3.3V | 3.3V | Power supply |
| **GND** | GND | GND | Ground |
| **SDA** | GPIO 21 | A5 | I2C Data |
| **SCL** | GPIO 22 | A4 | I2C Clock |

### <ins>📊 MAX30102 Technical Specifications</ins>
- **Communication:** I2C Bus (Default speed up to 400kHz)
- **Sensing Elements:** Red (660 nm) and Infrared (880 nm) LEDs
- **Measurement Metrics:** Heart Rate (BPM) and Blood Oxygen Saturation (SpO2)
- **Operating Voltage:** 3.3V to 5.5V (Typically supplied via VIN)
- **Current Draw:** ~600μA (during measurements), 
                    ~0.7μA (during standby mode)
- **Temperature Range:** -40˚C to +85˚C

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



