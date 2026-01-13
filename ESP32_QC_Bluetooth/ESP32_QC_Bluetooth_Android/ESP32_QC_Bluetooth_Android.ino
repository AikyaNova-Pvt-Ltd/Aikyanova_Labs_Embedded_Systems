#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// GPIO 2 is the standard Onboard LED for most ESP32 boards (e.g., ESP32 DevKit V1)
const int ledPin = 2; 
int incoming;

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  
  // Initialize Bluetooth Serial
  // "ESP32_LED_Control" is the name you will see on your phone
  SerialBT.begin("ESP32_LED_Control"); 
  Serial.println("The device started, now you can pair it with bluetooth!");

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Check if data is coming from the Bluetooth connection
  if (SerialBT.available()) {
    incoming = SerialBT.read(); // Read the incoming byte

    Serial.print("Received: ");
    Serial.println((char)incoming);

    if (incoming == '1') {
      digitalWrite(ledPin, HIGH); // Turn LED ON
      SerialBT.println("LED turned ON"); // Send feedback to phone
    }
    else if (incoming == '0') {
      digitalWrite(ledPin, LOW); // Turn LED OFF
      SerialBT.println("LED turned OFF"); // Send feedback to phone
    }
  }
  
  // Optional: Allow sending commands from Serial Monitor to Phone
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  
  delay(20);
}
