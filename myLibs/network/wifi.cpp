#include "wifi.hpp"
// <time.h> and <WiFiUdp.h> not needed. already included by core.
#include <EEPROM.h>

void myWifi::syncTimeNTP()
{
  // // When time() is called for the first time, the NTP sync is started immediately. It then takes 15 to 200ms or more to complete.
  configTime(TZ_America_New_York, "pool.ntp.org"); // daytime saying will be adjusted by SDK automatically.
  time(NULL); // fist call to start sync
}

void myWifi::waitSyncNTP()
{
  #ifdef _DEBUG
    unsigned long t0 = millis();
  #endif

  Serial.println("Sync time over NTP...");
  while(time(NULL) < NTP_MIN_VALID_EPOCH) {
    delay(500);
  }
  Serial.println(F("NTP synced."));

  #ifdef _DEBUG
    unsigned long t1 = millis() - t0;
    Serial.println("NTP time first synch took " + String(t1) + "ms");
  #endif
}

void myWifi::setupWifiListener()
{
  // The next two lines create handlers that will allow both the MQTT broker and Wi-Fi connection
  // to reconnect, in case the connection is lost.
  if(wifiConnectHandler == nullptr)
    wifiConnectHandler = WiFi.onStationModeGotIP([] (const WiFiEventStationModeGotIP& event) {
      Serial.print(F("Connected to Wi-Fi. IP: "));
      Serial.println(WiFi.localIP().toString().c_str());
      syncTimeNTP();
      setUpMQTT();
      connectToMqtt(); //or mqttReconnectTimer.once(1, connectToMqtt);
      setUpOTA();
    });

  if(wifiDisconnectHandler == nullptr)
    wifiDisconnectHandler = WiFi.onStationModeDisconnected([] (const WiFiEventStationModeDisconnected& event)
      {
        Serial.println(F("Disconnected from Wi-Fi."));
        // mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        // if(!portalOn) wifiReconnectTimer.once(2, autoConnect);
        wifiReconnectTimer.once(2, connect);
      });
}

void myWifi::connect()
{
  Serial.println(F("Connecting WiFi..."));
  // // check the reason of the the two line below: https://github.com/esp8266/Arduino/issues/2186
  // WiFi.persistent(false);
  // WiFi.disconnect();

  // do not change the mode (can be WIFI_STA or WIFI_AP_STA), just reconnect
  WiFi.begin(settings.ssid, settings.pass);
}

void myWifi::autoConnect(CommandHandler cmdHandler, const char* cmdTopic)
{
  // set up cmdHandler and cmdTopic
  // do this before anything else, as turn on portal cmd will be send to cmdHandler if there is connection issue
  _cmdHandler = cmdHandler;
  strncpy(_cmdTopic, cmdTopic, sizeof(_cmdTopic));
  
  // set up wifi listener if needed
  setupWifiListener();

  // read settings from EEPROM
  getSettings();

  Serial.println(F("Connecting WiFi..."));
  // check the reason of the the two line below: https://github.com/esp8266/Arduino/issues/2186
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.ssid, settings.pass);
  //uncomment below to trigger startConfigPortal
  // WiFi.disconnect(true);

  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (tries++ > 10) {
      #ifdef _DEBUG
        Serial.println(F("Can not connect WiFi. Start portal..."));
      #endif

      // this inform the main loop to open portal! (Can not handle it in the event handler or timer)
      if(_cmdHandler!= NULL)
      {
        _cmdHandler("cmdOpenPortal", NULL);
        strncpy(portalReason, "Wifi connect failure!", sizeof(portalReason));      
      }else
      {
        Serial.println(F("cmdHandler is not set!"));
      }
      break;
    }
    delay(1000);
  }

  if(WiFi.isConnected()) waitSyncNTP();
}

// This will start a AP allowing user to configure all settings by accessing
// a webserver on the AP. To access the page, connect to the AP ssid (ESP-XXXX)
// From browser input 192.168.4.1 (Check the serial port prompt for the actual AP IP address)
void myWifi::startConfigPortal(bool force) //char const *apName, char const *apPassword)
 {
   Serial.print("Force mode: ");
   Serial.println(force);

  // connect will be set to true when the configuration is done in handlePortal
  portalOn = true;   // this will enable the pollPortal call in the loop of main.cpp
  forcePortal = force;

  // if(!WiFi.isConnected()){
  //   WiFi.persistent(false);
  //   // disconnect sta, start ap
  //   WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
  //   WiFi.mode(WIFI_AP);
  //   WiFi.persistent(true);
  // }
  // else {
  //   //setup AP
  //   // WiFi.disconnect(true);
  //   WiFi.mode(WIFI_AP_STA);
  // }

  // use WIFI_AP_STA model such that WiFi reconnecting still going on
  // This is useful when the router is off. Once router is back on
  // the WiFi will reconnect and close the portal automatically.
  WiFi.mode(WIFI_AP_STA);

  String ssidAP = "ESP-" + String(ESP.getChipId());
  WiFi.softAP(ssidAP.c_str(), NULL); // apPassword);
  server.on("/",  handlePortal);
  server.begin();

  Serial.print(F("Configuration portal is on.\nSSID: "));
  Serial.print(ssidAP);
  Serial.print(F(", IP:"));
  Serial.println(WiFi.softAPIP());
  Serial.println(F("Connect AP Wifi and access the IP from Browser for configuration."));
}

// A loop to enable the server process client request and will invoke the
// callback function handlePortal set above. The loop will be running untill
// the connect is set to true within handlePortal when configuration is done.
//---------------------------------------------------------------------------
// Stop condition: 1) Configuration form submitted
//                 2) Portal non-force open, WiFi and MQTT are reconnected
void myWifi::pollPortal()
{
  if(!portalOn) return;

  // handle config portal form POST. When it is done, set portalOn false.
  server.handleClient();

  if (!portalOn) // Form submitted and handled by server
  {
    delay(1000); // leave enough time for handlePortal server.send() to complete before closing softAP mode.
    Serial.println(F("Configuration portal completed."));

    // The portal may be forced to open when WiFi is still on (e.g., cmd sent from Node-Red dashboard)
    Serial.println(F("Close AP."));
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    // reconnect WiFi if needed
    if(!WiFi.isConnected())  WiFi.begin(settings.ssid, settings.pass);

    // reconnect MQTT broker if needed
    if(!mqttClient.connected()) connectToMqtt();
    
    server.stop();
    return;
  }

  // return if WiFi is connected and mqtt is connected
  // this can happen when wifi is still reconnect periodically while the portal is on
  if(!forcePortal && WiFi.isConnected() && mqttClient.connected())
  {
    Serial.println(F("WiFi & MQTT reconnected. Exit portal."));
    server.stop();
    portalOn = false;
  }
}

void myWifi::handlePortal()
{
  if (server.method() == HTTP_POST)
  {
    Serial.println("POST received.");
  
    server.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Success!</h1> <br/> <p>Your settings have been saved successfully!<br />Device is starting...</p></main></body></html>");

    // save configuration received    
    strncpy(settings.ssid, server.arg("ssid").c_str(), sizeof(settings.ssid));
    strncpy(settings.pass, server.arg("password").c_str(), sizeof(settings.pass));

    strncpy(settings.mqttHost, server.arg("mqttHost").c_str(), sizeof(settings.mqttHost));
    settings.mqttPort = atoi(server.arg("mqttPort").c_str());
    strncpy(settings.mqttUser, server.arg("mqttUser").c_str(), sizeof(settings.mqttUser));
    strncpy(settings.mqttPass, server.arg("mqttPass").c_str(), sizeof(settings.mqttPass));
    strncpy(settings.otaHost, server.arg("otaHost").c_str(), sizeof(settings.otaHost));
    strncpy(settings.otaPass, server.arg("otaPass").c_str(), sizeof(settings.otaPass));

    setSettings();
    
    portalOn = false; // to turn off portal
  } else {
    // blank
    //server.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color:red;'>" + String(portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' type='pass' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='raspberrypi.local' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='1883' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' type='pass' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' type='pass' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");
    
    // with password mode
    //server.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='" + String(settings.ssid) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' type='password' value='" + String(settings.pass) + "' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='" + String(settings.mqttHost) + "' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='" + String(settings.mqttPort) + "' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='" + String(settings.mqttUser) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' type='password' value='" + String(settings.mqttPass) + "' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' value='" + String(settings.otaHost) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' type='password' value='" + String(settings.otaPass) + "' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");

    // non-password mode
    server.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='" + String(settings.ssid) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' value='" + String(settings.pass) + "' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='" + String(settings.mqttHost) + "' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='" + String(settings.mqttPort) + "' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='" + String(settings.mqttUser) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' value='" + String(settings.mqttPass) + "' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' value='" + String(settings.otaHost) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' value='" + String(settings.otaPass) + "' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");
  }
}

// use CRC to check EEPROM data
// https://community.particle.io/t/determine-empty-location-with-eeprom-get-function/18869/12
void myWifi::getSettings()
{
  EEPROM.begin(sizeof(struct Settings));
  EEPROM.get(0, settings);

  #ifdef _DEBUG
    Serial.println("Read from EEPROM:");
    Serial.println(settings.ssid);
    Serial.println(settings.pass);
    Serial.println(settings.mqttHost);
    Serial.println(settings.mqttPort);
    Serial.println(settings.mqttUser);
    Serial.println(settings.mqttPass);
    Serial.println(settings.otaHost);
    Serial.println(settings.otaPass);
  #endif
}

void myWifi::setSettings()
{
  #ifdef _DEBUG
    Serial.println("Set EEPROM:");
    Serial.println(settings.ssid);
    Serial.println(settings.pass);
    Serial.println(settings.mqttHost);
    Serial.println(settings.mqttPort);
    Serial.println(settings.mqttUser);
    Serial.println(settings.mqttPass);
    Serial.println(settings.otaHost);
    Serial.println(settings.otaPass);
  #endif
    
  // EEPROM.begin(sizeof(struct settings));
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void myWifi::connectToMqtt()
{
  mqttClient.connect();
  Serial.print(F("Connecting to MQTT: "));
  Serial.println(settings.mqttHost);
}

void myWifi::subscribeMqtt()
{
  Serial.println(F("listen for commands..."));
   // subscribe command topics
   mqttClient.subscribe(_cmdTopic, 2);
}

void myWifi::setUpMQTT()
{
    // set the event callback functions
  mqttClient.onConnect([](bool sessionPresent) {
    _nMqttReconnect = 0;

    Serial.println(F("Connected to MQTT."));
    #ifdef _DEBUG
      Serial.print("Session present: ");
      Serial.println(sessionPresent);
    #endif

    // subscribe command topics. Do not call subscribeMqtt() directly in eventHandler.
    // Better call it with a schedule timer!! Also do not use delay() in eventhandler as well!
    // refer https://github.com/marvinroger/async-mqtt-client/issues/264
    mqttsubscribeTimer.once(1, subscribeMqtt);
  });

  mqttClient.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
    #ifdef _DEBUG
      Serial.print("Disconnected from MQTT, reason: ");
      if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
        Serial.println("Bad server fingerprint.");
      } else if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
        Serial.println("TCP Disconnected.");
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION) {
        Serial.println("Bad server fingerprint.");
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
        Serial.println("MQTT Identifier rejected.");
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
        Serial.println("MQTT server unavailable.");
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
        Serial.println("MQTT malformed credentials.");
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
        Serial.println("MQTT not authorized.");
      } else if (reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE) {
        Serial.println("Not enough space on esp8266.");
      }
    #endif

    _nMqttReconnect++;

    // subscribe command topics. Do not call connectToMqtt() directly in eventHandler.
    // Better call it with a schedule timer!! Also do not use delay() in eventhandler as well!
    // refer https://github.com/marvinroger/async-mqtt-client/issues/264
    if (WiFi.isConnected() && _nMqttReconnect <=3)
      mqttReconnectTimer.once(2, connectToMqtt);
    else
    {
      // this inform the main loop to open portal! (Can not handle it in the event handler or timer)
      _cmdHandler("cmdOpenPortal", NULL);
      strncpy(portalReason, "MQTT broker connect failure!", sizeof(portalReason));
    }
  });

  mqttClient.onSubscribe([](uint16_t packetId, uint8_t qos) {
    #ifdef _DEBUG
      Serial.printf("Subscribe acknowledged. PacketId: %d, qos: %d\n", packetId, qos);
    #endif
  });

  mqttClient.onUnsubscribe([](uint16_t packetId) {
    #ifdef _DEBUG
      Serial.print(F("Unsubscribe acknowledged. PacketId: "));
      Serial.println(packetId);
    #endif
  });

  mqttClient.onPublish([](uint16_t packetId) {
    #ifdef _DEBUG
      Serial.print(F("Publish acknowledged. packetId: "));
      Serial.println(packetId);
    #endif
  });

  mqttClient.onMessage([](char* topic, char* payload,
    AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    
    #ifdef _DEBUG
      Serial.println("Publish received.");
      Serial.printf("  topic: %s\n", topic);
      Serial.printf("  qos: %d\n", properties.qos);
      Serial.printf("  dup: %s\n", properties.dup ? "true" : "false");
      Serial.printf("  retain: %s\n", properties.retain ? "true" : "false");
      Serial.printf("  len: %d\n", len);
      Serial.printf("  index: %d\n", index);
      Serial.printf("  total: %d\n", total);
      Serial.printf("  payload: %.*s\n", len, payload);
    #endif

    // Command payload is usually short, 20 is enough
    static char buffer[20];
     // copies at most size-1 non-null characters from src to dest and adds a null terminator
    snprintf(buffer, min(len+1, (size_t)sizeof(buffer)), "%s", payload);

    _cmdHandler(topic, buffer);
  });

  // set MQTT server
  mqttClient.setServer(settings.mqttHost, settings.mqttPort);
  
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(settings.mqttUser, settings.mqttPass);
}

void myWifi::setUpOTA()
{
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(settings.otaHost);

  // No authentication by default
  ArduinoOTA.setPassword(settings.otaPass);

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
