// ESP32-CAM Data Collection Tool - type any object name, then capture
// Board: AI-Thinker ESP32-CAM (RGB565 + software JPEG for sensors without JPEG support)
// Open the printed IP in your browser. Type a name, click Capture & Save.
// Files download as name_001.jpg, name_002.jpg, ...

#include "esp_camera.h"
#include "img_converters.h"
#include <WiFi.h>
#include <WebServer.h>

// ===== EDIT THESE =====
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
// =======================

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

WebServer server(80);

void handleRoot() {
  String html = R"HTML(
<html><body style='text-align:center;font-family:sans-serif'>
<h2>ESP32-CAM Data Collection</h2>
<img id="preview" src="/capture" style="max-width:70%"><br><br>
Object name: <input type="text" id="objName" value="cricket_ball" style="padding:8px;font-size:16px;width:180px">
&nbsp; Start number: <input type="number" id="startNum" value="1" min="1" style="padding:8px;font-size:16px;width:70px">
<br><br>
<button onclick="captureAndSave()" style="padding:12px 24px;font-size:18px;background:#4CAF50;color:white">Capture &amp; Save</button>
<p id="status"></p>
<script>
let busy = false;
function refreshPreview() {
  if (busy) return;
  document.getElementById('preview').src = '/capture?t=' + Date.now();
}
function captureAndSave() {
  let name = document.getElementById('objName').value.trim().toLowerCase().replace(/[^a-z0-9]+/g, '_');
  if (!name) { document.getElementById('status').innerText = 'Type an object name first.'; return; }
  let n = parseInt(document.getElementById('startNum').value) || 1;
  const filename = name + '_' + String(n).padStart(3, '0') + '.jpg';
  busy = true;
  fetch('/capture?t=' + Date.now())
    .then(res => res.blob())
    .then(blob => {
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      document.getElementById('status').innerText = 'Saved: ' + filename;
      document.getElementById('startNum').value = n + 1;
      busy = false;
    })
    .catch(() => { document.getElementById('status').innerText = 'Capture failed, try again.'; busy = false; });
}
setInterval(refreshPreview, 1000);
</script>
</body></html>
)HTML";
  server.send(200, "text/html", html);
}

void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }
  uint8_t *jpg_buf = NULL;
  size_t jpg_len = 0;
  bool ok = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
  esp_camera_fb_return(fb);
  if (!ok) {
    server.send(500, "text/plain", "JPEG conversion failed");
    return;
  }
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char*)jpg_buf, jpg_len);
  free(jpg_buf);
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
  config.frame_size    = FRAMESIZE_QVGA;
  config.fb_count       = 1;
  config.grab_mode      = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location    = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init FAILED with error 0x%x\n", err);
  } else {
    Serial.println("Camera init SUCCESS.");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! Open this in your browser: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed to connect.");
  }

  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.begin();
}

void loop() {
  server.handleClient();
}
