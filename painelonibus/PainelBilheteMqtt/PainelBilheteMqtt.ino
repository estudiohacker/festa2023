#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include <TM1637Display.h>



#define DEBUG

const char* id = "ESP-001-Bilhete";

//Wifi
const char* ssid = "Painel";
const char* password = "QmSesc@2024";

//informações do broker MQTT - Verifique as informações geradas pelo CloudMQTT
const char* mqttServer = "10.42.0.1";   //server
const char* mqttUser = "quilombo";            //user
const char* mqttPassword = "sesc";      //password
const int mqttPort = 1883;   //porta

const char* mqttTopicSubPainel ="painel/status";
const char* mqttTopicSubPainelBilhete ="painel/bilhete";
const char* mqttTopicSubPainelDisplay7seg ="painel/display7seg";

WiFiClient espClient;
PubSubClient client(espClient);

//####### Bilhete RFID################
// RC522  - ESP32
// SDA   -  D21
// SCK   -  D18
// MOSI  -  D23
// MISO -   D19
// RST  -   D22

#define SS_PIN    21   
#define RST_PIN   22        

#include <SPI.h>
#include <MFRC522.h>


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// Modulo display 7 segmentos
#define CLK_1 32
#define DIO_1 33

#define CLK_2 25
#define DIO_2 26

const uint8_t SEG_SESC[] = {
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,           // S
  SEG_A | SEG_D | SEG_E | SEG_F| SEG_G,           // E
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,           // S
  SEG_A | SEG_D | SEG_E | SEG_F                    // c
  };


const uint8_t SEG_FESTA[] = {
  SEG_A | SEG_E | SEG_F | SEG_G,                   // F
  SEG_A | SEG_D | SEG_E | SEG_F| SEG_G,            // E
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,           // S
  SEG_D | SEG_E | SEG_F | SEG_G,                   // T
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G    // A
  };
  

TM1637Display display1(CLK_1, DIO_1);
TM1637Display display2(CLK_2, DIO_2);

/// Fita de led
#include <Adafruit_NeoPixel.h>

#define LED_PIN    2
#define LED_COUNT 160

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


String bilheteAnterior = ""; //ultimo bilhete lido
int tempoEnvioBilhete = 60000; //tempo de espera para enviar o memso bilhete em milisegundos
long ultimoEnvioBilhete = 0;
int contador = 0;

void setup() {

  //bilhete
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522


  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);
  display1.setSegments(SEG_SESC);
  display2.showNumberDec(2023, true);  // Expect: 04__

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50);
  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  #ifdef DEBUG
  Serial.println("Conectado na rede WiFi");
  #endif

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.println("Conectando ao Broker MQTT...");
    #endif
 
    if (client.connect("ESPClientBilhete", mqttUser, mqttPassword )) {
      #ifdef DEBUG
      Serial.println("Conectado");  
      #endif
      client.publish(mqttTopicSubPainel,id);
      client.subscribe(mqttTopicSubPainelDisplay7seg);
    
    } else {
      #ifdef DEBUG 
      Serial.print("falha estado  ");
      Serial.print(client.state());
      #endif
      delay(2000);
 
    }
  }
}

void displaySesc(){
  display1.setSegments(SEG_SESC);
  display2.setSegments(SEG_FESTA);
}

void lerBilhete() 
{
  // Look for new cards
  if (  mfrc522.PICC_IsNewCardPresent()) 
  {
     //Show UID on serial monitor
      #ifdef DEBUG
      Serial.println();
      Serial.print(" UID tag :");
      #endif
    
   
    String content= "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
       
       content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
       content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    
    Serial.println(content);
    Serial.println(bilheteAnterior);
    
    if(!bilheteAnterior.equals(content) || ((millis() - ultimoEnvioBilhete) > tempoEnvioBilhete)){ 
      bilheteAnterior = content;
      ultimoEnvioBilhete = millis();
      client.publish(mqttTopicSubPainelBilhete, content.c_str());
      contador++;
      display2.showNumberDec(contador, true);
      theaterChase(strip.Color(random(0, 255), random(0, 255), random(0, 255)), 50); // White, half brightness
      
    }
     #ifdef DEBUG
      Serial.println(content);
    #endif
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  
} 

void callback(char* topic, byte* payload, unsigned int length) {

  //armazena msg recebida em uma sring
  payload[length] = '\0';
  String strMSG = String((char*)payload);
  String strTopic = String((char*)topic);

  #ifdef DEBUG
  Serial.print("Mensagem chegou do tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem:");
  Serial.print(strMSG);
  Serial.println();
  Serial.println("-----------------------");
  #endif

}

//função pra reconectar ao servido MQTT
void reconect() {
  //Enquanto estiver desconectado
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Tentando conectar ao servidor MQTT");
    #endif
     
    bool conectado = strlen(mqttUser) > 0 ?
                     client.connect("ESP8266Client", mqttUser, mqttPassword) :
                     client.connect("ESP8266Client");

    if(conectado) {
      #ifdef DEBUG
      Serial.println("Conectado!");
      #endif
      //subscreve no tópico
      client.subscribe(mqttTopicSubPainel, 1); //nivel de qualidade: QoS 1
      
      
    } else {
      #ifdef DEBUG
      Serial.println("Falha durante a conexão.Code: ");
      Serial.println( String(client.state()).c_str());
      Serial.println("Tentando novamente em 10 s");
      #endif
      //Aguarda 10 segundos 
      delay(10000);
    }
  }
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconect();
  }
  client.loop();
  lerBilhete();

  /* Serial.println(statusEntrada,BIN);
  statusEntradaAtual = 0;
  for(byte i=0;i<qtdeEntradas;i++)
    bitWrite(statusEntradaAtual,i, digitalRead(entradas[i]));

  if( statusEntradaAtual != statusEntrada){
       Serial.print("Anterior:");
       Serial.println(statusEntrada,BIN);
       Serial.print("Atual:");
       Serial.println(statusEntradaAtual,BIN);
       statusEntrada = statusEntradaAtual;
       Serial.print("Enviar:");
       Serial.println(statusEntrada);
       char strEntrada[5] ;
        itoa(statusEntrada,strEntrada,10);
       client.publish(mqttTopicSubPainelBilhete, strEntrada);
  }
  delay(200); */
}


// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
