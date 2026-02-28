# AikyaNova Labs: MAX30102 Biomedical Sensor QC Testing

This repository contains the Quality Control (QC) firmware and demonstration codes for testing the MAX30102 Pulse Oximeter and Heart-Rate Sensor using the ESP32 development board.

These scripts are designed for hardware validation, educational purposes, and developing advanced biomedical algorithms including photoplethysmography (PPG) waveform visualization, Heart Rate (BPM) calculation, and Blood Oxygen Saturation (SpO2) estimation.

## 🔬 About the MAX30102 Sensor
The MAX30102 is an integrated pulse oximetry and heart-rate monitor biosensor module. It includes internal LEDs (Red and Infrared), photodetectors, optical elements, and low-noise electronics with ambient light rejection. It operates via an I2C interface, making it easy to connect to microcontrollers like the ESP32.

## ⚙️ Hardware Setup

### Wiring Diagram (ESP32 to MAX30102)
The sensor communicates via the I2C protocol. [cite_start]Ensure the following connections are made[cite: 277, 278]:

| MAX30102 Pin | ESP32 DevKit Pin | Notes |
| :--- | :--- | :--- |
| **VIN** | 3.3V | Power supply |
| **GND** | GND | Ground |
| **SDA** | GPIO 21 | [cite_start]I2C Data [cite: 277, 278] |
| **SCL** | GPIO 22 | [cite_start]I2C Clock [cite: 277, 278] |

## 📚 Software Dependencies

[cite_start]To run these sketches, you will need to install the **SparkFun MAX3010x Pulse and Proximity Sensor Library**[cite: 277].
1. Open the Arduino IDE.
2. Go to **Sketch** -> **Include Library** -> **Manage Libraries...**
3. Search for `MAX30105` (The library supports the MAX30102).
4. Install the library provided by SparkFun.

---

## 📂 Code Overview

This folder contains a progressive series of scripts, from basic hardware validation to complex biometric calculations.

### 1. `test_1.ino` (Hardware Validation)
**Purpose:** A basic sanity check to ensure the ESP32 board is functioning correctly before testing the sensor.
[cite_start]**Description:** This firmware is part of the AikyaNova Labs ESP32 QC Testing Suite[cite: 319]. [cite_start]It simply configures the built-in LED (GPIO 2) as an output and blinks it ON and OFF every 1 second[cite: 322, 323, 324].
**How to use:** Upload and verify the onboard LED blinks.

### 2. `raw_values.ino` (Sensor Communication Test)
**Purpose:** Verifies I2C communication and raw sensor output.
[cite_start]**Description:** Initializes the MAX30102 sensor at 400kHz I2C speed[cite: 279]. [cite_start]It sets up basic PPG settings (LED brightness, sample averaging, and pulse width)[cite: 280, 281, 282]. [cite_start]The code continuously reads the internal FIFO buffer and outputs the raw Infrared (IR) and RED LED values to the Serial Monitor at approximately 50 Hz[cite: 284, 285, 286].
**How to use:** Upload, open the **Serial Monitor** (115200 baud), and place your finger on the sensor. You should see numerical values stream and change with your pulse.

### 3. `raw_plot.ino` (Waveform Visualization)
**Purpose:** Provides a visual representation of the heartbeat (PPG waveform).
[cite_start]**Description:** Reads the IR values and applies a fast baseline tracking algorithm (DC removal shifting by `>>4`) to isolate the AC components (the upright peaks of your pulse)[cite: 345, 347, 348]. 
**How to use:** Upload, open the **Serial Plotter** (115200 baud), and place your finger on the sensor. You will see a waveform that visualizes your heartbeat.

### 4. `BPM.ino` (Heart Rate Extraction)
**Purpose:** Calculates Beats Per Minute (BPM) using the IR sensor data.
**Description:** Uses advanced signal processing. [cite_start]It applies DC removal to track the baseline, extracts the AC signal, and uses an adaptive thresholding envelope to detect local maxima (peaks) representing heartbeats[cite: 325, 328, 332, 336, 337]. [cite_start]It includes a refractory period (300ms) to ignore false double-peaks[cite: 328, 329]. [cite_start]It applies an Exponential Moving Average (EMA) to smooth the final BPM value[cite: 340, 341].
**How to use:** Upload and open the **Serial Plotter**. [cite_start]It outputs three plotted lines: the filtered pulse waveform, the adaptive threshold line, and the scaled BPM (multiplied by 10 for visibility on the plot)[cite: 343, 344].

### 5. `SpO2.ino` (Full Biometric Extraction)
**Purpose:** Calculates both Heart Rate (BPM) and Blood Oxygen Saturation (SpO2).
**Description:** This is the most complex algorithm. [cite_start]It performs DC tracking and extracts AC components for *both* the Red and IR LEDs[cite: 297, 298, 299]. [cite_start]After detecting peaks using the IR envelope [cite: 300, 301, 304][cite_start], it calculates the AC amplitude (peak-to-peak) for both light wavelengths[cite: 302, 303, 308, 309]. [cite_start]It then uses the "ratio-of-ratios" (R-ratio = (AC_red / DC_red) / (AC_ir / DC_ir)) to estimate SpO2 using a standard calibration formula[cite: 308, 310, 311, 312].
**How to use:** Upload and open the **Serial Plotter**. [cite_start]The script outputs 5 columns of comma-separated data: IR waveform, threshold, BPM (x10), SpO2 (x10), and the R-ratio (x1000)[cite: 317, 318]. 
[cite_start]*Note: The SpO2 calculation includes a clamp function limiting output between 70% and 100% to maintain realistic physiological bounds[cite: 292, 293, 313].*

---

## 🛠️ Troubleshooting

* **"MAX30102 not found" Error:** Check your wiring. [cite_start]Ensure SDA is on GPIO 21 and SCL is on GPIO 22. Verify the sensor is receiving 3.3V power[cite: 278, 279].
* **Noisy or Erratic Readings:** The MAX30102 is highly sensitive to motion and ambient light. Ensure your finger is placed gently but firmly on the sensor (do not press too hard, as it restricts blood flow). Keep your hand completely still while measuring.

***
[cite_start]*Developed by AikyaNova Labs for hardware validation and educational purposes.* [cite: 319, 320]