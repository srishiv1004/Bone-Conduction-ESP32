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

// This creates a solid "Vibe" rather than a high-pitched buzz
void playVibe(int duration_ms) {
    size_t bw;
    // We send a 200Hz square wave - perfect for bone conduction
    int16_t sample_high = 20000;
    int16_t sample_low = -20000;
    
    unsigned long startTime = millis();
    while(millis() - startTime < duration_ms) {
        for(int i=0; i<40; i++) i2s_write(I2S_PORT, &sample_high, 2, &bw, portMAX_DELAY);
        for(int i=0; i<40; i++) i2s_write(I2S_PORT, &sample_low, 2, &bw, portMAX_DELAY);
    }
    i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
    Serial.begin(115200);
    i2sInit();
    
    WiFi.softAP("Micky_Project", "password123");
    server.listen(8888);
    Serial.println("SYSTEM READY. SEND COMMAND.");
}

void loop() {
    if (server.poll()) client = server.accept();
    
    if (client.available()) {
        WebsocketsMessage msg = client.readBlocking();
        String cmd = msg.c_str();
        cmd.trim(); cmd.toUpperCase();
        
        if (cmd == "READY") {
            Serial.println("Pulsing: READY");
            playVibe(300); // 0.3 second vibration
        }
        else if (cmd == "DANGER") {
            Serial.println("Pulsing: DANGER");
            for(int i=0; i<3; i++) {
                playVibe(150);
                delay(100);
            }
        }
    }
}