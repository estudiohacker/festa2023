#include "settings.h"

/*
  To upload through terminal:
  curl -F "image=@interfone.ino.doitESP32devkitV1.bin" http://192.168.4.1/firmware
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <EEPROM.h>
#include "version.h"

WebServer server(80);

bool canHandleClient = false;

void handleClient(void * parameter) {
  for(;;) {
    canHandleClient = true;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

char* upTime() {
  unsigned long secsUp = millis() / 1000;
  unsigned short int Second = secsUp%60;
  unsigned short int Minute = (secsUp/60)%60;
  unsigned short int Hour = (secsUp/(60*60))%24;
  unsigned long Day = (secsUp/(60*60*24));

  static char cUpTime[100];
  sprintf(cUpTime, "Uptime: %ld days %02d:%02d:%02d\n", Day, Hour, Minute, Second);
  return cUpTime;
}

void handleRoot() {
  String message = String(HEADER) + String(version_info()) + String(upTime());
  Serial.println("WebServer: handleRoot");
  server.send(200, "text/plain", message);
}

void handleNotFound(){
  Serial.println("WebServer: handleNotFound");
  server.send(404, "text/plain", "404: Not found");
}

void setupOTA() {
  Serial.print(F("Starting WebServer..."));
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  server.on("/restart", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    Serial.println("WebServer: Restarting...");
    server.send(200, "text/plain", "Restarting...");
    ESP.restart();
  });
  
  server.on("/firmware", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    Serial.println("WebServer: Firmware...");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();

  xTaskCreate(
    handleClient,    // Function that should be called
    "handleClient",   // Name of the task (for debugging)
    1000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
    
  Serial.println(F("Ok!"));
}

void loopOTA() {
  if (canHandleClient) {
    canHandleClient = false;
    server.handleClient();
  }
}
