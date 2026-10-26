# Workflow

## 1. Board setup

1. Install the Arduino IDE and the ESP32 board package.
2. On Windows, install the CH340 driver so the baseboard shows up as a COM port.
3. Select **AI Thinker ESP32-CAM**, PSRAM *Enabled*, Partition Scheme *Huge APP*.
4. Upload `firmware/camera_test` and confirm `Camera init SUCCESS` in the Serial Monitor (115200 baud).

## 2. Camera mode

The tested sensor cannot output JPEG, so the camera runs in RGB565 at QVGA (320 x 240) with one frame buffer in PSRAM. Frames are converted to JPEG in software only when an image is shown or saved.

## 3. Photo collection

Use `firmware/data_collection`. Tips:

- Type the object name; files download as `name_001.jpg`, `name_002.jpg`, and so on. Set **Start number** past your last file to avoid repeats.
- Keep the camera at the same distance and focus you will use for detection.
- Vary angle, background, lighting and position in the frame. The object should fill about a quarter to a half of the frame.
- Take background-only photos (name them `background`) in the same places and lighting.
- Delete blurry shots and ones where the object is cut off.

## 4. Edge Impulse

1. Create an **object detection** project.
2. **Data acquisition > Add data > Upload data**: upload one object at a time with *Automatically split between training and testing* and the same label every time.
3. **Labeling queue**: draw a tight box around each object. Leave background-only photos without boxes.
4. **Impulse design > Create impulse**: image width and height 96, an *Image* processing block and an *Object Detection* learning block.
5. **Image**: choose grayscale or RGB, click *Save parameters*, then *Generate features*.
6. **Object detection**: start with about 30 training cycles, then try 80 to 100 as the dataset grows. Start training.
7. **Model testing > Classify all**: check the F1 score on images the model has not seen.
8. Re-run steps 5 to 7 after adding data.

## 5. Deployment

1. **Deployment > Arduino library**, quantized (int8), Build, download the .zip.
2. Install it in the Arduino IDE and update the `#include` line in the `detect_*` sketches (see `model/README.md`).
3. Upload `detect_serial` for Serial output, `detect_web` for the browser page with boxes, or `detect_and_collect` for both detection and photo capture.

How the sketches feed the model: each RGB565 frame is center-cropped to a 240 x 240 square (matching the "fit shortest axis" resize used in training), resized to the model input size, and passed to the classifier as packed 24-bit RGB values.

## 6. Reading the results

- **Precision** = TP / (TP + FP): how many reported detections were correct
- **Recall** = TP / (TP + FN): how many real objects were found
- **F1** = 2PR / (P + R): one number combining both

Accuracy alone can mislead: a model that predicts background everywhere can score above 99 % when the object covers only a tiny part of the image.
