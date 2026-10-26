// ESP32-CAM Camera Init Test — RGB565 mode (for sensors without JPEG support, e.g. RHYX M21-45)
// Board: AI-Thinker ESP32-CAM, uploaded via ESP32-CAM-MB USB baseboard
// This sketch does NOT use WiFi or the web server — it just confirms the camera
// initializes and can capture a real frame, printing details to Serial Monitor.

#include "esp_camera.h"

// ===================
// AI-Thinker pin map
// ===================
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

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Starting camera init (RGB565 mode)...");

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

  // --- Key settings for sensors without JPEG support (e.g. RHYX M21-45) ---
  config.pixel_format = PIXFORMAT_RGB565;   // not PIXFORMAT_JPEG
  config.frame_size    = FRAMESIZE_QVGA;    // 320x240 — small enough for RGB565 memory needs
  config.fb_count       = 1;                 // single frame buffer, safer without JPEG compression
  config.jpeg_quality   = 12;                // unused in RGB565 mode, harmless to leave set
  config.grab_mode      = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    Serial.println("PSRAM found — using it for frame buffer.");
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.println("No PSRAM found — using internal DRAM (may limit frame size further).");
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init FAILED with error 0x%x\n", err);
    Serial.println("Check: ribbon cable fully seated, correct board selected, PSRAM setting.");
    return;
  }

  Serial.println("Camera init SUCCESS.");
}

void loop() {
  Serial.println("Capturing frame...");
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Frame capture FAILED (fb is null).");
    delay(3000);
    return;
  }

  Serial.printf("Frame captured OK — width: %d, height: %d, format: %d, length: %d bytes\n",
                fb->width, fb->height, fb->format, fb->len);

  esp_camera_fb_return(fb);
  delay(3000); // capture once every 3 seconds
}
