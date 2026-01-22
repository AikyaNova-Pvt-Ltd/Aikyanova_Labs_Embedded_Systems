# 💡 ESP32 Blink LED QC Test

This directory contains the basic "Hello World" firmware for the ESP32. It acts as the primary functional test to verify that the board can accept new firmware and that the basic digital I/O pins are operational.

---

## 📝 Description

The firmware configures **GPIO 2** (the standard on-board LED pin for most ESP32 DevKits) as a digital output. It toggles the pin **HIGH** and **LOW** in an infinite loop with a 1-second delay, creating a blinking effect.

This test validates:
1.  **USB-to-UART Bridge:** The ability to upload code.
2.  **Processor Core:** The MCU is running instructions.
3.  **GPIO Peripheral:** The pin can successfully drive a load (LED).

---

## ⚙️ Pin Configuration

| Component | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **On-board LED** | **GPIO 2** | Digital Output (Active HIGH) |

---

## 🚀 How to Run

1.  **Open Firmware:**
    * Navigate to the `Blink_LED` folder.
    * Open `Blink_LED.ino` in the Arduino IDE. 📂

2.  **Select Board & Port:**
    * **Board:** `ESP32 Dev Module` (or your specific board).
    * **Port:** Select the correct COM/USB port. 🔌

3.  **Upload:**
    * Click the **Upload (➡️)** button. 📤
    * *Tip:* If the upload gets stuck on `Connecting...`, press and hold the **BOOT** button on the ESP32 board until writing begins.

---

## ✅ Pass Criteria

The board passes the Basic IO Check if:
1.  **Upload Success:** The Arduino IDE shows "Done uploading." without errors.
2.  **Visual Output:** The On-board Blue LED blinks continuously.
3.  **Timing:** The LED stays **ON** for 1 second and **OFF** for 1 second. ⏱️

---

## 🛠️ Troubleshooting

* **⚠️ LED Not Blinking:**
    * Verify the upload was successful.
    * Check if your board uses a different pin for the LED. You can change `#define LED_PIN 2` in the code to `1`, `5`, or `13` to test.
* **🚫 "A fatal error occurred: Failed to connect":**
    * Hold the **BOOT** button instantly when the "Connecting..." text appears in the console.
    * Release it once you see "Writing at...".
