#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <config.h>
#include <language.h>

struct MqttMessage {
  String topic;
  String payload;
};

/* P R O T O T Y P E S ********************************************************/
const char *addTopic(const char *suffix);
void mqttSetup();
void mqttCyclic();
void checkMqtt();
void mqttPublish(const char *sendtopic, const char *payload, boolean retained);
const char *mqttGetLastError();
bool mqttIsConnected();

void mqtt_send_percent_closed_state(int channelNum, int percent, String command);

void addToQueue(const char* topic, const char* payload);
void processMessageQueue();
