 /*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: Plots the AC (pulsatile) component of the raw PPG IR signal
 *           from the MAX3010x sensor to the Arduino Serial Plotter in
 *           real time. DC baseline removal is applied so the waveform is
 *           centred at zero and each heartbeat appears as an upward peak.
 *           This is Sketch 2 of 4 in the AikyaNova Pulse Oximeter series.
 *           Use this sketch to visually verify signal quality and waveform
 *           shape before moving on to the BPM and SpO2 sketches.
 *           Open Tools > Serial Plotter @ 115200 baud to view the waveform.
 * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * * Dependencies & Credits:
 * - SparkFun MAX3010x Sensor Library (BSD License)
 * - Adafruit GFX and SSD1306 Libraries by Adafruit Industries (BSD License)
 * ------------------------------------------------------------------------
 */

#include <Wire.h>             // I2C communication library
#include "MAX30105.h"         // SparkFun MAX3010x optical sensor library
#include <Adafruit_GFX.h>     // Core graphics library for OLED display
#include <Adafruit_SSD1306.h> // SSD1306 OLED display driver

// ========================================================================
// OLED DISPLAY SETTINGS
// ========================================================================
#define SCREEN_WIDTH   128 // OLED display width, in pixels
#define SCREEN_HEIGHT  64 // OLED display height, in pixels
#define OLED_RESET     -1    // -1 = OLED RST pin not wired to a GPIO; reset handled by hardware power-on
#define SCREEN_ADDRESS 0x3C  // Default I2C address for most 0.96" SSD1306 OLEDs; change to 0x3D if display not found

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Create an instance of the SSD1306 display class

// ========================================================================
// SENSOR & SIGNAL VARIABLES
// ========================================================================
MAX30105 sensor;  // Create an instance of the MAX30105 sensor class

// Variable to store the DC (baseline) component of the IR signal
// Using int32_t to accommodate the large 18-bit values from the sensor
int32_t dcIR = 0;

// ========================================================================
// SCROLL TEXT HELPER
// ========================================================================
// Scrolls a single line of text right-to-left across the vertical centre of
// the display. Uses textSize 2 (12px wide × 16px tall per character).
// setTextWrap(false) prevents the text from breaking onto a second line.
//
// Parameters:
//   text        — the string to scroll
//   numPasses   — how many times the full string crosses the screen
//   drainSensor — set true ONLY after sensor.begin() has been called.
//                 When true, the IR value is sampled every animation frame.
//                 If a finger is detected (IR >= 15000) the scroll aborts
//                 immediately and returns true, so the caller can react at once.
//
// Returns: true if aborted by finger detection, false if completed normally.
bool scrollText(const char* text, int numPasses, bool drainSensor = false) {
  int charPxWidth = 6 * 2;                     // 12px per character at textSize 2
  int textPxWidth = strlen(text) * charPxWidth;
  int yPos        = (SCREEN_HEIGHT - 16) / 2;  // vertically centred (16px = textSize 2 height)

  for (int pass = 0; pass < numPasses; pass++) {
    // x starts at the right edge and moves left until the text is fully off-screen
    for (int x = SCREEN_WIDTH; x > -textPxWidth; x -= 3) {
      display.clearDisplay();
      display.setTextWrap(false);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(x, yPos);
      display.print(text);
      display.display();
      if (drainSensor && sensor.getIR() >= 15000) return true; // finger detected — abort immediately
      delay(8);
    }
  }
  return false;
}

// ========================================================================
// SETUP
// ========================================================================
void setup() {
  Serial.begin(115200);

  // --- CROSS-PLATFORM I2C INITIALIZATION ---
  // Detects the board type and starts the I2C bus accordingly
#if defined(ESP32)
  Wire.begin(21, 22); // ESP32 standard I2C pins (SDA=21, SCL=22)
#elif defined(ESP8266)
  Wire.begin(4, 5);   // ESP8266 standard I2C pins (SDA=D2/4, SCL=D1/5)
#else
  Wire.begin();       // Standard Arduino (Uno, Nano, Mega) uses default hardware pins
#endif

  // --- OLED DISPLAY INITIALIZATION ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring."));
    while (1) delay(10);
  }

  // --- WELCOME SCREEN ---
  // Scrolls the product name once across the display before sensor init.
  // drainSensor=false because sensor.begin() has not been called yet.
  scrollText("Welcome to AikyaNova Pulse Oximeter Demo Board", 1);

  // --- SENSOR INITIALIZATION ---
  // Attempt to communicate with the sensor at 400kHz (Fast I2C speed).
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("Sensor not found. Check wiring!"));
    display.clearDisplay();
    display.setTextWrap(true);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println(F("Sensor Error!"));
    display.display();
    while (1) delay(10);
  }

  // --- SENSOR CONFIGURATION ---
  //
  // Argument 1: powerLevel (LED Pulse Amplitude Configuration)
  //   Possible values: 0x00 (0mA) to 0xFF (50mA).
  //   Examples: 0x02 (~0.4mA), 0x1F (~6.4mA), 0x7F (~25.4mA), 0xFF (~50mA).
  //
  // Argument 2: sampleAverage (Number of samples averaged per FIFO reading)
  //   Possible values: 1, 2, 4, 8, 16, 32
  //
  // Argument 3: ledMode (Which LEDs to use)
  //   Possible values:
  //   1 = Red only
  //   2 = Red + IR (SpO2/Heart rate mode)
  //
  // Argument 4: sampleRate (Samples taken per second)
  //   Possible values: 50, 100, 200, 400, 800, 1000, 1600, 3200
  //
  // Argument 5: pulseWidth (LED on-time in microseconds)
  //   Possible values:
  //   69  (15-bit ADC resolution)
  //   118 (16-bit ADC resolution)
  //   215 (17-bit ADC resolution)
  //   411 (18-bit ADC resolution - best SNR)
  //
  // Argument 6: adcRange (Full scale ADC range)
  //   Possible values:
  //   2048  (7.81pA per LSB - Most sensitive)
  //   4096  (15.63pA per LSB)
  //   8192  (31.25pA per LSB)
  //   16384 (62.5pA per LSB - Least sensitive/Largest range)
  // sampleRate=800, sampleAverage=8 → effective output = 800/8 = 100 samples/sec.
  // At 100 Hz the Serial Plotter's 500-point window covers exactly 5 seconds,
  // showing ~5-6 heartbeat pulses at a normal resting rate of 60-70 BPM.
  sensor.setup(0x00, 8, 2, 800, 411, 4096);

  // Overwrite the initial power level (0) with specific amplitudes.
  // Amplitude values range from 0x00 (0mA) to 0xFF (50mA).
  sensor.setPulseAmplitudeIR(40);  // Set IR LED brightness to ~7.8mA
  sensor.setPulseAmplitudeRed(15); // Set Red LED brightness to ~2.9mA

  // --- WAIT UNTIL FINGER IS PLACED ---
  // IR reading above 15,000 reliably indicates a finger is on the sensor.
  // scrollText aborts mid-animation the moment a finger is detected.
  Serial.println(F("Waiting for finger..."));
  while (sensor.getIR() < 15000) {
    if (scrollText("Place your finger on sensor", 1, true)) break; // finger detected mid-scroll — exit immediately
  }

  // Allow the sensor to stabilize after finger placement, then seed the DC baseline.
  // Drain any stale FIFO samples accumulated during the scroll animation before seeding.
  delay(200);
  sensor.check();
  while (sensor.available()) sensor.nextSample(); // flush stale FIFO samples
  dcIR = sensor.getIR();

  // Show a brief "Plotting..." confirmation on the OLED before handing off to Serial Plotter
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, (SCREEN_HEIGHT - 16) / 2);
  display.print("Plotting... check Serial Plotter");
  display.display();
  delay(800);

  Serial.println(F("Finger detected. Plotting started."));
}

// ========================================================================
// MAIN LOOP
// ========================================================================
void loop() {
  // Wait for exactly one new sample from the FIFO before proceeding.
  // This locks the output rate to the sensor's effective sample rate (100 Hz),
  // eliminating FIFO backlog bursts that caused pulses to appear stretched.
  sensor.check();
  if (!sensor.available()) return; // no new sample yet — try again next call

  int32_t ir = sensor.getIR(); // read current sample
  sensor.nextSample();         // advance FIFO pointer so the next call gets the next sample

  // ---------------------------------------------------------
  // No finger: show scroll prompt on OLED, output 0 to Serial
  // Plotter so the trace stays flat and readable.
  // Abort as soon as the finger is placed again.
  // ---------------------------------------------------------
  if (ir < 15000) {
    Serial.println(0); // flat line on Serial Plotter while waiting
    while (true) {
      if (scrollText("Place your finger on sensor", 1, true)) break; // finger detected mid-scroll
      if (sensor.getIR() >= 15000) break;                            // finger detected between passes
    }
    // Flush stale FIFO samples and re-seed DC baseline cleanly
    delay(200);
    sensor.check();
    while (sensor.available()) sensor.nextSample();
    dcIR = sensor.getIR();

    // Brief confirmation before resuming plot
    display.clearDisplay();
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, (SCREEN_HEIGHT - 16) / 2);
    display.print("Plotting... check Serial Plotter");
    display.display();
    delay(800);
    return;
  }

  // ---------------------------------------------------------
  // Finger present: extract AC component and send to plotter
  // ---------------------------------------------------------

  // Low-Pass Filter (Exponential Moving Average) to track the DC baseline.
  // Bit-shifting right by 4 (>> 4) is a highly efficient way to divide by 16.
  // This smoothly updates the baseline dcIR towards the current ir value.
  // (Using >> 4 instead of >> 6 makes it adapt to baseline changes much faster).
  dcIR = dcIR + ((ir - dcIR) >> 4);

  // Extract the AC component (the actual heartbeat pulse).
  // During a heartbeat, more blood is in the finger, which absorbs MORE light.
  // Therefore, the raw 'ir' value drops. Subtracting 'ir' from 'dcIR' inverts
  // the signal so that heartbeat pulses appear as upward-pointing peaks.
  int32_t ac = dcIR - ir;

  // Print the AC value to the Serial Plotter.
  // Dividing by 8 scales down the amplitude so it fits nicely on the screen
  // without clipping or excessive noise (assumes adcRange=4096).
  Serial.println(ac / 8);
  // No delay needed — timing is governed by sensor.available() above (100 Hz)
}
