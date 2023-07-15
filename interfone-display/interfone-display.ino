#include <SPIFFS.h>
#include <Arduino_GFX_Library.h>
#include <PNGdec.h>

#define PNG_FILENAME "/laroye.png"

/*******************************************************************************
   Arduino_GFX try to find the settings depends on selected board in Arduino IDE
   Or you can define the display dev kit not in the board list
   ESP32 various dev board     : CS:  5, DC: 27, RST: 33, BL: 22, SCK: 18, MOSI: 23, MISO: nil
 ******************************************************************************/
#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_GC9A01(bus, DF_GFX_RST, 0 /* rotation */, true /* IPS */);

PNG png;
File pngFile;

int16_t w, h, xOffset, yOffset;

void *myOpen(const char *filename, int32_t *size) {
  pngFile = SPIFFS.open(filename, "r");
  if (!pngFile || pngFile.isDirectory()) {
    Serial.println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
    gfx->println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
  } else {
    *size = pngFile.size();
    //    Serial.printf("Opened '%s', size: %d\n", filename, *size);
  }
  return &pngFile;
}

void myClose(void *handle) {
  if (pngFile) {
    pngFile.close();
  }
}

int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!pngFile) {
    return 0;
  }
  return pngFile.read(buffer, length);
}

int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!pngFile) {
    return 0;
  }
  return pngFile.seek(position);
}

// Function to draw pixels to the display
void PNGDraw(PNGDRAW *pDraw) {
  uint16_t usPixels[320];
  uint8_t usMask[320];
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  png.getAlphaMask(pDraw, usMask, 1);
  gfx->draw16bitRGBBitmap(xOffset, yOffset + pDraw->y, usPixels, usMask, pDraw->iWidth, 1);
}

void setup() {
  Serial.begin(115200);

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(WHITE);
  w = gfx->width(), h = gfx->height();

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  if (!SPIFFS.begin()) {
    Serial.println(F("ERROR: file system mount failed!"));
  } else {
    unsigned long start = millis();
    int rc;
    rc = png.open(PNG_FILENAME, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
      int16_t pw = png.getWidth();
      int16_t ph = png.getHeight();

      xOffset = (w - pw) / 2;
      yOffset = (h - ph) / 2;

      rc = png.decode(NULL, 0);
      png.close();
    } else {
      Serial.println("ERROR: png.open() failed!");
    }
    Serial.printf("Time used: %lu\n", millis() - start);
  }
}

void loop() {
  //
}
