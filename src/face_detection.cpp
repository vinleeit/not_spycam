#include <Arduino.h>
#include "fd_forward.h"
#include "fr_forward.h"
#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7
#define BUTTON 13

static mtmn_config_t mtmnConfig = {0};
static int detections = 0;
static bool isRecognitionEnabled = true;
static int8_t isEnrolling = 0;
static face_id_list idList = {0};

static int runFaceRecognition(dl_matrix3du_t *imageMatrix, box_array_t *boxes)
{
    dl_matrix3du_t *alignedFace = NULL;
    int matchedID = 0;

    alignedFace = dl_matrix3du_alloc(1, 56, 56, 3);
    if (alignedFace == NULL)
    {
        Serial.println("Could not allocate face recognition buffer!");
        return matchedID;
    }
    if (align_face(boxes, imageMatrix, alignedFace) == ESP_OK)
    {
        if (isEnrolling == 1)
        {
            int8_t leftSampleFace = enroll_face(&idList, alignedFace);

            if (leftSampleFace == (ENROLL_CONFIRM_TIMES - 1))
            {
                Serial.printf("Enrolling Face ID: %d\n", idList.tail);
            }
            Serial.printf("Enrolling Face ID: %d sample %d\n", idList.tail, ENROLL_CONFIRM_TIMES - leftSampleFace);

            // rgb_printf(imageMatrix, FACE_COLOR_SYAN, "ID[%u] Sample [%u]", idList.tail, ENROLL_CONFIRM_TIMES - leftSampleFace);
            if (leftSampleFace == 0)
            {
                isEnrolling = 0;
                Serial.printf("Enrolled Face ID: %d\n", idList.tail);
            }
        }
        else
        {
            matchedID = recognize_face(&idList, alignedFace);
            if (matchedID >= 0)
            {
                Serial.printf("Match Face ID: %u\n", matchedID);
                // rgb_printf(imageMatrix, FACE_COLOR_GREEN, "Hello Subject %u", matchedID);
            }
            else
            {
                Serial.println("No Match Found");
                // rgb_print(imageMatrix, FACE_COLOR_RED, "Intruder Alert!");
                matchedID = -1;
            }
        }
    }
    else
    {
        Serial.println("Face Not Aligned");
        // rgb_print(image_matrix, FACE_COLOR_YELLOW, "Human Detected");
    }
    dl_matrix3du_free(alignedFace);
    return matchedID;
}

// Setup
void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    pinMode(BUTTON, INPUT);

    Serial.println("===<Setup>===");

    // Config
    Serial.print("# config: ");
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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    Serial.println("successful!");

    Serial.print("# psram: ");
    // init with high specs to pre-allocate larger buffers
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
    Serial.println("succesful!");

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif

    // Camera init
    Serial.print("# camera init: ");
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV2640_PID)
    {
        s->set_vflip(s, 1);       // flip it back
        s->set_brightness(s, 1);  // up the blightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    s->set_framesize(s, FRAMESIZE_QVGA);
    Serial.println("successful!");

#if defined(CAMERA_MODEL_M5STACK_WIDE)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#endif

    // MTMN
    Serial.print("# mtmn: ");
    mtmnConfig = mtmn_init_config();
    mtmnConfig.type = FAST;
    mtmnConfig.min_face = 80;
    mtmnConfig.pyramid = 0.707;
    mtmnConfig.pyramid_times = 4;
    mtmnConfig.p_threshold.score = 0.6;
    mtmnConfig.p_threshold.nms = 0.7;
    mtmnConfig.p_threshold.candidate_number = 20;
    mtmnConfig.r_threshold.score = 0.7;
    mtmnConfig.r_threshold.nms = 0.7;
    mtmnConfig.r_threshold.candidate_number = 10;
    mtmnConfig.o_threshold.score = 0.7;
    mtmnConfig.o_threshold.nms = 0.7;
    mtmnConfig.o_threshold.candidate_number = 1;
    Serial.println("successful!");

    // Face ID init
    Serial.println("face id init: ");
    face_id_init(&idList, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
    Serial.println("successful!");
    Serial.println("===<Setup Complete>===");
}

void loop()
{
    if (digitalRead(BUTTON) == LOW)
    {
        if (isEnrolling == 0)
        {
            Serial.println("Enrolling!");
            isEnrolling = 1;
        }
        else
        {
            Serial.println("Enrolling canceled");
            isEnrolling = 0;
        }
    }

    camera_fb_t *frame;
    frame = esp_camera_fb_get();
    int faceID = 0;

    dl_matrix3du_t *imageMatrix = dl_matrix3du_alloc(1, frame->width, frame->height, 3);
    fmt2rgb888(frame->buf, frame->len, frame->format, imageMatrix->item);

    esp_camera_fb_return(frame);

    box_array_t *boxes = face_detect(imageMatrix, &mtmnConfig);

    if (boxes != NULL)
    {
        if (isRecognitionEnabled)
        {
            faceID = runFaceRecognition(imageMatrix, boxes);
        }

        detections += 1;
        Serial.printf("Faces detected %d times!\n", detections);
        dl_lib_free(boxes->score);
        dl_lib_free(boxes->box);
        dl_lib_free(boxes->landmark);
        dl_lib_free(boxes);
    }
    dl_matrix3du_free(imageMatrix);
}
