# Model

The trained Edge Impulse model is not stored in this repository. Train your own and install it as follows.

1. In Edge Impulse, open **Deployment**, choose **Arduino library**, pick the quantized (int8) option, and click **Build**. Download the .zip.
2. In the Arduino IDE, use **Sketch > Include Library > Add .ZIP Library** and select the file. Restart the IDE.
3. Open `firmware/detect_serial`, `detect_web` or `detect_and_collect` and change this line to match your library's header name:

```cpp
#include <Object_Detection_LeandGren_inferencing.h>
```

The header name is shown under **File > Examples** after installing the library. It ends with `_inferencing.h` and starts with your Edge Impulse project name.

If you want to share your trained model, you can place the exported .zip here and remove `model/*.zip` from `.gitignore`. Check Edge Impulse's terms first, and make sure your training photos do not contain anything private.
