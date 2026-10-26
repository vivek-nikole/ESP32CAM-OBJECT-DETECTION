// ESP32-CAM live object detection with an Edge Impulse model
// Board: AI-Thinker ESP32-CAM, PSRAM Enabled, Partition Scheme: Huge APP
// Camera runs in RGB565 (sensors without JPEG support). Each frame is center-cropped
// to a square and resized to the model input size, matching "Fit shortest axis" training.
// Results are printed to Serial Monitor (115200 baud).

// Library name taken from your Edge Impulse example (esp32_camera.ino, line 27)
#include <Object_Detection_LeandGren_inferencing.h>

#include "esp_camera.h"

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM        35
#define Y8_GPIO_NUM        34
#define Y7_GPIO_NUM        39
#define Y6_GPIO_NUM        36
#define Y5_GPIO_NUM        21
#define Y4_GPIO_NUM        19
#define Y3_GPIO_NUM        18
#define Y2_GPIO_NUM         5
#define VSYNC_GPIO_NUM     25
#define HREF_GPIO_NUM      23
#define PCLK_GPIO_NUM      22

#define CAM_W 320
#define CAM_H 240

static camera_fb_t *current_fb = NULL;

// Called by the Edge Impulse SDK to fetch pixels from the current frame
static int get_data(size_t offset, size_t length, float *out_ptr) {
  const int crop = CAM_H;                 // 240x240 center square
  const int x0 = (CAM_W - crop) / 2;      // 40 px left margin
  for (size_t i = 0; i < length; i++) {
    size_t idx = offset + i;
    int mx = idx % EI_CLASSIFIER_INPUT_WIDTH;
    int my = idx / EI_CLASSIFIER_INPUT_WIDTH;
    int sx = x0 + (mx * crop) / EI_CLASSIFIER_INPUT_WIDTH;
    int sy = (my * crop) / EI_CLASSIFIER_INPUT_HEIGHT;
    size_t p = ((size_t)sy * CAM_W + sx) * 2;
    uint16_t v = ((uint16_t)current_fb->buf[p] << 8) | current_fb->buf[p + 1];
    uint8_t r = ((v >> 11) & 0x1F) << 3;
    uint8_t g = ((v >> 5) & 0x3F) << 2;
    uint8_t b = (v & 0x1F) << 3;
    out_ptr[i] = (r << 16) | (g << 8) | b;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
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
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size   = FRAMESIZE_QVGA;   // must stay 320x240 (same as data collection)
  config.fb_count     = 1;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init FAILED with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera init SUCCESS. Starting detection...");
}

void loop() {
  current_fb = esp_camera_fb_get();
  if (!current_fb) {
    Serial.println("Frame capture failed");
    delay(500);
    return;
  }

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &get_data;

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
  esp_camera_fb_return(current_fb);
  current_fb = NULL;

  if (res != EI_IMPULSE_OK) {
    Serial.printf("Classifier error: %d\n", res);
    delay(500);
    return;
  }

  bool found = false;
  for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
    ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
    if (bb.value == 0) continue;
    found = true;
    Serial.printf("DETECTED: %s (%.0f%%) at x=%u y=%u w=%u h=%u\n",
                  bb.label, bb.value * 100, bb.x, bb.y, bb.width, bb.height);
  }
  if (!found) Serial.println("No object detected");

  delay(200);
}
