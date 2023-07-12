#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#define DEBUG

const char* id = "ESP-001-Chaves";

//Wifi
const char* ssid = "Painel";
const char* password = "QmSesc@2024";

//informações do broker MQTT - Verifique as informações geradas pelo CloudMQTT
const char* mqttServer = "10.42.0.1";   //server
const char* mqttUser = "quilombo";            //user
const char* mqttPassword = "sesc";      //password
const int mqttPort = 1883;   //porta

const char* mqttTopicSubPainel ="painel/status";
const char* mqttTopicSubPainelChaves ="painel/chaves";

//Pinos de entrada para conectar as chaves
const byte qtdeEntradas = 15;
const byte entradas[] = {4,13,14,16,17,18,19,21,22,23,25,26,27,32,33};
short int statusEntrada = 0;
short int statusEntradaAtual = 0;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  for(byte i=0;i<qtdeEntradas;i++)
    pinMode(entradas[i], INPUT_PULLUP);
  
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
 
    if (client.connect("ESPClientChaves", mqttUser, mqttPassword )) {
      #ifdef DEBUG
      Serial.println("Conectado");  
      #endif
      client.publish(mqttTopicSubPainel,id);
    
    } else {
      #ifdef DEBUG 
      Serial.print("falha estado  ");
      Serial.print(client.state());
      #endif
      delay(2000);
 
    }
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

  Serial.println(statusEntrada,BIN);
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
       client.publish(mqttTopicSubPainelChaves, strEntrada);
  }
  delay(200);
}
