# Troubleshooting

| Problem | Likely cause | Fix |
|---|---|---|
| No COM port in the Arduino IDE | USB-serial driver missing, or a charge-only cable | Install the CH340 driver, try another cable, replug the board |
| Serial Monitor is blank after upload | Board not reset, wrong baud rate, or wrong port | Press reset or replug USB, use 115200 baud, select the same port used for upload |
| `Camera init failed with error 0x106` / `JPEG format is not supported on this sensor` | The sensor has no hardware JPEG | Use RGB565 at QVGA with a single frame buffer (as in these sketches) |
| `Camera init failed` after a change | Frame size too large for RGB565, PSRAM disabled, or loose ribbon cable | Step back to QVGA, enable PSRAM, reseat the camera ribbon cable |
| Edge Impulse example prints `ERR: Camera is not initialized` | The stock example selects another board's pins and asks for JPEG | Use the `detect_*` sketches in this repo |
| WiFi never connects | 5 GHz-only network, wrong SSID or password, weak signal | Use a 2.4 GHz network, re-check credentials, move closer to the router |
| Upload says port busy | Another Serial Monitor or IDE window holds the port | Close other monitors and extra IDE windows, replug USB |
| Very slow first compile | Large Edge Impulse library | Wait for the first build; later builds are faster |
| Compile error about the library header | The `#include` name does not match your library | Check the name under *File > Examples* and update the include line |
| F1 score of 0 | Too few labeled images, or images without boxes | Add more images and check that every object image has a tight box |
| Detections on empty scenes | No background-only photos, little variety | Add background photos with no boxes, add varied object photos, retrain |
| Web sketch will not load or runs out of memory | WiFi, model and camera together use most memory | Use `detect_serial`, or rebuild the model library with the quantized (int8) option |
