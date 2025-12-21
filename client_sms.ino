#include <WiFi.h>
#include <ArduinoWebsockets.h>

// --- Configuration ---
const char* ssid = "Micky_Project";
const char* password = "password123";
const char* serverAddress = "192.168.4.1"; 

using namespace websockets;
WebsocketsClient client;

void setup() {
    Serial.begin(115200);
    
    // Wait for Serial Monitor to be opened
    while (!Serial) { 
        delay(10); 
    } 
    delay(1000);
    
    Serial.println("\n\n=== CLIENT INITIALIZING ===");

    // 1. Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Searching for Transducer WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Joined!");

    // 2. Connect to Voice Server
    Serial.print("Connecting to Voice Channel");
    while (!client.connect(serverAddress, 8888, "/")) {
        delay(500);
        Serial.print("?");
    }
    Serial.println("\nSUCCESS! LINK ESTABLISHED.");
    Serial.println("Type 'READY', 'DANGER', or 'BUZZ' and press Enter.");
}

void loop() {
    client.poll();

    // Send commands from Serial Monitor to Transducer
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() > 0) {
            Serial.print("Sending Command: "); 
            Serial.println(input);
            client.send(input);
        }
    }
}