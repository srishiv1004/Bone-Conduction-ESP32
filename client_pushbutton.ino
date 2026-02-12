#include <WiFi.h>
#include <ArduinoWebsockets.h>

// --- Configuration ---
const char* ssid = "Micky_Project";
const char* password = "password123";
const char* serverAddress = "192.168.4.1"; 

// --- Button Pins ---
const int GREEN_BUTTON = 14; // Ready command
const int RED_BUTTON = 12;   // Danger command

// --- State Tracking (Prevents infinite loops) ---
bool lastGreenState = HIGH;
bool lastRedState = HIGH;

using namespace websockets;
WebsocketsClient client;

void setup() {
    Serial.begin(115200);
    
    // Set up button pins with internal pull-up resistors
    pinMode(GREEN_BUTTON, INPUT_PULLUP);
    pinMode(RED_BUTTON, INPUT_PULLUP);
    
    while (!Serial) { delay(10); } 
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

    // 2. Connect to Server
    Serial.print("Connecting to Voice Channel");
    while (!client.connect(serverAddress, 8888, "/")) {
        delay(500);
        Serial.print("?");
    }
    Serial.println("\nSUCCESS! LINK ESTABLISHED.");
    Serial.println("Press Green (14) for READY or Red (12) for DANGER.");
}

void loop() {
    client.poll();

    // Read the current physical state of the buttons
    bool currentGreen = digitalRead(GREEN_BUTTON);
    bool currentRed = digitalRead(RED_BUTTON);

    // --- Green Button Logic (READY) ---
    // Trigger only when state changes from Not-Pressed (HIGH) to Pressed (LOW)
    if (currentGreen == LOW && lastGreenState == HIGH) {
        Serial.println("Button Pressed: Sending READY");
        client.send("READY");
        delay(1000); // Debounce delay
    }
    lastGreenState = currentGreen;

    // --- Red Button Logic (DANGER) ---
    if (currentRed == LOW && lastRedState == HIGH) {
        Serial.println("Button Pressed: Sending DANGER");
        client.send("DANGER");
        delay(1000); // Debounce delay
    }
    lastRedState = currentRed;

    // Optional: Still allow Serial Monitor input as a backup
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() > 0) {
            input.toUpperCase();
            client.send(input);
        }
    }
}