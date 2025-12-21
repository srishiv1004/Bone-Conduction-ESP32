#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "driver/i2s.h"

#define I2S_PORT I2S_NUM_0
#define I2S_WS   26  
#define I2S_BCK  27  
#define I2S_DIN  25  

using namespace websockets;
WebsocketsServer server;
WebsocketsClient client;

void i2sInit() {
    i2s_driver_uninstall(I2S_PORT);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK, .ws_io_num = I2S_WS, .data_out_num = I2S_DIN, .data_in_num = -1
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void vibe(int duration_ms) {
    size_t bw;
    int16_t s_high = 18000;
    int16_t s_low = -18000;
    unsigned long start = millis();
    while(millis() - start < duration_ms) {
        for(int i=0; i<35; i++) i2s_write(I2S_PORT, &s_high, 2, &bw, portMAX_DELAY);
        for(int i=0; i<35; i++) i2s_write(I2S_PORT, &s_low, 2, &bw, portMAX_DELAY);
    }
    i2s_zero_dma_buffer(I2S_PORT);
    delay(150); // Gap between dots/dashes
}

String getMorse(char c) {
    switch (toupper(c)) {
        case 'A': return ".-";    case 'B': return "-...";  case 'C': return "-.-.";
        case 'D': return "-..";   case 'E': return ".";     case 'F': return "..-.";
        case 'G': return "--.";   case 'H': return "....";  case 'I': return "..";
        case 'J': return ".---";  case 'K': return "-.-";   case 'L': return ".-..";
        case 'M': return "--";    case 'N': return "-.";    case 'O': return "---";
        case 'P': return ".--.";  case 'Q': return "--.-";  case 'R': return ".-.";
        case 'S': return "...";   case 'T': return "-";     case 'U': return "..-";
        case 'V': return "...-";  case 'W': return ".--";   case 'X': return "-..-";
        case 'Y': return "-.--";  case 'Z': return "--..";
        case '1': return ".----"; case '2': return "..---"; case '3': return "...--";
        case '4': return "....-"; case '5': return "....."; case '6': return "-....";
        case '7': return "--..."; case '8': return "---.."; case '9': return "----.";
        case '0': return "-----"; case ' ': return " ";
        default: return "";
    }
}

void playMorse(String message) {
    for (int i = 0; i < message.length(); i++) {
        String code = getMorse(message[i]);
        if (code == " ") {
            delay(600); // Gap between words
        } else {
            for (int j = 0; j < code.length(); j++) {
                if (code[j] == '.') vibe(150); 
                else if (code[j] == '-') vibe(450);
            }
            delay(300); // Gap between letters
        }
    }
}

void setup() {
    Serial.begin(115200);
    i2sInit();
    WiFi.softAP("Micky_Project", "password123");
    server.listen(8888);
    Serial.println("--- BONE CONDUCTION MORSE READY ---");
}

void loop() {
    if (server.poll()) client = server.accept();
    if (client.available()) {
        String cmd = client.readBlocking().c_str();
        cmd.trim();
        Serial.println("Incoming Message: " + cmd);
        playMorse(cmd);
    }
}