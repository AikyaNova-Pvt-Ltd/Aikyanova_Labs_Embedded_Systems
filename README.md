# 🚀 Welcome to AikyaNova Labs Embedded Systems Repository

[![License: Non-Commercial](https://img.shields.io/badge/License-Non--Commercial-blue.svg)](#-license)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-green.svg)](#)

Welcome to the official repository for **AikyaNova Labs Embedded Systems**. This repository serves as a central hub for our firmware, hardware validation tools, and educational resources. 

---

## 🗂️ Core Modules

### 1. Quality Control (QC) Testing
If you are just getting started or need to perform incoming quality control (IQC) checks on your ESP32 board, please navigate to the **QC Testing** directory. It contains step-by-step setup guides and validation firmware for LED, Wi-Fi, and Bluetooth testing.

👉 **[Click here to go to the QC Testing Directory](./qc_testing)**

### 2. Biomedical Sensors
This directory contains standardized firmware, example sketches, and documentation for integrating physiological sensors into your projects. It currently features reference code for Analog sensors (like the MAX30102 and AD8232) for tracking metrics like heart rate, SpO2, and ECG.

👉 **[Click here to go to the Biomedical Sensors Directory](./Biomedical_Sensors)**

---

## 🛠️ High-Level Prerequisites

Before diving into the specific modules, ensure you have the following ready:
* An ESP32 Development Board (e.g., AikyaNova ESP32 Dev Kit)
* A reliable USB **Data** Cable (power-only cables will not allow firmware uploads)
* A computer running the [Arduino IDE](https://www.arduino.cc/en/software)
* Appropriate USB-to-UART bridge drivers installed (e.g., CP210x, standard for AikyaNova boards)

*Note: Detailed setup instructions for the IDE and drivers can be found inside the `qc_testing` directory.*

---

## 📂 Repository Structure

Below is the current structure of the repository. This layout is designed to keep different modules and projects organized.

```text
AikyaNova_Labs_Embedded_Systems/
├── Biomedical_Sensors/             # Health & Physiological Data Acquisition
│   ├── AD_8232/                    # Single-Lead ECG Sensor Firmware
│   ├── MAX_30102/                  # SpO2 & Heart Rate Sensor Firmware
│   └── README.md                   # Sensor Setup & Wiring Guides
├── qc_testing/                     # QC Firmware & Setup Guides
│   ├── Blink_LED/                  # Basic GPIO Test
│   ├── ESP32_QC_Bluetooth/         # Bluetooth & BLE Tests
│   ├── ESP32_QC_WiFi/              # Wi-Fi AP & Station Tests
│   ├── Images/                     # Documentation Images
│   └── README.md                   # Detailed Setup & Testing Instructions
├── .gitignore                      # Git ignore file
└── README.md                       # This file
```
## 🚀 Getting Started

To get started with the firmware and validation tools, follow these steps:

### 1. Clone the Repository
First, download this repository to your local machine using Git:

```bash
git clone [https://github.com/AikyaNova-Pvt-Ltd/Aikynova_Labs_Embedded_Systems.git](https://github.com/AikyaNova-Pvt-Ltd/Aikynova_Labs_Embedded_Systems.git)
```

Navigate into the cloned directory:

```bash
cd Aikynova_Labs_Embedded_Systems
```
