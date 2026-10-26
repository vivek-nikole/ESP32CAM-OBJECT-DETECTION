// ESP32-CAM: live object detection + image collection in ONE sketch
// Board: AI-Thinker ESP32-CAM, PSRAM Enabled, Partition Scheme: Huge APP
// Open the printed IP in a browser on the same WiFi.
//  - Live image with detection boxes (switch detection on/off with the checkbox)
//  - Type an object name and click Save to download a clean photo (no boxes) as name_001.jpg
// Camera runs in RGB565 (sensor without JPEG support); frames are converted to JPEG in software.

#include <Object_Detection_LeandGren_inferencing.h>
#include "esp_camera.h"
#include "img_converters.h"
#include <WiFi.h>
#include <WebServer.h>

// ===== EDIT THESE =====
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
// =======================

// Only show detections at or above this confidence (0.0 - 1.0)
const float MIN_CONF = 0.5;

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

WebServer server(80);
static camera_fb_t *current_fb = NULL;
static bool camera_ok = false;

// Pixels for the Edge Impulse model: center 240x240 square, resized to model input size
static int get_data(size_t offset, size_t length, float *out_ptr) {
  const int crop = CAM_H;
  const int x0 = (CAM_W - crop) / 2;
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

void handleRoot() {
  String html = R"HTML(
<html><body style='text-align:center;font-family:sans-serif;background:#111;color:#eee'>
<h2>ESP32-CAM Detect + Collect</h2>
<canvas id="cv" width="320" height="240" style="width:640px;max-width:95%;border:2px solid #444"></canvas>
<h3 id="status">Starting...</h3>
<label><input type="checkbox" id="detOn" checked> Run detection</label>
<hr style="border-color:#333;width:70%">
Object name: <input type="text" id="objName" value="grenade" style="padding:8px;font-size:16px;width:160px">
&nbsp; Start number: <input type="number" id="startNum" value="1" min="1" style="padding:8px;font-size:16px;width:70px">
<br><br>
<button onclick="saveImage()" style="padding:12px 24px;font-size:18px;background:#4CAF50;color:white;border:0;border-radius:6px">Save photo</button>
<p id="saveStatus" style="color:#9f9"></p>
<p style="color:#888;font-size:13px">Tip: for background-only photos, point the camera at a scene without your object and type the name <b>background</b>. Saved photos never contain boxes.</p>
<script>
const cv = document.getElementById('cv');
const ctx = cv.getContext('2d');
const img = new Image();
let saving = false;

async function loop() {
  if (saving) { setTimeout(loop, 100); return; }
  const detect = document.getElementById('detOn').checked;
  try {
    const r = await fetch((detect ? '/detect' : '/capture') + '?t=' + Date.now());
    if (!r.ok) throw new Error('bad response');
    let det = [];
    if (detect) {
      try { det = JSON.parse(r.headers.get('X-Detections') || '[]'); } catch (e) {}
    }
    const blob = await r.blob();
    const url = URL.createObjectURL(blob);
    await new Promise(res => { img.onload = res; img.src = url; });
    ctx.drawImage(img, 0, 0, cv.width, cv.height);
    URL.revokeObjectURL(url);
    ctx.lineWidth = 2;
    ctx.font = '14px sans-serif';
    det.forEach(d => {
      ctx.strokeStyle = '#00ff66';
      ctx.strokeRect(d.x, d.y, d.w, d.h);
      const text = d.l + ' ' + Math.round(d.c * 100) + '%';
      const tw = ctx.measureText(text).width + 6;
      ctx.fillStyle = '#00ff66';
      ctx.fillRect(d.x, Math.max(0, d.y - 18), tw, 18);
      ctx.fillStyle = '#000';
      ctx.fillText(text, d.x + 3, Math.max(13, d.y - 5));
    });
    document.getElementById('status').innerText = !detect ? 'Detection off'
      : (det.length ? det.map(d => d.l + ' (' + Math.round(d.c * 100) + '%)').join(', ') : 'No object detected');
  } catch (e) {
    document.getElementById('status').innerText = 'Waiting for camera...';
    await new Promise(res => setTimeout(res, 1000));
  }
  setTimeout(loop, 50);
}

async function saveImage() {
  let name = document.getElementById('objName').value.trim().toLowerCase().replace(/[^a-z0-9]+/g, '_');
  if (!name) { document.getElementById('saveStatus').innerText = 'Type an object name first.'; return; }
  let n = parseInt(document.getElementById('startNum').value) || 1;
  const filename = name + '_' + String(n).padStart(3, '0') + '.jpg';
  saving = true;
  try {
    const r = await fetch('/capture?t=' + Date.now());
    if (!r.ok) throw new Error('bad response');
    const blob = await r.blob();
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 2000);
    document.getElementById('saveStatus').innerText = 'Saved: ' + filename;
    document.getElementById('startNum').value = n + 1;
  } catch (e) {
    document.getElementById('saveStatus').innerText = 'Save failed, try again.';
  }
  saving = false;
}
loop();
</script>
</body></html>
)HTML";
  server.send(200, "text/html", html);
}

// Plain JPEG frame, no model and no boxes (used for live view with detection off, and for saving)
void handleCapture() {
  if (!camera_ok) {
    server.send(500, "text/plain", "Camera not ready");
    return;
  }
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
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "image/jpeg", (const char *)jpg_buf, jpg_len);
  free(jpg_buf);
}

// JPEG frame plus detections (in 320x240 image coordinates) in the X-Detections header
void handleDetect() {
  if (!camera_ok) {
    server.send(500, "text/plain", "Camera not ready");
    return;
  }
  current_fb = esp_camera_fb_get();
  if (!current_fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &get_data;
  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  String json = "[";
  if (res == EI_IMPULSE_OK) {
    const float scaleX = (float)CAM_H / EI_CLASSIFIER_INPUT_WIDTH;
    const float scaleY = (float)CAM_H / EI_CLASSIFIER_INPUT_HEIGHT;
    const int x0 = (CAM_W - CAM_H) / 2;
    bool first = true;
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
      ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
      if (bb.value < MIN_CONF) continue;
      char item[160];
      snprintf(item, sizeof(item),
               "%s{\"l\":\"%s\",\"c\":%.2f,\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}",
               first ? "" : ",", bb.label, bb.value,
               (int)(x0 + bb.x * scaleX), (int)(bb.y * scaleY),
               (int)(bb.width * scaleX), (int)(bb.height * scaleY));
      json += item;
      first = false;
    }
  } else {
    Serial.printf("Classifier error: %d\n", res);
  }
  json += "]";

  uint8_t *jpg_buf = NULL;
  size_t jpg_len = 0;
  bool ok = frame2jpg(current_fb, 70, &jpg_buf, &jpg_len);
  esp_camera_fb_return(current_fb);
  current_fb = NULL;
  if (!ok) {
    server.send(500, "text/plain", "JPEG conversion failed");
    return;
  }

  server.sendHeader("X-Detections", json);
  server.sendHeader("Access-Control-Expose-Headers", "X-Detections");
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "image/jpeg", (const char *)jpg_buf, jpg_len);
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
  config.frame_size   = FRAMESIZE_QVGA;   // keep 320x240 for both collecting and detecting
  config.fb_count     = 1;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init FAILED with error 0x%x\n", err);
  } else {
    camera_ok = true;
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
    Serial.println("WiFi failed to connect. Check SSID/password and 2.4GHz.");
  }

  server.on("/", handleRoot);
  server.on("/detect", handleDetect);
  server.on("/capture", handleCapture);
  server.begin();
}

void loop() {
  server.handleClient();
}
