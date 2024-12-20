#include <WiFi.h>
#include <basics.h>
#include <jarolift.h>
#include <language.h>
#include <message.h>
#include <mqtt.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
#define PAYLOAD_LEN 512
#define MAX_QUEUE_SIZE 10

struct s_MqttMessage {
  char topic[512];
  char payload[PAYLOAD_LEN];
  int len;
  long intVal = 0;
  float floatVal = 0.0;
};

static void processMqttMessage();
static AsyncMqttClient mqtt_client;
static bool bootUpMsgDone, setupDone = false;
static const char *TAG = "MQTT"; // LOG TAG
static bool mqttMsgAvailable = false;
static char lastError[64] = "---";
static int mqtt_retry = 0;
static muTimer mqttReconnectTimer;
static s_MqttMessage msgCpy;
static s_MqttMessage messageQueue[MAX_QUEUE_SIZE];
static int queueHead = 0;
static int queueTail = 0;

/**
 * *******************************************************************
 * @brief   mqtt publish wrapper
 * @param   topic, payload, retained
 * @return  none
 * *******************************************************************/
void mqttPublish(const char *topic, const char *payload, boolean retained) { mqtt_client.publish(topic, 0, retained, payload); }

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char *addTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s%s", config.mqtt.topic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char *addCfgCmdTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s/setvalue/%s", config.mqtt.topic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

  msgCpy.len = len;

  if (topic == NULL) {
    msgCpy.topic[0] = '\0';
  } else {
    strncpy(msgCpy.topic, topic, sizeof(msgCpy.topic) - 1);
    msgCpy.topic[sizeof(msgCpy.topic) - 1] = '\0';
  }
  if (payload == NULL) {
    msgCpy.payload[0] = '\0';
  } else if (len > 0 && len < PAYLOAD_LEN) {
    memcpy(msgCpy.payload, payload, len);
    msgCpy.payload[len] = '\0';
  }

  // payload as number
  msgCpy.intVal = 0;
  msgCpy.floatVal = 0.0;
  if (len > 0) {
    msgCpy.intVal = atoi(msgCpy.payload);
    msgCpy.floatVal = atoff(msgCpy.payload);
  }

  mqttMsgAvailable = true;

  MY_LOGI(TAG, "msg received | topic: %s | payload: %s", msgCpy.topic, msgCpy.payload);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets connected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttConnect(bool sessionPresent) {
  mqtt_retry = 0;
  MY_LOGI(TAG, "MQTT connected");
  // Once connected, publish an announcement...
  sendWiFiInfo();
  // ... and resubscribe
  mqtt_client.subscribe(addTopic("/cmd/#"), 0);
  mqtt_client.subscribe(addTopic("/setvalue/#"), 0);
  mqtt_client.subscribe("homeassistant/status", 0);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets disconnected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

  switch (reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
    snprintf(lastError, sizeof(lastError), "TCP DISCONNECTED");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
    snprintf(lastError, sizeof(lastError), "MQTT UNACCEPTABLE PROTOCOL VERSION");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
    snprintf(lastError, sizeof(lastError), "MQTT IDENTIFIER REJECTED");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
    snprintf(lastError, sizeof(lastError), "MQTT SERVER UNAVAILABLE");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
    snprintf(lastError, sizeof(lastError), "MQTT MALFORMED CREDENTIALS");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
    snprintf(lastError, sizeof(lastError), "MQTT NOT AUTHORIZED");
    break;
  case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
    snprintf(lastError, sizeof(lastError), "TLS BAD FINGERPRINT");
    break;
  default:
    snprintf(lastError, sizeof(lastError), "UNKNOWN ERROR");
    break;
  }
}

/**
 * *******************************************************************
 * @brief   is MQTT connected
 * @param   none
 * @return  none
 * *******************************************************************/
bool mqttIsConnected() { return mqtt_client.connected(); }

const char *mqttGetLastError() { return lastError; }

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttSetup() {

  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);
  mqtt_client.setServer(config.mqtt.server, config.mqtt.port);
  mqtt_client.setClientId(config.wifi.hostname);
  mqtt_client.setCredentials(config.mqtt.user, config.mqtt.password);
  mqtt_client.setWill(addTopic("/status"), 0, true, "offline");
  mqtt_client.setKeepAlive(10);
  mqtt_client.connected();

  MY_LOGI(TAG, "MQTT setup done!");
}

/**
 * *******************************************************************
 * @brief   MQTT cyclic function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttCyclic() {

  // process incoming messages
  if (mqttMsgAvailable) {
    processMqttMessage();
    mqttMsgAvailable = false;
  }

  // call setup when connection is established
  if (config.mqtt.enable && !setupMode && !setupDone && (eth.connected || wifi.connected)) {
    mqttSetup();
    setupDone = true;
  }

  // automatic reconnect to mqtt broker if connection is lost - try 5 times, then reboot
  if (!mqtt_client.connected() && (wifi.connected || eth.connected)) {
    if (mqtt_retry == 0) {
      mqtt_retry++;
      mqtt_client.connect();
      MY_LOGI(TAG, "MQTT - connection attempt: 1/5");
    } else if (mqttReconnectTimer.delayOnTrigger(true, MQTT_RECONNECT)) {
      mqttReconnectTimer.delayReset();
      if (mqtt_retry < 5) {
        mqtt_retry++;
        mqtt_client.connect();
        MY_LOGI(TAG, "MQTT - connection attempt: %i/5", mqtt_retry);
      } else {
        MY_LOGI(TAG, "MQTT connection not possible, esp rebooting...");
        saveRestartReason("no mqtt connection");
        yield();
        delay(1000);
        yield();
        ESP.restart();
      }
    }
  }

  // send bootup messages after restart and established mqtt connection
  if (!bootUpMsgDone && mqtt_client.connected()) {
    bootUpMsgDone = true;
    char restartReason[64];
    getRestartReason(restartReason, sizeof(restartReason));
    MY_LOGI(TAG, "ESP restarted (%s)", restartReason);

    if (config.mqtt.ha_enable) {
      mqttDiscoverySetup(false);
    }
  }
}

int checkShutterCmd(const char *topicCopy, const char *cmpTopic) {

  size_t cmpTopicLen = strlen(cmpTopic);

  if (strncmp(topicCopy, cmpTopic, cmpTopicLen) == 0) {
    const char *suffix = topicCopy + cmpTopicLen;
    char *endPtr;
    int channel = strtol(suffix, &endPtr, 10);

    if (*endPtr == '\0' && channel >= 1 && channel <= 15) {
      return channel;
    }
  }
  return -1;
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void processMqttMessage() {

  const char *shutterTopic = addTopic("/cmd/shutter/");
  int channel = checkShutterCmd(msgCpy.topic, shutterTopic);

  // restart ESP command
  if (strcasecmp(msgCpy.topic, addTopic("/cmd/restart")) == 0) {
    saveRestartReason("mqtt command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
    // reconfigure
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/reconfigure")) == 0) {
    mqttDiscoverySetup(true);
    yield();
    delay(1000);
    yield();
    mqttDiscoverySetup(false);
    // homeassistant/status
  } else if (strcmp(msgCpy.topic, "homeassistant/status") == 0) {
    if (config.mqtt.ha_enable && strcmp(msgCpy.payload, "online") == 0) {
      mqttDiscoverySetup(false); // send actual discovery configuration
    }
    // Shutter commands
  } else if (channel != -1) {
    if (channel >= 1 && channel <= 16) {
      if (strcasecmp(msgCpy.payload, "UP") == 0 || strcasecmp(msgCpy.payload, "OPEN") == 0 || strcmp(msgCpy.payload, "0") == 0) {
        jaroCmdUp(channel - 1);
      } else if (strcasecmp(msgCpy.payload, "DOWN") == 0 || strcasecmp(msgCpy.payload, "CLOSE") == 0 || strcmp(msgCpy.payload, "1") == 0) {
        jaroCmdDown(channel - 1);
      } else if (strcasecmp(msgCpy.payload, "STOP") == 0 || strcmp(msgCpy.payload, "2") == 0) {
        jaroCmdStop(channel - 1);
      } else if (strcasecmp(msgCpy.payload, "SHADE") == 0 || strcmp(msgCpy.payload, "3") == 0) {
        jaroCmdShade(channel - 1);
      } else if (strcasecmp(msgCpy.payload, "SETSHADE") == 0 || strcmp(msgCpy.payload, "4") == 0) {
        jaroCmdSetShade(channel - 1);
      } else {
        mqttPublish(addTopic("/message"), "invalid shutter cmd", false);
        MY_LOGW(TAG, "invalid shutter cmd");
      }
    } else {
      mqttPublish(addTopic("/message"), "invalid channel", false);
      MY_LOGW(TAG, "invalid channel for shutter cmd");
    }
  } else {
    mqttPublish(addTopic("/message"), "unknown topic", false);
    MY_LOGI(TAG, "unknown topic received");
  }
}

/**
 * *******************************************************************
 * @brief   Fügt eine Nachricht zur Queue hinzu
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void addToQueue(const char* topic, const char* payload) {
    int nextHead = (queueHead + 1) % MAX_QUEUE_SIZE;
    
    if (nextHead != queueTail) {  // Queue ist nicht voll
        strncpy(messageQueue[queueHead].topic, topic, sizeof(messageQueue[queueHead].topic) - 1);
        strncpy(messageQueue[queueHead].payload, payload, sizeof(messageQueue[queueHead].payload) - 1);
        // Null-Terminierung sicherstellen
        messageQueue[queueHead].topic[sizeof(messageQueue[queueHead].topic) - 1] = '\0';
        messageQueue[queueHead].payload[sizeof(messageQueue[queueHead].payload) - 1] = '\0';
        queueHead = nextHead;
    }
}

/**
 * *******************************************************************
 * @brief   Verarbeitet die Queue
 * @param   none
 * @return  none
 * *******************************************************************/
void processMessageQueue() {
    while (queueTail != queueHead) {
        // Nachricht kopieren und verarbeiten
        strncpy(msgCpy.topic, messageQueue[queueTail].topic, sizeof(msgCpy.topic) - 1);
        strncpy(msgCpy.payload, messageQueue[queueTail].payload, sizeof(msgCpy.payload) - 1);
        // Null-Terminierung sicherstellen
        msgCpy.topic[sizeof(msgCpy.topic) - 1] = '\0';
        msgCpy.payload[sizeof(msgCpy.payload) - 1] = '\0';
        
        processMqttMessage();
        
        // Kurze Pause zwischen den Befehlen
        delay(50);
        
        queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char payloadStr[128];
    // Payload in String umwandeln
    strlcpy(payloadStr, (char*)payload, min(length + 1, sizeof(payloadStr)));
    
    // Zur Queue hinzufügen statt direkter Verarbeitung
    addToQueue(topic, payloadStr);
}
