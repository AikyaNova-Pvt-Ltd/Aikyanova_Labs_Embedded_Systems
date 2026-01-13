void setup() {
  // Initialize Serial at 115200 baud rate
  Serial.begin(115200);

  // Wait a moment for the serial connection to stabilize
  delay(1000);

  // Send a startup message to check Transmission (TX)
  Serial.println("-----------------------------------------");
  Serial.println("ESP32 USB-Serial Communication Test");
  Serial.println("Type something in the input field above!");
  Serial.println("-----------------------------------------");
}

void loop() {
  // Check if data is available to read (RX Check)
  if (Serial.available() > 0) {
    // Read the incoming string until a newline character
    String receivedData = Serial.readStringUntil('\n');

    // Print it back to the monitor (Echo)
    Serial.print("ESP32 Received: ");
    Serial.println(receivedData);
    Serial.println("USB-Serial communication PASSED");
  }
}
