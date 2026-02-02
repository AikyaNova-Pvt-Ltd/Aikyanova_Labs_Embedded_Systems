# Welcome to AikyaNova Labs Embedded Systems Repository

Welcome to the official repository for **AikyaNova Labs Embedded Systems**. This repository serves as a central hub for our firmware, hardware validation tools, and educational resources.

---

## 🚀 Getting Started with ESP32

If you are just getting started or need to perform Quality Control (QC) checks on your ESP32 board, please navigate to the **QC Testing** directory.

The **[qc_testing](./qc_testing)** folder contains everything you need to begin, including:
* **Step-by-step Guides:** How to install the Arduino IDE and CP210x Drivers.
* **Validation Firmware:** Code for testing LED Blink, Wi-Fi, and Bluetooth.
* **Troubleshooting:** Solutions for common connection issues.

👉 **[Click here to go to the QC Testing Directory](./qc_testing)**

---

## 📂 Repository Structure

Below is the current structure of the repository. This layout is designed to keep different modules and projects organized.

```text
AikyaNova_Labs_Embedded_Systems/
├── qc_testing/                     # QC Firmware & Setup Guides
│   ├── Blink_LED/                  # Basic "Hello World" GPIO Test
│   ├── ESP32_QC_Bluetooth/         # Bluetooth Classic (Android) & BLE (iOS) Tests
│   ├── ESP32_QC_WiFi/              # Wi-Fi AP & Station Mode Tests
│   ├── Images/                     # Documentation Images
│   └── README.md                   # Detailed Setup & Testing Instructions
├── .gitignore                      # Git ignore file
└── README.md                       # This file
```
## Clone the Repository

```bash
git clone https://github.com/AikyaNova-Pvt-Ltd/Aikynova_Labs_Embedded_Systems.git
```

```bash
cd Aikynova_Labs_Embedded_Systems
```
