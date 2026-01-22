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
//                  ENTER YOUR WIFI CREDENTIALS HERE                //
// ---------------------------------------------------------------- //
const char* ssid     = "WIFI-NAME";
const char* password = "WIFI-PASSWORD";

WiFiServer server(80);
const int ledPin = 2;

String header;
String output2State = "off";

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("---------------------------------------");
  Serial.println("WiFi Connected!");
  Serial.print("Control page available at: http://");
  Serial.println(WiFi.localIP());
  Serial.println("---------------------------------------");
  
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    // Only print when a new user connects, not every character they send
    // Serial.println("New Client Connected"); 
    
    String currentLine = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // REMOVED: Serial.write(c); <--- This was the noisy line
        header += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Logic for GPIO 2
            if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("[ACTION] LED turned ON"); // Clean Log
              output2State = "on";
              digitalWrite(ledPin, HIGH);
            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("[ACTION] LED turned OFF"); // Clean Log
              output2State = "off";
              digitalWrite(ledPin, LOW);
            }
            
            // HTML & CSS
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            client.println("<style>");
            client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("h1 { color: #333; padding: 2vh;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; border-radius: 10px;}");
            client.println(".button2 {background-color: #555555;}");
            client.println(".status {font-size: 20px; color: #555; margin-bottom: 20px;}");
            client.println("</style></head>");
            
            client.println("<body><h1>ESP32 Control Panel</h1>");
            client.println("<p class='status'>Device Status: <strong>" + output2State + "</strong></p>");
            
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
