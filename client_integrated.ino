#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "driver/i2s.h"

// --- HARDWARE CONFIG ---
#define I2S_WS_RX  27
#define I2S_SCK_RX 14
#define I2S_SD_RX  12
#define BUTTON_PIN 32  

using namespace websockets;
WebsocketsClient client;

// Timing variables for Single-Button Logic
unsigned long buttonPressTime = 0;
bool isPressing = false;
const int LONG_PRESS_TIME = 1000; // 1 second threshold

// I2S Configuration
const i2s_config_t i2s_config_rx = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = 16000,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 32,
  .dma_buf_len = 64
};

const i2s_pin_config_t pin_config_rx = {
  .bck_io_num = I2S_SCK_RX,
  .ws_io_num = I2S_WS_RX,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_SD_RX
};

// Audio Scaling Logic
void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len) {
    uint32_t j = 0;
    for (int i = 0; i < len; i += 2) {
        uint16_t dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 2048 ;
    }
}

static void i2s_adc_task(void *arg) {
    i2s_driver_install(I2S_NUM_1, &i2s_config_rx, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &pin_config_rx);
    size_t bytes_read;
    uint8_t* i2s_read_buff = (uint8_t*) calloc(1024, sizeof(uint8_t));
    uint8_t* flash_write_buff = (uint8_t*) calloc(1024, sizeof(uint8_t));

    while (1) {
        if (client.available()) {
            i2s_read(I2S_NUM_1, (void*) i2s_read_buff, 1024, &bytes_read, portMAX_DELAY);
            i2s_adc_data_scale(flash_write_buff, i2s_read_buff, 1024);
            client.sendBinary((const char*)flash_write_buff, 1024);
        }
        vTaskDelay(1);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    WiFi.begin("Micky_Project", "password123");
    while (WiFi.status() != WL_CONNECTED) { delay(500); }
    while (!client.connect("192.168.4.1", 8888, "/")) { delay(500); }
    
    xTaskCreate(i2s_adc_task, "MicTask", 4096, NULL, 1, NULL);
    Serial.println("CLIENT READY - SINGLE BUTTON (G32)");
} 

void loop() {
    client.poll();
    int buttonState = digitalRead(BUTTON_PIN);

    // Press detected
    if (buttonState == LOW && !isPressing) {
        buttonPressTime = millis();
        isPressing = true;
    }

    // Release detected
    if (buttonState == HIGH && isPressing) {
        unsigned long holdDuration = millis() - buttonPressTime;
        
        if (holdDuration >= LONG_PRESS_TIME) {
            Serial.println("LONG PRESS: DANGER SENT");
            client.send("DANGER");
        } else if (holdDuration > 50) { // Debounce
            Serial.println("SHORT CLICK: READY SENT");
            client.send("READY");
        }
        isPressing = false;
    }

    if (Serial.available()) {
        String sms = Serial.readStringUntil('\n'); sms.trim();
        if (sms.length() > 0) client.send(sms);
    }
}