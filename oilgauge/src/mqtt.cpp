#include "main.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"

extern void cmdHandler(const char* topic, const char* payload);

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

//---------------------------------------------------

void setupMqtt()
{
  // set the event callback functions
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);

  // set MQTT server
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  #ifdef DEBUG
  	Serial.printf("Session present: %s\n", sessionPresent ? "true" : "false");
  #endif

  // subscribe topics
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SUB_CMD, 2);
  Serial.printf("Subscribing at QoS 2, packetId: %d\n", packetIdSub);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.printf("Subscribe acknowledged. packetId: %d, qos: %d\n", packetId, qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.printf("Unsubscribe acknowledged.  packetId: %d\n", packetId);
}

// For both published/subscribed MQTT mesages
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
//  Serial.println("Publish received.");
//  Serial.printf("  topic: %s\n", topic);
//  Serial.printf("  qos: %d\n", properties.qos);
//  Serial.printf("  dup: %s\n", properties.dup ? "true" : "false");
//  Serial.printf("  retain: %s\n", properties.retain ? "true" : "false");
//  Serial.printf("  len: %d\n", len);
//  Serial.printf("  index: %d\n", index);
//  Serial.printf("  total: %d\n", total);
//  Serial.printf("  payload: %.*s\n", len, payload);

  // Command payload is usually short, 20 is enough
  static char buffer[20];
  strncpy(buffer, payload, min(len, (size_t)sizeof(buffer)));
  cmdHandler(topic, buffer);
}

void onMqttPublish(uint16_t packetId)
{
//  Serial.printf("Publish acknowledged. packetId: %d\n", packetId);
}

void msgInfo(const char* msg)
{
  mqttClient.publish(MQTT_PUB_INFO, 0, false, msg);
}

void msgWarn(const char* msg)
{
  mqttClient.publish(MQTT_PUB_WARN, 0, false, msg);
}

void msgError(const char* msg)
{
  mqttClient.publish(MQTT_PUB_ERROR, 0, false, msg);
}
