#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "driver/i2s.h"

#define I2S_PORT I2S_NUM_0

using namespace websockets;
WebsocketsServer server;
WebsocketsClient client;

void i2sInit() {
    i2s_driver_uninstall(I2S_PORT);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, 
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 32,
        .dma_buf_len = 64
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = 27, .ws_io_num = 26, .data_out_num = 25, .data_in_num = -1
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void playVibe(int duration_ms) {
    size_t bw;
    int32_t s_high = (int32_t)30000 << 16; 
    int32_t s_low = -s_high;
    unsigned long start = millis();
    while(millis() - start < duration_ms) {
        for(int i=0; i<40; i++) i2s_write(I2S_PORT, &s_high, 4, &bw, portMAX_DELAY);
        for(int i=0; i<40; i++) i2s_write(I2S_PORT, &s_low, 4, &bw, portMAX_DELAY);
        yield(); 
    }
    i2s_zero_dma_buffer(I2S_PORT);
}

// Morse Helper Functions
String getMorseChar(char c) {
    switch (toupper(c)) {
        case 'A': return ".-";   case 'B': return "-..."; case 'C': return "-.-.";
        case 'D': return "-..";  case 'E': return ".";    case 'F': return "..-.";
        case 'G': return "--.";  case 'H': return "...."; case 'I': return "..";
        case 'J': return ".---"; case 'K': return "-.-";  case 'L': return ".-..";
        case 'M': return "--";   case 'N': return "-.";   case 'O': return "---";
        case 'P': return ".--."; case 'Q': return "--.-"; case 'R': return ".-.";
        case 'S': return "...";  case 'T': return "-";    case 'U': return "..-";
        case 'V': return "...-"; case 'W': return ".--";  case 'X': return "-..-";
        case 'Y': return "-.--"; case 'Z': return "--.."; case ' ': return " ";
        default: return "";
    }
}

void playMorse(String message) {
    for (int i = 0; i < message.length(); i++) {
        String code = getMorseChar(message[i]);
        if (code == " ") delay(600);
        else {
            for (int j = 0; j < code.length(); j++) {
                if (code[j] == '.') playVibe(150);
                else if (code[j] == '-') playVibe(450);
                delay(150); 
            }
            delay(300); 
        }
    }
}

void setup() {
    Serial.begin(115200);
    i2sInit();
    WiFi.softAP("Micky_Project", "password123");
    server.listen(8888);
}

void loop() {
    if (server.poll()) { client = server.accept(); }
    if (client.available()) {
        WebsocketsMessage msg = client.readBlocking();
        if (msg.isText()) {
            String cmd = msg.c_str(); cmd.trim(); cmd.toUpperCase();
            
            if (cmd == "READY") { 
                playVibe(400); // 1 Beep
            }
            else if (cmd == "DANGER") { 
                for(int i=0; i<3; i++) { // 3 Beeps
                    playVibe(200); 
                    delay(200); 
                } 
            } 
            else { playMorse(cmd); }
        } else if (msg.isBinary()) {
            size_t bytes_written;
            i2s_write(I2S_PORT, msg.c_str(), msg.length(), &bytes_written, portMAX_DELAY);
        }
    }
}