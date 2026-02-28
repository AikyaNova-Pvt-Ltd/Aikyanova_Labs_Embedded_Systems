# Biomedical Sensors

This directory contains the firmware, example sketches, and documentation for integrating various biomedical sensors into AikyaNova Labs embedded systems projects. These modules are primarily focused on health and fitness monitoring applications, such as heart rate, blood oxygen saturation (SpO2), and electrocardiogram (ECG) data acquisition.

## Available Sensors

### 1. MAX30102 (Pulse Oximeter and Heart-Rate Sensor)
The `MAX_30102` folder contains code for interfacing with the MAX30102 sensor. This I2C-based module uses Red and IR LEDs for non-invasive tracking of blood oxygen levels and heart rate. 

**Included Examples:**
* **`BPM.ino`**: Calculates and outputs Heart Rate (Beats Per Minute).
* **`SpO2.ino`**: Calculates and outputs Blood Oxygen Saturation percentages.
* **`raw_values.ino`** & **`raw_plot.ino`**: Outputs raw Red and IR optical data, which is highly useful for debugging and visualizing the pulse waveform on the Serial Plotter.
* **`test_1.ino`**: Basic hardware verification sketch.

*(See the `MAX_30102/README.md` for specific library dependencies and I2C wiring instructions.)*

### 2. AD8232 (Single-Lead Heart Rate Monitor)
The `AD_8232` folder focuses on the AD8232 Analog Front-End. This sensor measures the electrical activity of the heart to produce an ECG (Electrocardiogram) analog output, which can be read by the microcontroller's ADC pins.

*(See the `AD_8232/README.md` for wiring diagrams, lead placement, and usage instructions.)*

## Getting Started

To test any of these sensors:
1. Navigate into the specific sensor's sub-directory.
2. Review the dedicated `README.md` inside that folder for specific wiring diagrams.
3. Open the provided `.ino` files in your IDE, select your development board, and flash the code.