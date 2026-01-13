#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// GPIO 2 is the standard Onboard LED for most ESP32 boards
const int ledPin = 2; 

// Unique IDs for the Service and Characteristic
// (You can generate your own, but these will work fine)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool deviceConnected = false;

// Setup callbacks to handle connection states
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device Connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
      // Restart advertising so you can connect again
      pServer->getAdvertising()->start(); 
    }
};

// Setup callbacks to handle incoming data (Writing to the ESP32)
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // FIX: specific conversion to Arduino String using .c_str()
      String value = pCharacteristic->getValue().c_str();

      if (value.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < value.length(); i++) {
          Serial.print(value[i]);
        }
        Serial.println();

        // Check the first character
        if (value[0] == '1') {
          digitalWrite(ledPin, HIGH); // Turn LED ON
          Serial.println("LED ON");
        }
        else if (value[0] == '0') {
          digitalWrite(ledPin, LOW);  // Turn LED OFF
          Serial.println("LED OFF");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // 1. Initialize BLE
  BLEDevice::init("ESP32_iPhone_Control"); // Device Name

  // 2. Create the Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create the Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create the Characteristic
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // 5. Start the Service
  pService->start();

  // 6. Start Advertising (so the iPhone can find it)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("Waiting for iPhone connection...");
}

void loop() {
  // BLE works via callbacks, so the loop can be empty 
  // or used for other tasks.
  delay(2000);
}
