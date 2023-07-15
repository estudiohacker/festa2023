#include <SPIFFS.h>
#include <Arduino_GFX_Library.h>
#include <PNGdec.h>
#include "GifClass.h"

#define PNG_FILENAME "/laroye.png"
#define GIF_FILENAME "/laroye.gif"

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
static GifClass gifClass;

int16_t w, h, xOffset, yOffset;

void *myOpen(const char *filename, int32_t *size) {
  pngFile = SPIFFS.open(filename, "r");
  if (!pngFile || pngFile.isDirectory()) {
    Serial.println(F("ERROR: Failed to open " PNG_FILENAME " file for reading"));
  } else {
    *size = pngFile.size();
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

void showPNG() {
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

void playGif() {
  File gifFile = SPIFFS.open(GIF_FILENAME, "r");
  if (!gifFile || gifFile.isDirectory()) {
    Serial.println(F("ERROR: open gifFile Failed!"));
  } else {
    // read GIF file header
    gd_GIF *gif = gifClass.gd_open_gif(&gifFile);
    if (!gif) {
      Serial.println(F("gd_open_gif() failed!"));
    } else {
      uint8_t *buf = (uint8_t *)malloc(gif->width * gif->height);
      if (!buf) {
        Serial.println(F("buf malloc failed!"));
      } else {
        int16_t x = (gfx->width() - gif->width) / 2;
        int16_t y = (gfx->height() - gif->height) / 2;

        int32_t t_fstart, t_delay = 0, t_real_delay, delay_until;
        int32_t res = 1;
        int32_t duration = 0, remain = 0;
        while (res > 0) {
          t_fstart = millis();
          t_delay = gif->gce.delay * 10;
          res = gifClass.gd_get_frame(gif, buf);
          if (res < 0) {
            break;
          } else if (res > 0) {
            gfx->drawIndexedBitmap(x, y, buf, gif->palette->colors, gif->width, gif->height);

            t_real_delay = t_delay - (millis() - t_fstart);
            duration += t_delay;
            remain += t_real_delay;
            delay_until = millis() + t_real_delay;
            while (millis() < delay_until) {
              delay(1);
            }
          }
        }
        gifClass.gd_close_gif(gif);
        free(buf);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(WHITE);
  w = gfx->width(), h = gfx->height();

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  if (!SPIFFS.begin()) {
    Serial.println(F("ERROR: file system mount failed!"));
  } else {
    showPNG();
    delay(5000);       
  }
}

void loop() {
  playGif();
}
