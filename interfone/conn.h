#pragma once

#include <Arduino.h>
#include "src/PubSubClient/PubSubClient.h"

class Conn {
  public:
    Conn();
    void loop();
    
    void notify_sensor(String sensor, uint8_t value);
    
    void notify_sensor(String sensor, String value);

    void notify_topic(String topic, String payload, bool retained = false);
  private:
    String _id;
    String _hostname;
    PubSubClient* _PubSubClient;
    void connect();
    String fullTopic(String topic);
};
