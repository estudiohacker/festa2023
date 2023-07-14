# Setting up the TFT_eSPI Library

## User_Setup.h

On line 45 comment out (i.e. add “//” in front of the text) the line defining the ILI9341 driver.
On line 65 uncomment (i.e. remove the “//”) the line defining the GC9A01 driver.
On line 207 set MOSI to 23.
On line 208 set SCLK to 18.
On line 209 set CS to 22.
On line 210 set DC to 16.
On line 211 set RST to 4.


// #define ILI9341_DRIVER       // Generic driver for common displays
#define GC9A01_DRIVER

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   22  // Chip select control pin
#define TFT_DC   16  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
#define SPI_FREQUENCY  80000000

# Reference
https://dronebotworkshop.com/gc9a01/
