#include "wifi.hpp"

void myWifi::setWifiCredential(const char* ssid, const char* password)
{
  strncpy(_ssid, ssid, sizeof(_ssid));
  strncpy(_password, password, sizeof(_password));
  // Serial.println(password);
  // Serial.println(sizeof(password));
  // Serial.println(_password);
}

void myWifi::setOTACredential(const char* otaHostName, const char* otaPassword)
{
  strncpy(_otaHostName, otaHostName, sizeof(_otaHostName));
  strncpy(_otaPassword, otaPassword, sizeof(_otaPassword));
  Serial.print(F("OTA host: "));
  Serial.println(_otaHostName);
}

void myWifi::setMqttCredential(const char* mqttHost, int mqttPort=1883)
{
  strncpy(_mqttHost, mqttHost, sizeof(_mqttHost));
  _mqttPort = mqttPort;
  Serial.print(F("MQTT host: "));
  Serial.print(_mqttHost);
  Serial.print(F(":"));
  Serial.println(_mqttPort);
}

void myWifi::setupWifi()
{
  setUpMQTT();

  // The next two lines create handlers that will allow both the MQTT broker and Wi-Fi connection
  // to reconnect, in case the connection is lost.
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  connectToWifi();
  setUpOTA();
}

void myWifi::connectToWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);
  Serial.printf("Connecting to Wi-Fi: '%s' ...\n", _ssid);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(1000);
  }
  Serial.println();
}

void myWifi::connectToMqtt()
{
  Serial.println(F("Connecting to MQTT..."));
  mqttClient.connect();
}

void myWifi::onWifiConnect(const WiFiEventStationModeGotIP& event)
{
  Serial.printf("Connected to Wi-Fi. IP: %s\n", WiFi.localIP().toString().c_str());
  connectToMqtt();
}

// In case Wifi disconnected, try reconnect twice
void myWifi::onWifiDisconnect(const WiFiEventStationModeDisconnected& event)
{
  Serial.println(F("Disconnected from Wi-Fi."));
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void myWifi::OnCommand(const char* cmdTopic, CommandHandler cmdHandler)
{
  strncpy(_cmdTopic, cmdTopic, sizeof(_cmdTopic));
  _cmdHandler = cmdHandler;
}

void myWifi::setUpMQTT()
{
    // set the event callback functions
  mqttClient.onConnect([](bool sessionPresent) {
    Serial.println(F("Connected to MQTT."));
    #ifdef DEBUG
      Serial.printf("Session present: %s\n", sessionPresent ? "true" : "false");
    #endif

    // subscribe command topics
    //mqttClient.subscribe(MQTT_SUB_CMD, 2);
    mqttClient.subscribe("sumppit/cmd/#", 2);
  });

  mqttClient.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
    Serial.println(F("Disconnected from MQTT."));
    if (WiFi.isConnected()) mqttReconnectTimer.once(2, connectToMqtt);
  });

  mqttClient.onSubscribe([](uint16_t packetId, uint8_t qos) {
    Serial.printf("Subscribe acknowledged. packetId: %d, qos: %d\n", packetId, qos);
  });

  mqttClient.onUnsubscribe([](uint16_t packetId) {
    Serial.printf("Unsubscribe acknowledged.  packetId: %d\n", packetId);
  });

  mqttClient.onPublish([](uint16_t packetId) {
    Serial.printf("Publish acknowledged. packetId: %d\n", packetId);
  });

  mqttClient.onMessage([](char* topic, char* payload,
    AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
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
    
    _cmdHandler(topic, buffer);
  });

  // set MQTT server
  mqttClient.setServer(_mqttHost, _mqttPort);
  
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
}

void myWifi::setUpOTA()
{
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(_otaHostName);

  // No authentication by default
  ArduinoOTA.setPassword(_otaPassword);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = F("sketch");
    } else { // U_FS
      type = F("filesystem");
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println(F("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println(F("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println(F("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      Serial.println(F("End Failed"));
    }
  });
  ArduinoOTA.begin();
  Serial.printf("OTA Ready. IP: %s\n", WiFi.localIP().toString().c_str());
}
