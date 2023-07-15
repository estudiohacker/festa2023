#include <SPIFFS.h>
#include <Arduino_GFX_Library.h>
#include <PNGdec.h>
#include "GifClass.h"

#define GFX_BUTTON_1 16
#define GFX_BUTTON_2 13 

#define PNG_FILENAME "/laroye.png"
const char *gifFiles[] = {"/laroye.gif", "/exu2.gif", "/ezgif.com-optimize.gif"};
const int8_t maxGifFiles = 3;
int8_t currentGifFileIdx = 0;

const char *pngFiles[] = {"/laroye.png", "/bluemarble.png"};
const int8_t maxPngFiles = 2;
int8_t currentPngFileIdx = 0;


Arduino_DataBus *busVSPI = new Arduino_ESP32SPI(21 /* DC */,  5 /* CS */, 18 /* SCK */, 23 /* MOSI */, GFX_NOT_DEFINED /* MISO */, VSPI /* spi_num */);
Arduino_DataBus *busHSPI = new Arduino_ESP32SPI(25 /* DC */, 33 /* CS */, 14 /* SCK */, 27 /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx =    new Arduino_GC9A01(busVSPI, 22 /* RST */, 0 /* rotation */, true /* IPS */);
Arduino_GFX *gfxPNG = new Arduino_GC9A01(busHSPI, 26 /* RST */, 0 /* rotation */, true /* IPS */);

PNG png;
File pngFile;
static GifClass gifClass;

int16_t xOffsetPNG, yOffsetPNG;

bool GFXButton1Pressed = false;
bool GFXButton2Pressed = false;


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
  gfxPNG->draw16bitRGBBitmap(xOffsetPNG, yOffsetPNG + pDraw->y, usPixels, usMask, pDraw->iWidth, 1);
}

void showPNG(const char *path) {
  int rc;
  rc = png.open(path, myOpen, myClose, myRead, mySeek, PNGDraw);
  if (rc == PNG_SUCCESS) {
    int16_t pw = png.getWidth();
    int16_t ph = png.getHeight();

    xOffsetPNG = (gfxPNG->width() - pw) / 2;
    yOffsetPNG = (gfxPNG->height() - ph) / 2;

    rc = png.decode(NULL, 0);
    png.close();
  } else {
    Serial.println("ERROR: png.open() failed!");
  }
}

void handleGFXButton1(void * parameter) {
  for(;;) {
    if (digitalRead(GFX_BUTTON_1)) {
      Serial.println("GFX_BUTTON_1 PRESSED");
      GFXButton1Pressed = true;
      currentGifFileIdx += 1;
      if (currentGifFileIdx >= maxGifFiles) {
        currentGifFileIdx = 0;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }    
  }
}

void handleGFXButton2(void * parameter) {
  for(;;) {
    if (digitalRead(GFX_BUTTON_2)) {
      Serial.println("GFX_BUTTON_2 PRESSED");
      GFXButton2Pressed = true;
      currentPngFileIdx += 1;
      if (currentPngFileIdx >= maxPngFiles) {
        currentPngFileIdx = 0;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }    
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(GFX_BUTTON_1, INPUT_PULLDOWN);
  pinMode(GFX_BUTTON_2, INPUT_PULLDOWN);

  xTaskCreate(handleGFXButton1, "handleGFXButton1", 1000, NULL, 1, NULL);
  xTaskCreate(handleGFXButton2, "handleGFXButton2", 1000, NULL, 1, NULL);

  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed!");
  }
  gfx->fillScreen(WHITE);

  if (!gfxPNG->begin()) {
    Serial.println("ERROR: gfxPNG->begin() failed!");
  }
  gfxPNG->fillScreen(WHITE);

  if (!SPIFFS.begin()) {
    Serial.println(F("ERROR: file system mount failed!"));
  }

  showPNG(pngFiles[currentPngFileIdx]);
}

void loop() {
  File gifFile = SPIFFS.open(gifFiles[currentGifFileIdx], "r");
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
        while (res > 0 && !GFXButton1Pressed) {
          if (GFXButton2Pressed) {
            showPNG(pngFiles[currentPngFileIdx]);
            GFXButton2Pressed = false;
          }
          
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
        GFXButton1Pressed = false;
        gifClass.gd_close_gif(gif);
        free(buf);
      }
    }
  }
}
