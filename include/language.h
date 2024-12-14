#pragma once
#include <config.h>

#define MAX_LANG 2 // increase if you add more languages

#ifndef MYLOG_LEVEL
#define MYLOG_LEVEL = ESP_LOG_INFO
#endif

namespace LANG {
constexpr const char *CODE[] = {"de", "en"};
};

// ======================================================================================
// mqtt messages : texts that are used for mqtt messages
// ======================================================================================
namespace WEB_TXT {
constexpr const char *ON[] = {"EIN", "ON"};
constexpr const char *OFF[] = {"AUS", "OFF"};
constexpr const char *OK[] = {"OK", "OK"};
constexpr const char *ERROR[] = {"FEHLER", "ERROR"};
constexpr const char *ACTIVE[] = {"aktiv", "active"};
constexpr const char *INACTIVE[] = {"inaktiv", "inactive"};
constexpr const char *FULL_DUPLEX[] = {"Vollduplex", "Full-Duplex"};
constexpr const char *CONNECTED[] = {"verbunden", "connected"};
constexpr const char *NOT_CONNECTED[] = {"nicht verbunden", "not connected"};
}; // namespace WEB_TXT

// ======================================================================================
// mqtt commands : texts that are used as topics for commands
// ======================================================================================
namespace MQTT_TOPIC {
constexpr const char *CMD_RESTART = {"/cmd/restart"};
constexpr const char *CMD_SHUTTER = {"/cmd/shutter"};
constexpr const char *STAT_SHUTTER = {"/status/shutter"};

}; // namespace MQTT_CMD

// ======================================================================================
// mqtt messages : texts that are used for mqtt messages
// ======================================================================================
namespace MQTT_MSG {
constexpr const char *DATETIME_CHANGED = {"date and time set to"};

}; // namespace MQTT_MSG
