#include <JaroliftController.h>
#include <config.h>
#include <mqtt.h>

static const char *TAG = "JARO"; // LOG TAG

JaroliftController jarolift;

void msgCallback(esp_log_level_t level, const char *msg) {
  switch (level) {
  case ESP_LOG_ERROR:
    MY_LOGE(TAG, "%s", msg);
    break;
  case ESP_LOG_WARN:
    MY_LOGW(TAG, "%s", msg);
    break;
  case ESP_LOG_INFO:
    MY_LOGI(TAG, "%s", msg);
    break;
  case ESP_LOG_DEBUG:
    MY_LOGD(TAG, "%s", msg);
    break;
  default:
    MY_LOGI(TAG, "[UNKNOWN]: %s", msg);
    break;
  }
}

void jaroliftSetup() {

  // initialize
  MY_LOGI(TAG, "initializing the CC1101 Transceiver");
  jarolift.setMessageCallback(msgCallback);
  jarolift.setGPIO(config.gpio.sck, config.gpio.miso, config.gpio.mosi, config.gpio.cs, config.gpio.gdo0, config.gpio.gdo2);
  jarolift.setKeys(config.jaro.masterMSB, config.jaro.masterLSB);
  jarolift.setBaseSerial(config.jaro.serial);
  jarolift.begin();

  if (jarolift.getCC1101State()) {
    MY_LOGI(TAG, "CC1101 Transceiver connected!");
  } else {
    MY_LOGE(TAG, "CC1101 Transceiver NOT connected!");
  }

  MY_LOGI(TAG, "read Device Counter from EEPROM: %i", jarolift.getDeviceCounter());
}

void mqttSendPosition(uint8_t channel, uint8_t position) {
  char topic[64];
  char pos[16];

  if (position > 100)
    position = 100;
  if (position < 0)
    position = 0;
  if (mqttIsConnected()) {
    itoa(position, pos, 10);
    snprintf(topic, sizeof(topic), "%s%d", addTopic("/status/shutter/"), channel + 1);
    mqttPublish(topic, pos, true);
  }
}

void jaroCmdUp(uint8_t channel) {
  jarolift.cmdUp(channel);
  mqttSendPosition(channel, 0);
};

void jaroCmdDown(uint8_t channel) {
  jarolift.cmdDown(channel);
  mqttSendPosition(channel, 100);
};
void jaroCmdStop(uint8_t channel) { jarolift.cmdStop(channel); };

void jaroCmdShade(uint8_t channel) {
  jarolift.cmdShade(channel);
  mqttSendPosition(channel, 90);
};

void jaroCmdReInit() { jaroliftSetup(); };
void jaroCmdSetShade(uint8_t channel) { jarolift.cmdSetShade(channel); };
void jaroCmdLearn(uint8_t channel) { jarolift.cmdLearn(channel); };
void jaroCmdSetDevCnt(uint16_t value) { jarolift.setDeviceCounter(value); };
uint16_t jaroGetDevCnt() { return jarolift.getDeviceCounter(); };
bool getCC1101State() { return jarolift.getCC1101State(); };
uint8_t getCC1101Rssi() { return jarolift.getRssi(); }

void jaroliftCyclic() { jarolift.loop(); }