#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// WiFi credentials
const char *ssid = "<wifi_ssid>";
const char *password = "<wifi_password>";

// WebSocket server details
const char *websocket_host = "<server_ip_addres>"; // Replace with your server IP
const int websocket_port = 4200;
const char *websocket_path = "/stream";

// Camera configuration for AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// LED GPIO
#define LED_GPIO_NUM 4

WebSocketsClient webSocket;

// Camera variables
camera_fb_t *fb = NULL;
bool camera_initialized = false;
bool websocket_connected = false;

// Timing variables
unsigned long last_frame_time = 0;
const unsigned long frame_interval = 100; // 100ms = ~10 FPS

bool initCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Frame size and quality settings
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_VGA; // 640x480
        config.jpeg_quality = 10;          // 0-63 lower means higher quality
        config.fb_count = 2;
        Serial.println("PSRAM found - using higher quality settings");
    }
    else
    {
        config.frame_size = FRAMESIZE_CIF; // 352x288
        config.jpeg_quality = 12;
        config.fb_count = 1;
        Serial.println("PSRAM not found - using lower quality settings");
    }

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    // Additional sensor settings
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL)
    {
        s->set_brightness(s, 0);                 // -2 to 2
        s->set_contrast(s, 0);                   // -2 to 2
        s->set_saturation(s, 0);                 // -2 to 2
        s->set_special_effect(s, 0);             // 0 to 6 (0-No Effect, 1-Negative, 2-Grayscale, 3-Red Tint, 4-Green Tint, 5-Blue Tint, 6-Sepia)
        s->set_whitebal(s, 1);                   // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);                   // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);                    // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        s->set_exposure_ctrl(s, 1);              // 0 = disable , 1 = enable
        s->set_aec2(s, 0);                       // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);                   // -2 to 2
        s->set_aec_value(s, 300);                // 0 to 1200
        s->set_gain_ctrl(s, 1);                  // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);                   // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
        s->set_bpc(s, 0);                        // 0 = disable , 1 = enable
        s->set_wpc(s, 1);                        // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);                    // 0 = disable , 1 = enable
        s->set_lenc(s, 1);                       // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);                    // 0 = disable , 1 = enable
        s->set_vflip(s, 0);                      // 0 = disable , 1 = enable
        s->set_dcw(s, 1);                        // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);                   // 0 = disable , 1 = enable
    }

    return true;
}

void connectToWiFi()
{
    Serial.printf("Connecting to WiFi: %s\n", ssid);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println();
        Serial.println("Failed to connect to WiFi!");
    }
}
void sendMetadata()
{
    if (!websocket_connected)
        return;

    // Create JSON metadata
    JsonDocument doc;
    doc["type"] = "metadata";
    doc["data"]["name"] = "ESP32-CAM Stream";
    doc["data"]["device"] = "AI Thinker ESP32-CAM";
    doc["data"]["resolution"] = psramFound() ? "VGA (640x480)" : "CIF (352x288)";
    doc["data"]["fps"] = 1000 / frame_interval;

    String jsonString;
    serializeJson(doc, jsonString);

    webSocket.sendTXT(jsonString);
    Serial.println("Metadata sent: " + jsonString);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.println("[WSc] Disconnected!");
        websocket_connected = false;
        break;

    case WStype_CONNECTED:
        Serial.printf("[WSc] Connected to: %s\n", payload);
        websocket_connected = true;

        // Send initial metadata
        sendMetadata();
        break;

    case WStype_TEXT:
        Serial.printf("[WSc] Received text: %s\n", payload);
        break;

    case WStype_BIN:
        Serial.printf("[WSc] Received binary length: %u\n", length);
        break;

    case WStype_PING:
        Serial.println("[WSc] Received ping");
        break;

    case WStype_PONG:
        Serial.println("[WSc] Received pong");
        break;

    case WStype_ERROR:
        Serial.printf("[WSc] Error: %s\n", payload);
        break;

    default:
        break;
    }
}

void initWebSocket()
{
    webSocket.begin(websocket_host, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    Serial.printf("WebSocket connecting to: ws://%s:%d%s\n", websocket_host, websocket_port, websocket_path);
}

void captureAndSendFrame()
{
    if (!websocket_connected)
        return;

    // Capture frame
    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        return;
    }

    // Send binary frame data
    webSocket.sendBIN(fb->buf, fb->len);

    Serial.printf("Frame sent - Size: %d bytes\n", fb->len);

    // Return frame buffer
    esp_camera_fb_return(fb);
}

// Additional utility functions
void restartCamera()
{
    esp_camera_deinit();
    delay(100);
    if (initCamera())
    {
        Serial.println("Camera restarted successfully");
    }
    else
    {
        Serial.println("Camera restart failed");
    }
}

void printCameraInfo()
{
    if (camera_initialized)
    {
        sensor_t *s = esp_camera_sensor_get();
        if (s != NULL)
        {
            Serial.printf("PID: 0x%02X\n", s->id.PID);
            Serial.printf("VER: 0x%02X\n", s->id.VER);
            Serial.printf("MIDL: 0x%02X\n", s->id.MIDL);
            Serial.printf("MIDH: 0x%02X\n", s->id.MIDH);
        }
    }
}

void setup()
{
    // Disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    Serial.println("ESP32-CAM WebSocket Streamer Starting...");

    // Initialize LED
    pinMode(LED_GPIO_NUM, OUTPUT);

    // Initialize camera
    if (initCamera())
    {
        Serial.println("Camera initialized successfully");
        camera_initialized = true;
    }
    else
    {
        Serial.println("Camera initialization failed!");
        return;
    }

    // Connect to WiFi
    connectToWiFi();

    // Initialize WebSocket
    initWebSocket();
}

void loop()
{
    webSocket.loop();

    if (websocket_connected && camera_initialized)
    {
        unsigned long current_time = millis();

        // Send frame at specified interval
        if (current_time - last_frame_time >= frame_interval)
        {
            captureAndSendFrame();
            last_frame_time = current_time;
        }
    }

    delay(1);
}