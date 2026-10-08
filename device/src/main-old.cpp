#include "wifi_cfg.h"
#include "portal.h"
#include "esp_camera.h"
#include <WiFi.h>

#include "app_httpd.cpp"

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

#include "camera_pins.h"

bool init_camera()
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
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition

    config.frame_size = FRAMESIZE_UXGA;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    //                      for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        if (psramFound())
        {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        }
        else
        {
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    }
    else
    {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_240X240;
    }

    // Initialize camera
    if (esp_err_t err = esp_camera_init(&config); err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }

    // Additional sensor settings
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL)
    {
        s->set_brightness(s, 1);                 // -2 to 2
        s->set_contrast(s, 0);                   // -2 to 2
        s->set_saturation(s, -2);                // -2 to 2
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
        s->set_vflip(s, 1);                      // 0 = disable , 1 = enable
        s->set_dcw(s, 1);                        // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);                   // 0 = disable , 1 = enable
    }

    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        s->set_framesize(s, FRAMESIZE_QVGA);
    }

    return true;
}

void setup()
{
    delay(3000);
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("Starting...");

    WifiCfg c;
    if (!(loadCfg(c) && connectSTA(c)))
    {
        connectAP();
        startPortal(AP_IP);
    }
    else
    {
        const bool is_ok = init_camera();
        if (!is_ok)
        {
            return;
        }
        startCameraServer();

        delay(1000);
        // Serial.printf("Camera Ready! Use 'http://%s to connect", ((is_ap) ? WiFi.softAPIP() : WiFi.localIP()).toString().c_str());
    }
}

void loop()
{
    if (portalMode)
    {
        portalLoop();
    }
}
