/*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * * This firmware is part of the AikyaNova Labs ESP32 Quality Control (QC) 
 * Testing Suite. It is intended for hardware validation and educational 
 * purposes.
 *
 * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * ------------------------------------------------------------------------
 */

#define LED_PIN 2   // Built-in LED pin (usually GPIO 2)

void setup() {
  pinMode(LED_PIN, OUTPUT);   // Set LED pin as output
}

void loop() {
  digitalWrite(LED_PIN, HIGH);  // Turn LED ON
  delay(1000);                  // Wait 1 second
  digitalWrite(LED_PIN, LOW);   // Turn LED OFF
  delay(1000);                  // Wait 1 second
}