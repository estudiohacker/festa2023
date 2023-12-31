/*
 * Autores
 *  João Adriano Freitas <jaafreitas@gmail.com>
 *  Allan Moreira <allan.moreira@gmail.com>
 *
 * No algorítmo atual, estamos verificando a posição dos interruptores um a um.
 * A forma mais eficiente seria para cada posição possível, fazer a leitura de todos os interruptores,
 * porém isso exigiria guardarmos os valores para posterior visualização.
 *
 */

#include "settings.h"
#include "version.h"
#include "conn.h"

const unsigned short int num_interruptores = 12;
const unsigned short int interruptores[num_interruptores] = {13, 18, 14, 22, 34, 35, 32, 33, 25, 26, 27, 23};
unsigned short int interruptores_estado[num_interruptores] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const unsigned short int num_posicoes = 4;
const unsigned short int posicoes[num_posicoes] = {15, 2, 4, 16};

unsigned long previousMillis = 0;
const unsigned long intervalo = 250;

Conn* conn;

void setup() {
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, HIGH);

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print(HEADER);
  Serial.println(version_info());

  conn = new Conn();

  for (uint8_t i = 0; i < num_interruptores; i++) {
    pinMode(interruptores[i], INPUT_PULLUP);
  }

  for (uint8_t i = 0; i < num_posicoes; i++) {
    pinMode(posicoes[i], OUTPUT);
  }

  Serial.println(F("Monitoring..."));
  digitalWrite(LED_STATUS_PIN, LOW);  
}

bool test_posicao_chave(uint8_t pino, bool valor1, bool valor2, bool valor3, bool valor4) {
  digitalWrite(posicoes[0], valor1);
  digitalWrite(posicoes[1], valor2);
  digitalWrite(posicoes[2], valor3);
  digitalWrite(posicoes[3], valor4);
  return digitalRead(pino) == 0;
}

uint8_t posicao_chave(uint8_t pino) {
  int posicao;
  if (test_posicao_chave(pino, LOW, HIGH, HIGH, HIGH)) {
    posicao = 1;
  }
  else if (test_posicao_chave(pino, HIGH, LOW, HIGH, HIGH)) {
    posicao = 2;
  }
  else if (test_posicao_chave(pino, HIGH, HIGH, LOW, HIGH)) {
    posicao = 4;
  }
  else if (test_posicao_chave(pino, HIGH, HIGH, HIGH, LOW)) {
    posicao = 5;
  }
  else {
    posicao = 3;
  }
  return posicao;
}

void loop() {
  conn->loop();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalo) {
    previousMillis = currentMillis;

    for (uint8_t i = 0; i < num_interruptores; i++) {
      if (posicao_chave(interruptores[i]) != interruptores_estado[i]) {
        if (interruptores_estado[i] != 0) {
          conn->notify_sensor("chave/" + String(i+1), posicao_chave(interruptores[i]));
        };
        interruptores_estado[i] = posicao_chave(interruptores[i]);
      };
    }
  }
}
