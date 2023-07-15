#include <Adafruit_NeoPixel.h>

#define LED_PIN    5
#define LED_COUNT 300

#define BUTTON1 32
#define BUTTON2 33
#define BUTTON3 25
#define BUTTON4 26

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t buttonNum = 3;
bool buttonPressed = false;

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    if (buttonPressed) return;
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for (int a = 0; a < 10; a++) { // Repeat 10 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      if (buttonPressed) return;
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    strip.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    strip.show(); // Update strip with new contents
    if (buttonPressed) return;
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++) { // Repeat 30 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      if (buttonPressed) return;
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

void handleButtonPressed(void * parameter) {
  for(;;) {
    if (digitalRead(BUTTON1)) {
      Serial.println("BUTTON1 PRESSED");
      buttonNum = 1;
      buttonPressed = true;
      vTaskDelay(pdMS_TO_TICKS(500));
    } else if (digitalRead(BUTTON2)) {
      Serial.println("BUTTON2 PRESSED");
      buttonNum = 2;
      buttonPressed = true;
      vTaskDelay(pdMS_TO_TICKS(500));
    } else if (digitalRead(BUTTON3)) {
      Serial.println("BUTTON3 PRESSED");
      buttonNum = 3;
      buttonPressed = true;
      vTaskDelay(pdMS_TO_TICKS(500));
    } else if (digitalRead(BUTTON4)) {
      Serial.println("BUTTON4 PRESSED");
      buttonNum = 4;
      buttonPressed = true;
      vTaskDelay(pdMS_TO_TICKS(500));
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }    
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON1, INPUT_PULLDOWN);
  pinMode(BUTTON2, INPUT_PULLDOWN);
  pinMode(BUTTON3, INPUT_PULLDOWN);
  pinMode(BUTTON4, INPUT_PULLDOWN);

  xTaskCreate(handleButtonPressed, "handleButtonPressed", 1000, NULL, 1, NULL);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(10); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void loop() {
  buttonPressed = false;

  Serial.printf("%d %d %d %d %d\n", buttonNum, digitalRead(BUTTON1), digitalRead(BUTTON2), digitalRead(BUTTON3), digitalRead(BUTTON4));
  if (buttonNum == 1) {
    // Fill along the length of the strip in various colors...
    strip.clear();
    colorWipe(strip.Color(255,   0,   0), 10); // Red
    colorWipe(strip.Color(  0, 255,   0), 10); // Green
    colorWipe(strip.Color(  0,   0, 255), 10); // Blue
  }

  if (buttonNum == 2) {
    // Do a theater marquee effect in various colors...
    strip.clear();
    theaterChase(strip.Color(127, 127, 127), 40); // White, half brightness
    theaterChase(strip.Color(127,   0,   0), 40); // Red, half brightness
    theaterChase(strip.Color(  0,   0, 127), 40); // Blue, half brightness
  }

  if (buttonNum == 3) {
    strip.clear();
    rainbow(10);             // Flowing rainbow cycle along the whole strip
  }

  if (buttonNum == 4) {
    strip.clear();
    theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant
  }
}
