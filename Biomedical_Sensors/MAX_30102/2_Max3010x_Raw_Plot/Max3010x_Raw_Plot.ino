 /*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: This firmware is designed for plotting raw Photoplethysmography 
 *           (PPG) data using the MAX30105 optical sensor. It is intended for sensor 
 *           evaluation, algorithm development, and educational purposes.
 * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * * Dependencies & Credits:
 * - SparkFun MAX3010x Sensor Library (BSD License)
 * - PBA & SpO2 Algorithms by Maxim Integrated Products (MIT-style License)
 * - Adafruit GFX and SSD1306 Libraries by Adafruit Industries (BSD License)
 * ------------------------------------------------------------------------
 */

#include <Wire.h>         // I2C communication library
#include "MAX30105.h"     // SparkFun MAX3010x optical sensor library

// Instantiate the sensor object
MAX30105 sensor;

// Variable to store the DC (baseline) component of the IR signal
// Using int32_t to accommodate the large 18-bit values from the sensor
int32_t dcIR = 0;

void setup() {
  // Initialize serial communication at 115200 baud for the Serial Plotter
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

  // Attempt to communicate with the sensor at 400kHz (Fast I2C speed).
  // If the sensor is not found, trap the code in an infinite loop.
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Sensor not found. Check wiring!");
    while(1); 
  }

  // Configure the sensor with specific parameters:
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
  //   3 = MultiLED (Red + IR + Green)
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
  sensor.setup(0x00, 8, 2, 1200, 411, 4096);

  // Overwrite the initial power level (0) with specific amplitudes.
  // Amplitude values range from 0x00 (0mA) to 0xFF (50mA).
  sensor.setPulseAmplitudeIR(40);  // Set IR LED brightness to ~7.8mA
  sensor.setPulseAmplitudeRed(15); // Set Red LED brightness to ~2.9mA

  // Wait a moment for the sensor to stabilize and fill its FIFO buffer
  delay(200);
  
  // Initialize our baseline DC tracker with the very first IR reading
  dcIR = sensor.getIR();
}

void loop() {
  // Fetch the latest raw Infrared (IR) reading from the sensor
  int32_t ir = sensor.getIR();

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
  // without clipping or excessive noise.
  Serial.println(ac / 8);

  // Small delay to prevent flooding the serial port, 
  // roughly matching the loop speed to the sensor's configured sample rate.
  delay(10);
}
