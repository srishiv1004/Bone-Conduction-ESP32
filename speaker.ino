#include <SPI.h>
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include "driver/i2s.h"

#define I2S_WS_TX       12
#define I2S_SCK_TX      13
#define I2S_DATA_OUT_TX 15

#define I2S_PORT        I2S_NUM_0
#define I2S_SAMPLE_RATE 16000
#define I2S_SAMPLE_BITS 32
#define UPDATE_INTERVAL 500   // ms

const char* ssid = "Srivani";
const char* password = "srivani1905";

using namespace websockets;
WebsocketsServer server;
WebsocketsClient client;

bool isConnected = false;
unsigned long last_update_sent = 0;

// I2S TX configuration
const i2s_config_t i2s_config_tx = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = I2S_SAMPLE_RATE,
  .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 32,
  .dma_buf_len = 64
};

const i2s_pin_config_t pin_config_tx = {
  .bck_io_num = I2S_SCK_TX,
  .ws_io_num = I2S_WS_TX,
  .data_out_num = I2S_DATA_OUT_TX,
  .data_in_num = I2S_PIN_NO_CHANGE
};

// -------------------- Filtering --------------------
float GAIN = 1.0;                // Amplification factor (now adjustable)
int32_t NOISE_THRESHOLD = 500;  // Noise gate threshold (now adjustable)
#define ALPHA 0.9             // High-pass filter coefficient

int32_t prev_sample = 0;

int32_t highPassFilter(int32_t sample) {
    int32_t filtered = ALPHA * (prev_sample + sample - prev_sample);
    prev_sample = filtered;
    return filtered;
}

int32_t applyGain(int32_t sample) {
    int64_t temp = (int64_t)sample * GAIN;
    if (temp > INT32_MAX) temp = INT32_MAX;
    if (temp < INT32_MIN) temp = INT32_MIN;
    return (int32_t)temp;
}

// -------------------- Setup --------------------
void setup() {
    Serial.begin(115200);

    // Start WiFi AP
    WiFi.softAP(ssid, password);
    Serial.print("Access Point Started. IP: ");
    Serial.println(WiFi.softAPIP());

    // Start WebSocket server
    server.listen(8888);
    Serial.println("WebSocket server listening on port 8888");

    // Start task for listening to clients
    xTaskCreate(server_task, "server_task", 4096, NULL, 1, NULL);
}

// -------------------- Loop --------------------
void loop() {
    i2s_write_from_client();
}

// -------------------- WebSocket server task --------------------
static void server_task(void *arg) {
    while (1) {
        if (server.poll()) {
            client = server.accept();
            Serial.println("Client connected!");
            i2sInit();
            last_update_sent = millis();
            isConnected = true;
        }

        if (isConnected && millis() - last_update_sent > UPDATE_INTERVAL) {
            Serial.println("Client disconnected or timed out. Stopping I2S...");
            isConnected = false;
            i2s_driver_uninstall(I2S_PORT);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// -------------------- I2S write with filtering and adjustable settings --------------------
void i2s_write_from_client() {
    if (isConnected && client.available()) {
        last_update_sent = millis();
        client.poll();
        WebsocketsMessage msg = client.readBlocking();

        // Check if message is a command to adjust settings
        String data = msg.c_str();
        if (data.startsWith("GAIN:")) {
            GAIN = data.substring(5).toFloat();
            Serial.print("Updated GAIN to: "); Serial.println(GAIN);
            return;
        }
        if (data.startsWith("THRESH:")) {
            NOISE_THRESHOLD = data.substring(7).toInt();
            Serial.print("Updated NOISE_THRESHOLD to: "); Serial.println(NOISE_THRESHOLD);
            return;
        }

        // Otherwise treat as audio data
        int32_t* samples = (int32_t*)msg.c_str();
        size_t sample_count = msg.length() / sizeof(int32_t);

        for (size_t i = 0; i < sample_count; i++) {
            int32_t filtered = highPassFilter(samples[i]);
            if (abs(filtered) < NOISE_THRESHOLD) filtered = 0;
            samples[i] = applyGain(filtered);
        }

        size_t bytes_written;
        i2s_write(I2S_PORT, (const void*)samples, msg.length(), &bytes_written, portMAX_DELAY);
    }
}

// -------------------- I2S Initialization --------------------
void i2sInit() {
    i2s_driver_install(I2S_PORT, &i2s_config_tx, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config_tx);
    Serial.println("I2S initialized for TX");
}