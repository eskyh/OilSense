#pragma once

// Async MQTT client for ESP8266 and ESP32
// Source: https://registry.platformio.org/libraries/marvinroger/AsyncMqttClient
// Samples: https://registry.platformio.org/libraries/marvinroger/AsyncMqttClient/examples
#include <AsyncMqttClient.h>

//--------------------------------------------------
// Raspberri Pi Mosquitto MQTT Broker
//#define MQTT_HOST IPAddress(192, 168, 0, 194)
// For a cloud MQTT broker, type the domain name
#define MQTT_HOST "raspberrypi.local" //Does not work: "esky.tplinkdns.com"
#define MQTT_PORT 1883

//-------------------------------------------------
// MQTT Pub/Sub topics

// #define PRJNAME "oilgauge"

// #define MQTT_PUB_HEARTBEAT  NAME"/heartbeat"
// #define MQTT_PUB_INFO       NAME"/info"

#define MQTT_SUB_CMD        PRJNAME"/cmd/#"

// #define PUB_QOS 0

extern AsyncMqttClient mqttClient;
extern Ticker mqttReconnectTimer;

void setupMqtt();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);

void msgInfo(const char* msg);
void msgWarn(const char* msg);
void msgError(const char* msg);
