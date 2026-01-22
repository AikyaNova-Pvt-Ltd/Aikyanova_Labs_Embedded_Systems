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

#include <WiFi.h>

// ---------------------------------------------------------------- //
//             DEFINE YOUR NEW NETWORK CREDENTIALS HERE             //
// ---------------------------------------------------------------- //
const char* ssid     = "ESP32_Hotspot";      // Name of the network to create
const char* password = "password123";        // Password (min 8 chars)

WiFiServer server(80);
const int ledPin = 2;

String header;
String output2State = "off";

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.println("Setting up Access Point...");

  // Start the Access Point
  // You can remove 'password' argument to make it an open network
  WiFi.softAP(ssid, password);

  // Get the IP address of the Access Point
  IPAddress IP = WiFi.softAPIP();
  
  Serial.println("---------------------------------------");
  Serial.println("Access Point Started!");
  Serial.print("Network Name: "); Serial.println(ssid);
  Serial.print("IP Address:   "); Serial.println(IP);
  Serial.println("---------------------------------------");

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // LED Control Logic
            if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("[ACTION] LED turned ON");
              output2State = "on";
              digitalWrite(ledPin, HIGH);
            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("[ACTION] LED turned OFF");
              output2State = "off";
              digitalWrite(ledPin, LOW);
            }
            
            // Same Professional HTML/CSS as before
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            client.println("<style>");
            client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("h1 { color: #333; padding: 2vh;}");
            client.println(".button { background-color: #008CBA; border: none; color: white; padding: 16px 40px;"); // Blue button for AP mode
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; border-radius: 10px;}");
            client.println(".button2 {background-color: #555555;}");
            client.println(".status {font-size: 20px; color: #555; margin-bottom: 20px;}");
            client.println("</style></head>");
            
            client.println("<body><h1>ESP32 Hotspot Control</h1>");
            client.println("<p class='status'>Status: <strong>" + output2State + "</strong></p>");
            
            if (output2State == "off") {
              client.println("<p><a href=\"/2/on\"><button class=\"button\">TURN ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button button2\">TURN OFF</button></a></p>");
            } 
               
            client.println("</body></html>");
            client.println();
            break;
          } else { 
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;      
        }
      }
    }
    header = "";
    client.stop();
  }
}
