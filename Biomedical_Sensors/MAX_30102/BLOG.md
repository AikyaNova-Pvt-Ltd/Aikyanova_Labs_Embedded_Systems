# Build Your Own Pulse Oximeter — A Complete Guide to PPG, BPM & SpO2 with the MAX30102

**By AikyaNova Labs** | Biomedical Sensors Series

![AikyaNova Pulse Oximeter Demo Board](Images/board_hero.jpg)
*The AikyaNova Pulse Oximeter Demo Board — MAX30102 sensor + 0.96" OLED display, works with any I2C microcontroller.*

---

## What You Will Learn

By the end of this guide you will understand:

- What a pulse oximeter actually measures and how it works physically
- How the MAX30102 sensor converts light into a digital signal
- How to extract a heartbeat from raw sensor data in firmware
- How to estimate blood oxygen saturation (SpO2) from two wavelengths of light
- How to build and run four progressive firmware sketches on the **AikyaNova Pulse Oximeter Demo Board**

No biomedical background is required. Every concept is introduced from first principles.

---

## Part 1 — The Science Behind Pulse Oximetry

### 1.1 What Is a Pulse Oximeter?

A pulse oximeter is a non-invasive device that measures two vital signs simultaneously:

| Measurement | What It Tells You |
|---|---|
| **SpO2** (blood oxygen saturation) | The percentage of haemoglobin in your blood that is carrying oxygen. A healthy adult is typically 95–100%. Below 90% is clinically significant. |
| **Heart Rate (BPM)** | The number of complete heartbeats per minute. A resting adult is typically 60–100 BPM. |

You have almost certainly seen one — it is the small clip placed on a fingertip in hospitals and clinics. What you may not know is that the physics behind it is elegant and entirely learnable.

---

### 1.2 Why Light? — The Photoplethysmography (PPG) Principle

Your blood changes colour depending on how much oxygen it is carrying.

- **Oxygenated blood (HbO₂)** absorbs more **red** light and lets infrared (IR) light pass through.
- **Deoxygenated blood (Hb)** absorbs more **infrared** light and lets red light pass through.

A pulse oximeter exploits this difference. It shines two LEDs — one **red (~660 nm)** and one **infrared (~880 nm)** — through your fingertip and measures how much of each wavelength reaches a photodetector on the other side. The ratio of the two absorption values is what tells us the oxygen level.

This technique of measuring blood volume changes optically is called **Photoplethysmography (PPG)**.

![PPG principle — red and IR LEDs shining through a fingertip](Images/ppg_principle.jpg)
*The MAX30102 contains both LEDs and the photodetector in a single 5.6mm package.*

---

### 1.3 The PPG Waveform — DC and AC Components

The raw sensor signal has two distinct components:

```
Raw IR Signal
│
│  ████████████████████████████████  ← DC Component (~100,000 counts)
│                                      Constant absorption by tissue,
│                                      bone, and venous blood.
│
│  ∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿  ← AC Component (~500–2000 counts)
│                                      Pulsatile absorption by arterial
│                                      blood. One cycle = one heartbeat.
```

The **DC component** is large and useless for measurement — it just represents the tissue in the way. The **AC component** is tiny but contains all the information: heart rate and oxygen saturation.

Extracting the AC component requires **DC removal**, which is a key step in all four of our firmware sketches.

![Serial Plotter showing the AC PPG waveform after DC removal](Images/serial_plotter_ppg.png)
*Sketch 2 output in Arduino Serial Plotter — each upward peak is one heartbeat.*

---

### 1.4 The Ratio-of-Ratios — How SpO2 Is Calculated

Once we have the AC amplitude for both the red and IR channels, SpO2 is calculated as follows:

```
         AC_RED / DC_RED
R  =  ─────────────────────    (the Ratio of Ratios)
         AC_IR  / DC_IR

SpO2  ≈  110  −  25 × R        (empirical calibration formula)
```

When blood is fully oxygenated (SpO2 ≈ 99%), the red signal is barely absorbed — so `AC_RED` is small and R is low. As oxygen falls, red absorption rises, R increases, and the formula gives a lower SpO2 value. The constants `110` and `25` are derived from empirical calibration against certified reference oximeters.

---

### 1.5 The Dicrotic Notch — Why Peak Detection Is Hard

A real PPG waveform is not a simple sine wave. Each heartbeat produces a **two-humped shape**:

```
    Systolic Peak          Second Peak
         ▲                     ▲
        ╱ ╲                   ╱ ╲
       ╱   ╲     Dicrotic    ╱   ╲
      ╱     ╲    Notch ↓    ╱     ╲
─────╱       ╲────╱──╲────╱       ╲─────
                       ↑
              This dip is caused by the
              aortic valve closing after
              the heart contracts.
```

The **dicrotic notch** — the small dip between the two peaks — is caused by the aortic valve snapping shut after the heart's contraction. A naïve peak-detection algorithm will count both peaks as separate heartbeats, reporting double the actual BPM. Our firmware handles this with a **refractory period** (minimum time between accepted beats) and a **plausibility filter**.

![Serial Plotter showing Signal, Threshold and BeatMarker traces](Images/serial_plotter_bpm.png)
*Sketch 3 Serial Plotter output — the green BeatMarker spike fires once per real heartbeat, not twice.*

---

## Part 2 — The AikyaNova Pulse Oximeter Demo Board

![AikyaNova board top view](Images/board_top.jpg)

The **AikyaNova Pulse Oximeter Demo Board** is a compact sensor module carrying a **MAX30102 optical sensor** and a **0.96" SSD1306 OLED display**, designed specifically for learning and prototyping pulse oximetry systems.

> **The board does not include a microcontroller.** It is designed to plug into the microcontroller you already have — ESP32, Arduino Uno, Arduino Nano, Arduino Mega, ESP8266, Raspberry Pi Pico, or any other board with an I2C bus. This keeps the cost low and lets you use the development environment you are already familiar with.

### What Is on the Board

| Component | Part | Purpose |
|---|---|---|
| Optical Sensor | MAX30102 | Red (~660 nm) + IR (~880 nm) LEDs with integrated photodetector |
| Display | 0.96" SSD1306 OLED | 128×64 pixels, I2C — shows BPM, SpO2, prompts |
| Connector | 4-pin header | VCC, GND, SDA, SCL — connects to any microcontroller |

### Wiring — Works with Any Microcontroller

The board uses the standard I2C bus. Connect four wires:

| Board Pin | ESP32 | Arduino Uno/Nano | Arduino Mega | ESP8266 |
|---|---|---|---|---|
| VCC | 3.3V | 3.3V or 5V | 3.3V or 5V | 3.3V |
| GND | GND | GND | GND | GND |
| SDA | GPIO 21 | A4 | 20 | D2 (GPIO 4) |
| SCL | GPIO 22 | A5 | 21 | D1 (GPIO 5) |

![Wiring diagram — board connected to ESP32](Images/wiring_esp32.jpg)
*4-wire connection to an ESP32. The firmware auto-detects the board type and sets the correct I2C pins.*

![Wiring diagram — board connected to Arduino Uno](Images/wiring_arduino.jpg)
*Same board, same 4 wires — connected to an Arduino Uno.*

The firmware includes a cross-platform I2C initialisation block that automatically selects the correct pins for each supported board:

```cpp
#if defined(ESP32)
  Wire.begin(21, 22); // SDA = GPIO 21, SCL = GPIO 22
#elif defined(ESP8266)
  Wire.begin(4, 5);   // SDA = D2, SCL = D1
#else
  Wire.begin();       // Arduino Uno / Nano / Mega: A4 / A5
#endif
```

No configuration changes needed — just select your board in `Tools > Board` and upload.

---

### What Makes It Different

**Skin Tone Compensation.**
Most consumer pulse oximeters are calibrated only for fair skin. Melanin in darker skin absorbs significantly more red light, causing weak signals and inaccurate readings. Our firmware features an **independent auto-LED brightness feedback loop** for both the red and IR LEDs — it adjusts every 50ms to keep the signal in the optimal range regardless of skin tone.

![OLED showing BPM reading](Images/oled_bpm.jpg)
*Live BPM display on the OLED with the pulsing heart indicator.*

![OLED showing SpO2 and BPM](Images/oled_spo2.jpg)
*Sketch 4 — BPM and SpO2 displayed simultaneously.*

**Educational Firmware Series.**
Instead of a single opaque sketch, we provide four progressive sketches that build on each other — starting from raw ADC values and ending with a full dual-parameter oximeter. Every phase of every algorithm is commented to explain not just *what* the code does but *why*.

**Open Firmware.**
Released under the AikyaNova Non-Commercial License — free for students, educators, and hobbyists. The full source is in this repository.

---

## Part 3 — The Firmware Series (4 Sketches)

All four sketches share a common structure:
- Cross-platform I2C initialisation (ESP32, ESP8266, Arduino AVR)
- Scrolling welcome screen on startup
- Immediate finger detection — the display stops animating the instant your finger touches the sensor
- Identical section layout and comment style so you can compare sketches side by side

---

### Sketch 1 of 4 — Raw Values (`Max3010x_Raw_Values`)

**Goal:** Verify your hardware is working. Understand what the sensor actually produces before any processing.

![OLED showing raw IR and RED values](Images/oled_raw_values.jpg)
*Sketch 1 OLED output — raw IR and RED ADC counts update in real time.*

**What it does:**
- Reads raw IR and Red ADC counts from the MAX30102 FIFO
- Displays both values live on the OLED
- Prints them to the Serial Monitor

**What to observe:**
- With no finger: IR and RED values are very low (< 10,000)
- Place your finger: values jump to 50,000–250,000 depending on skin tone and LED power
- Watch carefully: you can see a tiny pulsatile variation with each heartbeat even in the raw numbers

```cpp
// This is the core of the sketch — just two reads per loop
long irValue  = particleSensor.getIR();
long redValue = particleSensor.getRed();
```

**Key concept introduced:** The difference between the DC baseline (tissue absorption) and the AC pulse signal is visible even in raw numbers. The next sketch makes this explicit.

---

### Sketch 2 of 4 — Raw Plot (`Max3010x_Raw_Plot`)

**Goal:** See the PPG waveform. Understand DC removal and what the AC signal looks like.

**What it does:**
- Applies an Exponential Moving Average (EMA) filter to track the DC baseline
- Subtracts the DC from the raw signal to isolate the AC pulse wave
- Sends the AC signal to the Arduino Serial Plotter at a controlled rate

**The DC removal filter:**

```cpp
// EMA low-pass filter — tracks the slow baseline
// >> 4 is equivalent to dividing by 16 (fast integer math)
dcIR = dcIR + ((ir - dcIR) >> 4);

// Subtract baseline to isolate the pulsatile waveform
// Inverted so peaks point upward (IR drops when blood rushes in)
int32_t ac = dcIR - ir;

Serial.println(ac / 8); // /8 scales it to fit the plotter window
```

![Serial Plotter showing clean PPG waveform](Images/serial_plotter_ppg_clean.png)
*Open Tools > Serial Plotter at 115200 baud. Each peak is one heartbeat.*

**What to observe in Serial Plotter:**
- Each upward peak is one heartbeat
- The waveform shows the characteristic double-hump shape (systolic peak + dicrotic notch)
- Fair skin = taller peaks; darker skin = shorter peaks (auto-brightness in later sketches compensates)

**Key concept introduced:** DC removal, the EMA filter, and the inverted AC signal.

---

### Sketch 3 of 4 — BPM (`Max3010x_BPM`)

**Goal:** Detect heartbeats reliably and display BPM. Understand every decision in a production-quality peak detection algorithm.

![OLED BPM display with heart symbol](Images/oled_bpm_heart.jpg)
*The heart symbol pulses on every confirmed beat. "LOW SIGNAL" appears if contact is poor.*

**The algorithm has 7 phases:**

#### Phase 1 — Finger Detection & Warmup
A 500ms warmup window is applied after finger placement. During this time, the auto-brightness is allowed to settle before the finger-absent check is enforced. This prevents false removals on dark skin tones where the signal starts weak.

#### Phase 2 — Auto-LED Brightness
```cpp
// Too weak (dark skin / thick finger) → increase LED current
if (ir < 60000 && currentPower < 250)  currentPower += 2;

// Heavily saturated (very fair skin) → decrease aggressively
else if (ir > 220000 && currentPower > 5)
  currentPower = (currentPower > 10) ? currentPower - 5 : 5;

// Mildly strong → gentle decrease
else if (ir > 180000 && currentPower > 5)  currentPower -= 2;
```

#### Phase 3 — DC Removal
Same EMA filter as Sketch 2, extended to also track the signal envelope.

#### Phase 4 — Dynamic Thresholding
Instead of a fixed threshold, the algorithm tracks the live amplitude of the signal using an envelope follower. The beat trigger threshold is always 50% of the current envelope — so it adapts automatically to both strong and weak pulses.

```cpp
env    = 0.95f * env + 0.05f * |x|;  // envelope tracks average peak amplitude
thresh = 0.5f * env;                  // trigger at 50% of envelope
```

#### Phase 5 — Peak Detection with Refractory Period
```cpp
// Three-point local maximum, above threshold, above quality gate
bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh) && (env >= MIN_SIGNAL_QUALITY);

// Refractory period blocks the dicrotic notch (arrives 300-350ms after main peak)
// 400ms refractory caps max BPM at 150 — sufficient for a consumer device
if (dt > REFRACTORY_MS) { ... }
```

#### Phase 6 — BPM Smoothing with Plausibility Filter
```cpp
// First beat is discarded — the first interval is measured from an arbitrary
// timestamp, not a real beat, so it would seed a wrong BPM.
if (beatCount == 1) { /* skip */ }

// Subsequent beats: smooth with 30% new weight (~3 beats to settle)
bpm = 0.70f * bpm + 0.30f * instBpm;

// Plausibility check: >40% sudden jump → near-zero weight (likely noise)
if (instBpm > bpm * 1.4f)  bpm = 0.98f * bpm + 0.02f * instBpm;
```

#### Phase 7 — OLED + Serial Plotter Output
![Serial Plotter showing Signal, Threshold, BeatMarker](Images/serial_plotter_bpm_annotated.png)
*Three traces: PPG signal (blue), dynamic threshold (orange), beat marker spike (green).*

---

### Sketch 4 of 4 — SpO2 (`Max3010x_SpO2`)

**Goal:** Add blood oxygen estimation on top of BPM. Understand the Ratio-of-Ratios method and its limitations.

![OLED showing BPM 72 and SpO2 98%](Images/oled_spo2_reading.jpg)
*Sketch 4 live output — BPM on the left, SpO2% on the right.*

**What is added over Sketch 3:**
- Independent auto-brightness for both the Red and IR LEDs
- Peak-to-peak amplitude tracking for both channels per beat cycle
- SpO2 calculation using the Ratio-of-Ratios formula
- Dual display: BPM on the left, SpO2% on the right
- SpO2 timeout (10 seconds) — display clears if no valid beat arrives

**The SpO2 calculation per heartbeat:**

```cpp
// Normalize AC amplitude against DC baseline for each channel
float nir  = acIRAmp  / dcIR;   // AC/DC for infrared
float nred = acREDAmp / dcRED;  // AC/DC for red

// Ratio of Ratios
Rratio = nred / nir;

// Empirical calibration formula (standard industry approximation)
float spo2Inst = 110.0f - 25.0f * Rratio;
spo2Inst = clamp(spo2Inst, 70.0f, 100.0f); // physically valid range

// Smooth with EMA — SpO2 changes slowly
spo2 = 0.85f * spo2 + 0.15f * spo2Inst;
```

> **Important note for learners:** The formula `SpO2 = 110 − 25×R` is an empirical approximation. Medical-grade devices use a lookup table derived from calibration studies with human subjects and certified blood-gas analysers. Our firmware provides an educational approximation suitable for learning and demonstration — **it is not a medical device**.

---

## Part 4 — Learning Path & Experiments

Once you have all four sketches running, try these experiments to deepen your understanding:

### Experiment 1 — Observe DC Removal
Run Sketch 2 (Raw Plot) in Serial Plotter. Press your finger harder against the sensor — the DC baseline will shift but the AC waveform will remain centred at zero. This is the EMA filter working.

### Experiment 2 — Observe the Dicrotic Notch
Run Sketch 2 and look carefully at the waveform. After the main systolic peak there is a smaller secondary bump approximately 300–400ms later — that is the dicrotic notch. Change `REFRACTORY_MS` to `200` in Sketch 3 and watch the BPM reading double. Change it back to `400` and it corrects.

### Experiment 3 — Observe Skin Tone Compensation
In Sketch 3, add `Serial.print(currentPower)` to the Serial output. Compare readings between people with different skin tones. You will see the LED current adjust significantly — higher for darker skin, lower for fair skin.

### Experiment 4 — Understand the Smoothing Filter
In Sketch 3, change `0.70f` to `0.50f` in the BPM smoothing line. The BPM will respond faster to changes but will be noisier. Change it to `0.90f` and it will be very stable but slow to respond. This trade-off between responsiveness and noise rejection is fundamental to all signal processing.

### Experiment 5 — Breathe and Watch SpO2
Run Sketch 4. Breathe normally — SpO2 should read 96–100%. Take several rapid deep breaths, then hold your breath for 20 seconds and watch the SpO2 value begin to fall. Do not hold your breath long enough to feel discomfort. This demonstrates that the sensor is genuinely responding to real blood oxygen changes.

---

## Part 5 — Licensing & Usage

This firmware is released under the **AikyaNova Non-Commercial License v1.0**.

✅ Use, modify, and redistribute for personal, educational, and academic purposes.
✅ Attribution to AikyaNova Labs must be retained in all copies.
❌ Commercial use requires a separate commercial license.

Third-party library licenses (SparkFun BSD, Adafruit BSD, Maxim MIT-style) are reproduced in full in [LICENSE.md](LICENSE.md) in compliance with their redistribution requirements.

**For commercial licensing:**
📧 aikyanova.labs@gmail.com
🌐 www.aikyanova.com

---

## Part 6 — Get the Demo Board

![AikyaNova board in hand with finger on sensor](Images/board_in_use.jpg)
*The board in use — finger resting on the MAX30102 sensor, OLED showing live BPM.*

The **AikyaNova Pulse Oximeter Demo Board** is available for educators, students, and makers.

### What Is Included
- AikyaNova MAX30102 + SSD1306 OLED sensor board
- 4-pin connector cable
- All 4 firmware sketches (open source)
- Quick-start wiring card

### Compatible Microcontrollers
Works with any board that has an I2C bus and 3.3V or 5V GPIO:

| Board | Supported |
|---|---|
| ESP32 (all variants) | ✅ |
| Arduino Uno / Nano / Mega | ✅ |
| ESP8266 (NodeMCU, Wemos D1) | ✅ |
| Raspberry Pi Pico | ✅ (with I2C configured) |
| Any I2C-capable board | ✅ |

### Who Is It For
- Engineering students learning embedded systems or biomedical instrumentation
- Educators building hands-on lab exercises around vital sign monitoring
- Makers and hobbyists exploring health-sensing projects
- Researchers prototyping non-invasive optical sensing algorithms

### Contact & Orders
📧 **aikyanova.labs@gmail.com**
🌐 **www.aikyanova.com**

---

## Quick Reference — Sensor Configuration

| Parameter | Sketch 1 | Sketch 2 | Sketch 3 | Sketch 4 |
|---|---|---|---|---|
| LED Mode | Red + IR | Red + IR | Red + IR | Red + IR |
| Sample Rate | 100 Hz | 800 Hz | 400 Hz | 100 Hz |
| Sample Average | 4 | 8 | 8 | 8 |
| **Effective Output** | **25 Hz** | **100 Hz** | **50 Hz** | **12.5 Hz** |
| Pulse Width | 411 µs (18-bit) | 411 µs (18-bit) | 411 µs (18-bit) | 411 µs (18-bit) |
| ADC Range | 4096 | 4096 | 4096 | 4096 |
| Auto-LED | No | Fixed | IR only | IR + Red (independent) |

---

## Image Reference

When adding images, place them in the `Images/` folder and name them as follows:

| Filename | Content |
|---|---|
| `board_hero.jpg` | Hero shot of the board — angled, well-lit |
| `board_top.jpg` | Top-down flat lay of the board |
| `board_in_use.jpg` | Board connected to a microcontroller, finger on sensor |
| `wiring_esp32.jpg` | Wiring photo or diagram — board to ESP32 |
| `wiring_arduino.jpg` | Wiring photo or diagram — board to Arduino Uno |
| `oled_raw_values.jpg` | OLED showing IR and RED raw numbers (Sketch 1) |
| `oled_bpm.jpg` | OLED showing BPM value (Sketch 3) |
| `oled_bpm_heart.jpg` | OLED showing BPM + pulsing heart symbol |
| `oled_spo2.jpg` | OLED showing BPM + SpO2 side by side (Sketch 4) |
| `oled_spo2_reading.jpg` | Close-up of OLED with a real reading (e.g. 72 BPM, 98%) |
| `serial_plotter_ppg.png` | Serial Plotter screenshot — raw PPG waveform (Sketch 2) |
| `serial_plotter_ppg_clean.png` | Serial Plotter screenshot — zoomed to show ~5 clean peaks |
| `serial_plotter_bpm.png` | Serial Plotter screenshot — Signal + Threshold + BeatMarker (Sketch 3) |
| `serial_plotter_bpm_annotated.png` | Same as above with annotations labelling each trace |
| `ppg_principle.jpg` | Diagram or photo showing light through fingertip concept |

---

## Glossary

| Term | Definition |
|---|---|
| **PPG** | Photoplethysmography — optical measurement of blood volume changes |
| **SpO2** | Peripheral blood oxygen saturation, measured non-invasively |
| **BPM** | Beats Per Minute — heart rate |
| **DC Component** | The constant, non-pulsatile portion of the optical signal |
| **AC Component** | The pulsatile portion caused by arterial blood with each heartbeat |
| **EMA** | Exponential Moving Average — a one-pole IIR low-pass filter |
| **Dicrotic Notch** | Secondary pressure wave in the PPG from aortic valve closure |
| **Refractory Period** | Minimum time between accepted beats — blocks double-counting |
| **Ratio of Ratios** | R = (AC_RED/DC_RED) / (AC_IR/DC_IR) — the core SpO2 quantity |
| **Envelope** | Running estimate of the AC signal's average peak amplitude |
| **FIFO** | First-In First-Out buffer inside the MAX30102 that stores sensor samples |
| **I2C** | Inter-Integrated Circuit — two-wire serial protocol (SDA + SCL) used by both the sensor and OLED |

---

*Copyright © 2025 AikyaNova™ | AikyaNova Labs. All rights reserved.*
*This firmware is not a medical device. Do not use for clinical diagnosis or patient monitoring.*
