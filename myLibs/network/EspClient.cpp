
#include "EspClient.hpp"

#include <ArduinoOTA.h>
#include <FS.h>
#include "LittleFS.h"

#include "AsyncJson.h"
#include "ArduinoJson.h"

#include <AsyncElegantOTA.h>

#include "sr04.hpp"
#include "dht11.hpp"
#include "vl53l0x.hpp"

Config &cfg = Config::instance();
StensTimer *pTimer = StensTimer::getInstance();

EspClient& EspClient::instance()
{
    static EspClient _instance;
    return _instance;
}

// void EspClient::setup(CommandHandler cmdHandler, const char* cmdTopic)
void EspClient::setup()
{
  #ifdef _DEBUG
    Serial.println("\n\nsetup()");
  #endif

  if(!cfg.loadConfig())
  {
    Serial.println(F("Failed to load configuration."));
    setupPortal(true); // blocking mode so other setup is onhold
  }else
  {
    setupPortal(false);
  }

  _setupWifi();
  _setupOTA(); // only set up OTA when got IP?
  _setupMQTT();
  
  _connectToWifi();

  _initSensors();

  //-- Create timers
  pTimer->setInterval(this, ACT_HEARTBEAT, 1e3);  // heartbeat every 1 second
  _timer_sensor = pTimer->setInterval(this, ACT_MEASURE, 10e3);   // every 10 seconds. Save the returned timer for reset the time interval
}

void EspClient::_initSensors()
{
  JsonArray arr = cfg.doc["sensors"].as<JsonArray>();

  // using C++11 syntax (preferred):
  for (JsonVariant value : arr) {
      
    JsonObject sensor = value.as<JsonObject>();
    String type = sensor["type"].as<String>();
    String name = sensor["name"].as<String>();

    Sensor *pSensor = NULL;
    if(type == "HC-SR04")
    {
      String pin0 = sensor["pins"][0].as<String>();
      String pin1 = sensor["pins"][1].as<String>();
      uint8_t pinTrig = cfg.pinByName(pin0);
      uint8_t pinEcho = cfg.pinByName(pin1);
      pSensor = new SR04(name.c_str(), pinTrig, pinEcho);

    }else if(type == "VL53L0X")
    {
      pSensor = new VL53L0X(name.c_str());

    }else if(type == "DHT11")
    {
      String pin0 = sensor["pins"][0].as<String>();
      uint8_t pinData = cfg.pinByName(pin0);
      pSensor = new DH11(name.c_str(), pinData);
    }

    if(pSensor != NULL)
    {
      String mqtt_pub_sensor = cfg.module + "/sensor/" + name;
      pSensor->setMqtt(&mqttClient, mqtt_pub_sensor.c_str(), 0, false);
      Serial.print(("Sensor init: ")); Serial.println(name);
    }else
    {
      Serial.print(F("Invalide sensor type: "));
      Serial.println(type);
    }

    _sensors.push_back(pSensor);
  }
}

void EspClient::loop()
{
  // let StensTimer do it's magic every time loop() is executed
  pTimer->run();

  // if(_wifiConnected) 
  ArduinoOTA.handle(); // OTA does not need wifi connected (AP also fine)

  delay(1); 
}

// Initiate a Wifi connection
void EspClient::_connectToWifi()
{
  #ifdef ESP8266
    WiFi.hostname(cfg.module.c_str());
  #elif defined(ESP32)
    WiFi.setHostname(cfg.module.c_str());
  #else
    #error Platform not supported
  #endif

  // Wnable WiFi station mode <= this is done in _setupWiFi()
  // WiFiMode_t mode = WiFi.getMode();
  // if(mode == WIFI_OFF) WiFi.mode(WIFI_STA);
  // else if(mode == WIFI_AP) WiFi.mode(WIFI_AP_STA);
    
  // check if static ip address configured (non-empty)
  if(cfg.ip != "")
  {
    IPAddress ip;   //ESP static ip
    if(ip.fromString(cfg.ip)) // parsing ip address
    {
      Serial.println(F("Using static ip"));

      IPAddress subnet(255,255,255,0);  //Subnet mask
      
      IPAddress gateway;
      if(gateway[0]=='\0' || !gateway.fromString(cfg.gateway))
        gateway = IPAddress(192,168,0,1);//IP Address of your WiFi Router (Gateway)

      IPAddress dns(0, 0, 0, 0);        //DNS 167.206.10.178
      // WiFi.config(ip, gateway, subnet);
      WiFi.config(ip, subnet, gateway, dns);

    }else
    {
      Serial.print(F("Invalid static IP:"));
      Serial.println(cfg.ip.c_str());
    }
  }

  Serial.println(F("Starting WiFi scan..."));
  int scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  if (scanResult == 0) {
    Serial.println(F("No networks found"));
  } else if (scanResult > 0) {

    #ifdef _DEBUG
    String ssid;
    int32_t rssi;
    uint8_t encryptionType;
    uint8_t* bssid;
    int32_t channel;
    bool hidden;

    Serial.printf(PSTR("%d networks found:\n"), scanResult);

    // Print unsorted scan results
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);

      Serial.printf(PSTR("  %02d: [CH %02d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %s\n"),
                    i,
                    channel,
                    bssid[0], bssid[1], bssid[2],
                    bssid[3], bssid[4], bssid[5],
                    rssi,
                    (encryptionType == ENC_TYPE_NONE) ? ' ' : '*',
                    hidden ? 'H' : 'V',
                    ssid.c_str());
      delay(0);
    }
    #endif

    if ( WiFi.status() != WL_CONNECTED )
    {
      #ifdef _DEBUG
      Serial.println(F("WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3"));
      Serial.print(F("WiFi mode: "));
      Serial.println(WiFi.getMode());
      Serial.println();

    // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
      Serial.println(F("0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses"));
      Serial.println(F("1 : WL_NO_SSID_AVAIL in case configured SSID cannot be reached"));
      Serial.println(F("3 : WL_CONNECTED after successful connection is established"));
      Serial.println(F("4 : WL_CONNECT_FAILED if connection failed"));
      Serial.println(F("6 : WL_CONNECT_WRONG_PASSWORD if password is incorrect"));
      Serial.println(F("7 : WL_DISCONNECTED if module is not configured in station mode\n"));
      Serial.print(F("WiFi status:"));
      #endif

      WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());

      // set timmer for Wifi connect timeout in 120s (will open portal in blocking mode!!)
      StensTimer *pTimer = StensTimer::getInstance(); 
      pTimer->setTimer(this, ACT_WIFI_CONNECT_TIMEOUT, WIFI_CONNECTING_TIMEOUT);

      while (WiFi.status() != WL_CONNECTED)
      {
        loop();
        Serial.print(WiFi.status());
        delay(1000);
      }
    }
  }

  WiFi.setAutoReconnect(true);    // Set whether module will attempt to reconnect to an access point in case it is disconnected.
}

// Try to connect to the MQTT broker and return True if the connection is successfull (blocking)
void EspClient::_connectToMqttBroker()
{
  mqttClient.connect();
  Serial.print(F("MQTT: Connecting "));
  Serial.println(cfg.mqttServer.c_str());
}

void EspClient::_setupWifi()
{
  #ifdef _DEBUG
    Serial.println("_setupWifi()");
  #endif

  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);

  // Disconnect from an AP if it was previously connected
  WiFi.disconnect();
  delay(100);

  // Set up WiFi even handler. NOTE:
  // 1. Must save the handler returned, otherwise it will be auto deleted and won't catch up any events
  //    see https://github.com/esp8266/Arduino/issues/2545
  // 2. For labmda function, check diff between [&], [=], [this]
  //    see https://stackoverflow.com/questions/21105169/is-there-any-difference-betwen-and-in-lambda-functions
  
  // static WiFiEventHandler CONNECT_HANDLER = WiFi.onStationModeConnected([&] (const WiFiEventStationModeConnected& event) {
  //   _printLine();
  //   Serial.println(F("WiFi: Connected"));
  //   _printLine();
  // });

  static WiFiEventHandler GOT_IP_HANDLER = WiFi.onStationModeGotIP([&] (const WiFiEventStationModeGotIP& event) {
    _printLine();
    Serial.print(F("WiFi: Got IP "));
    Serial.println(WiFi.localIP().toString().c_str());
    _printLine();

    //TODO: check if the IP is valide!!! e.g., not 192.168.x.x
    if(!_wifiConnected) // just got disconnected
    {
      _wifiConnected = true;

      // reconnect MQTT
      StensTimer *pTimer = StensTimer::getInstance(); 
      pTimer->setTimer(this, ACT_MQTT_RECONNECT, MQTT_RECONNECT_INTERVAL, MQTT_MAX_TRY);

      #ifdef _DEBUG
        Serial.println("MQTT: Set reconnect timmer");
      #endif

      _stopAP();
    }
  });

  static WiFiEventHandler DISCONNECT_HANDLER = WiFi.onStationModeDisconnected([&] (const WiFiEventStationModeDisconnected& event) {
    _printLine();
    Serial.println(F("WiFi: Disconnected"));
    _printLine();

    if(_wifiConnected) // just got disconnected
    {
      _startAP();
      _wifiConnected = false;
    }
  });
}

void EspClient::_setupMQTT()
{
  #ifdef _DEBUG
    Serial.println("_setupMQTT()");
  #endif

  // set the event callback functions
  mqttClient.onConnect([&](bool sessionPresent) {
    if(!_mqttConnected) // just got connected
    {
      // set delay timmer to subscribe command topic
      StensTimer *pTimer = StensTimer::getInstance(); 
      pTimer->setTimer(this, ACT_MQTT_SUBSCRIBE, MQTT_SUBSCRIBE_DELAY);

      // set the flag at the end to make sure there is no other hareware interrup action casusing exception!!
      _mqttConnected = true;

      _printLine();
      Serial.println(F("MQTT: Connected"));
      _printLine();
    }
  });

  mqttClient.onDisconnect([&](AsyncMqttClientDisconnectReason reason) {
    // reconnect MQTT, check if it is on to avoid recreate timer
    // if(!_mqttReconnectOn)

    if(_mqttConnected) // means just turned to disconnect, set reconnet timer
    {
      _mqttConnected = false;

      StensTimer *pTimer = StensTimer::getInstance(); 
      pTimer->setTimer(this, ACT_MQTT_RECONNECT, MQTT_RECONNECT_INTERVAL, MQTT_MAX_TRY);

      _printLine();
      Serial.println(F("MQTT: Disconnected"));
      _printLine();

      #ifdef _DEBUG
          _printMqttDisconnectReason(reason);
      #endif
    }
  });

  mqttClient.onSubscribe([&](uint16_t packetId, uint8_t qos) {
    #ifdef _DEBUG
      Serial.printf("Subscribe acknowledged. PacketId: %d, qos: %d\n", packetId, qos);
    #endif
  });

  mqttClient.onUnsubscribe([&](uint16_t packetId) {
    #ifdef _DEBUG
      Serial.print(F("Unsubscribe acknowledged. PacketId: "));
      Serial.println(packetId);
    #endif
  });

  mqttClient.onPublish([&](uint16_t packetId) {
    #ifdef _DEBUG
      Serial.print(F("Publish acknowledged. packetId: "));
      Serial.println(packetId);
    #endif
  });

  mqttClient.onMessage([&](char* topic, char* payload,
    AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

    #ifdef _DEBUG
      Serial.println(F("MQTT msg received:"));
      Serial.printf("  topic: %s\n", topic);
      Serial.printf("  qos: %d\n", properties.qos);
      Serial.printf("  dup: %s\n", properties.dup ? "true" : "false");
      Serial.printf("  retain: %s\n", properties.retain ? "true" : "false");
      Serial.printf("  len: %d\n", len);
      Serial.printf("  index: %d\n", index);
      Serial.printf("  total: %d\n", total);
      Serial.printf("  payload: %.*s\n", len, payload);
    #endif

    // NOTE: onMessage callback is hadware interuption triggered, if the _cmdHandler is not done yet, can cause
    // buffer mess and crash!! so do not do heavy work in _cmdHandler!!

    // Reason to make a copy: https://github.com/marvinroger/async-mqtt-client/issues/42
    static char buffer[20]; // Command payload is usually short, 20 is enough.
    // copies at most size-1 non-null characters from src to dest and adds a null terminator
    snprintf(buffer, min(len+1, (size_t)sizeof(buffer)), "%s", payload);

    int size = cfg.module.length();
    _cmdHandler(&(topic[size]), buffer);
  });

  // set MQTT server
  mqttClient.setServer(cfg.mqttServer.c_str(), cfg.mqttPort);
  
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(cfg.mqttUser.c_str(), cfg.mqttPass.c_str());
}

#ifdef _DEBUG
void EspClient::_printMqttDisconnectReason(AsyncMqttClientDisconnectReason reason)
{
    
      Serial.print(F("reason: "));
      if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
        Serial.println(F("Bad server fingerprint."));
      } else if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
        Serial.println(F("TCP Disconnected."));
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION) {
        Serial.println(F("Bad server fingerprint."));
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
        Serial.println(F("MQTT Identifier rejected."));
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
        Serial.println(F("MQTT server unavailable."));
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
        Serial.println(F("MQTT malformed credentials."));
      } else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
        Serial.println(F("MQTT not authorized."));
      } else if (reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE) {
        Serial.println(F("Not enough space on esp8266."));
      }
}
#endif

// This is called in WiFi connected callback set in _setupWifi()
// NOTE: 1. Must do this after Wifi got IP address. Otherwise, won't work!
//       2. OTA is using mDNS, so no need to set mDNS specifically
void EspClient::_setupOTA()
{
  #ifdef _DEBUG
    Serial.println("_setupOTA()");
  #endif

  // OTA port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  Serial.print("otaName:"); Serial.println(cfg.module.c_str());
  ArduinoOTA.setHostname(cfg.module.c_str());

  // No authentication by default
  Serial.print("otaPass:"); Serial.println(cfg.otaPass.c_str());
  ArduinoOTA.setPassword(cfg.otaPass.c_str());

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
    Serial.printf("OTA: Error[%u]: ", error);

    #ifdef _DEBUG
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
    #endif
  });

  ArduinoOTA.begin();
  Serial.printf("OTA: %s @ %s\n", cfg.module.c_str(), WiFi.localIP().toString().c_str());
}

// This will start a webserver allowing user to configure all settings via web.
// Two approach to access the webserver:
//     1. Connect AP WiFi SSID (usually named "ESP-XXXX"). Browse 192.168.4.1 
//        (Confirm the console prompt for the actual AP IP address). This
//        approach is good even the module is disconnected from family Wifi router.
//     2. Connect via Family network. Only works when the module is still connected
//        to the family Wifi router with a valid ip address.
// parameter: force means force open the portal (even there is no connection issue)
void EspClient::setupPortal(bool blocking) //char const *apName, char const *apPassword)
 {
  _portalOn = true;         // this will enable the pollPortal call in the loop of main.cpp
  _portalSubmitted = false; // used when portal is in blocking mode. will be set to true when the configuration is done in _handleConfigPortal

  // -- setup web handlers ------------------------------
  // Route for root / web page
  // change the index.html.gz (gzipped) file name to index.html and uploadd to esp. This will work
  // on both chrome and Safari (.gz file name won't work on Safari!).
  _webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Homepage request");
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html", "text/html", false);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);    
  });

  _webServer.on(PSTR("/api/files/list"), HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Got list");

    String JSON;
    StaticJsonDocument<1000> jsonBuffer;
    JsonArray files = jsonBuffer.createNestedArray("files");
 
    //get file listing
    Dir dir = LittleFS.openDir("");
    while (dir.next())
        files.add(dir.fileName()); //.substring(1));

    //get used and total data
    FSInfo fs_info;
    LittleFS.info(fs_info);
    jsonBuffer["used"] = String(fs_info.usedBytes);
    jsonBuffer["max"] = String(fs_info.totalBytes);
    serializeJson(jsonBuffer, JSON);
    Serial.println(JSON);
    request->send(200, PSTR("text/html"), JSON);
  });

  _webServer.on(PSTR("/api/files/upload"), HTTP_POST, [](AsyncWebServerRequest *request) {},
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){

    static File fsUploadFile;

    if (!index)
    {
        Serial.println(PSTR("Start file upload"));
        Serial.println(filename);

        if (!filename.startsWith("/"))
            filename = "/" + filename;

        // Delete existing file, otherwise it will appended to the file
        if(LittleFS.exists(filename)) LittleFS.remove(filename);

        fsUploadFile = LittleFS.open(filename, "w");
    }

    for (size_t i = 0; i < len; i++)
    {
        fsUploadFile.write(data[i]);
    }

    if (final)
    {
        String JSON;
        StaticJsonDocument<100> jsonBuffer;

        jsonBuffer["success"] = fsUploadFile.isFile();
        serializeJson(jsonBuffer, JSON);

        request->send(200, PSTR("text/html"), JSON);
        fsUploadFile.close();       
    }
  });

  //remove file
  _webServer.on(PSTR("/api/files/remove"), HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("filename", true))
    {
      #ifdef _DEBUG
      Serial.print("filename: "); Serial.println(request->getParam("filename", true)->value());
      #endif
      LittleFS.remove("/" + request->getParam("filename", true)->value());
      request->send(200, PSTR("text/html"), "File removed.");
    }else
    {
      request->send(500, PSTR("text/html"), "File does not exists!");
    }
  });

  _webServer.on("/restart", HTTP_GET, [&](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Restarted");

    StensTimer *pTimer = StensTimer::getInstance(); 
    pTimer->setTimer(this, ACT_CMD_RESTART, 100);
  });

  // send config.json per client request
  _webServer.on("/api/config/get", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get config.");
    request->send(LittleFS, "/config.json", "text/plain");
  });

  // NOTE: this on is using Json handler!! => AsyncCallbackJsonWebHandler
  _webServer.addHandler(new AsyncCallbackJsonWebHandler("/api/config/set", [&](AsyncWebServerRequest *request, JsonVariant &json) {
    String buffer;
    serializeJsonPretty(json, buffer);
    Serial.println(buffer);

    //TODO: validate the config json
    DeserializationError error = deserializeJson(cfg.doc, buffer);
    if (error)
    {
      Serial.println(F("Failed to read file, using default configuration"));
      request->send(422, "text/plain", "Invalid json string.");
    } else
    {
      cfg.saveConfig();
      request->send(200, "application/json", buffer);
    }
  }));

  // Do not close the webserver, instead set it response 404 instead, then the webOTA is still on
  _webServer.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  AsyncElegantOTA.begin(&_webServer);  // Start ElegantOTA right before webserver start

  _webServer.begin(); // start web server

  _printLine();
  Serial.println(F("Config portal on"));
  // if(_wifiConnected) Serial.printf("Browse http://%s/ or %s for portal, or\n", cfg.module.c_str(), WiFi.localIP().toString().c_str());
  // Serial.printf("Connect to Wifi \"%s\" and browse http://%s/ or %s.\n", ssid, cfg.module.c_str(), WiFi.softAPIP().toString().c_str());
  _printLine();

  if(blocking)
  {
    // blocking mode, open untill configuration done!
    Serial.println(F("Portal open on blocking mode."));
    while(_portalOn && !_portalSubmitted)
    {
      loop();
      delay(1000);
    } 
  }
}

void EspClient::_startAP()
{
  if(WiFi.getMode() == WIFI_AP_STA) return;

  // construct ssid name
  char ssid[40];    // this will be used as AP WiFi SSID
  int n = snprintf(ssid, sizeof(ssid), "ESP-%s-%zu", cfg.module.c_str(), ESP.getChipId());
  if(n == sizeof(ssid)) Serial.println("ssid might be trunckated!");

  // Enable WiFi AP mode  <= This is done in _setupWiFi()
  // WiFiMode_t mode = WiFi.getMode();
  // if(mode == WIFI_OFF) WiFi.mode(WIFI_AP);
  // else if(mode == WIFI_STA) WiFi.mode(WIFI_AP_STA);
  // WiFi.mode(WIFI_AP_STA);

  _printLine();
  Serial.printf("AP on: "); Serial.println(ssid);
  _printLine();

  // NOTE: WiFi.softAP call will change WiFi mode to WIFI_AP_STA
  // Serial.printf("startAP WiFi mode0: %d\n", WiFi.getMode());
  WiFi.softAP(ssid, cfg.apPass); // wpa2 requires an (exact) 8 character password.
  // Serial.printf("startAP WiFi mode1: %d\n", WiFi.getMode());
}

void EspClient::_stopAP()
{
  if(WiFi.getMode() == WIFI_STA) return;

  // NOTE: WiFi.softAPdisconnect call will change WiFi mode back to WIFI_STA
  // Serial.printf("stopAP WiFi mode0: %d\n", WiFi.getMode());
  WiFi.softAPdisconnect (true);
  // Serial.printf("stopAP WiFi mode1: %d\n", WiFi.getMode());

  _printLine();
  Serial.printf("AP off");
  _printLine();
}

void EspClient::timerCallback(Timer* timer)
{
    /* check if the timer is one we expect */
    int action = timer->getAction();

    switch(action)
    {
      case ACT_HEARTBEAT:
      {
        static String mqtt_pub_heartbeat = cfg.module + MQTT_PUB_HEARTBEAT;
        mqttClient.publish(mqtt_pub_heartbeat.c_str(), 0, true); // send heartbeat message
        _blink();
        break;
      }

      case ACT_MEASURE:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_MEASURE"));
        #endif

        if (_autoMode) _measure();
        break;

      case ACT_CMD_RESTART:
        _restart();
        break;

      case ACT_CMD_MEASURE:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_CMD_MEASURE"));
        #endif

        _measure();
        break;

      case ACT_WIFI_CONNECT_TIMEOUT:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_WIFI_CONNECT_TIMEOUT"));
        #endif

        if(_wifiConnected) Serial.println(F("WIFI: connected already!"));
        break;

      case ACT_MQTT_RECONNECT:
      {
        int repetitions = timer->getRepetitions();

        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_MQTT_RECONNECT"));
          if(_mqttConnected) Serial.println(F("MQTT connected already"));
          Serial.println(F("Repetitions: "));
          Serial.println(repetitions);
        #endif
        
        if(repetitions > 1)
        {
          // not reaching max number of reconnect try, continue if wifi is on but mqtt is not
          if(_wifiConnected && !_mqttConnected) _connectToMqttBroker();
        }
        else
        {
          // reach max try, create a longer timer if not connected which means
          // mqtt will keep trying untill it is connected.
          if(!_mqttConnected)
          {
            // Reset MQTT connect timer to longer interval if wifi is not connected, otherwise same interval
            // StensTimer *pTimer = StensTimer::getInstance(); 
            // pTimer->setTimer(this, ACT_MQTT_RECONNECT,
            //   _wifiConnected ? MQTT_RECONNECT_INTERVAL:MQTT_RECONNECT_INTERVAL_LONG, MQTT_MAX_TRY);

            // avoid recreate new timer object!
            timer->setDelay(_wifiConnected ? MQTT_RECONNECT_INTERVAL:MQTT_RECONNECT_INTERVAL_LONG);
            timer->setRepetitions(MQTT_MAX_TRY)
          }
        }
        break;
      }

      case ACT_MQTT_SUBSCRIBE:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_MQTT_SUBSCRIBE"));
        #endif

        // check mqtt connection before subscribe. Sometime it disconnect
        if(_mqttConnected)
        {
          String module = String(cfg.module);
          // module.toLowerCase();
          auto cmdTopic = module + "/cmd/#";

          Serial.print(F("Subscribe command topic: "));
          Serial.println(cmdTopic);

          // subscribe command topics
          mqttClient.subscribe(cmdTopic.c_str(), 2);
        }
        break;
    }
}

void EspClient::_blink()
{
  // blink the built-in LED light
  if(_ledBlink)
  {
    static bool led_on = true;
    digitalWrite(LED_BUILTIN, led_on ? LOW : HIGH); // turn the LED on (LOW) and off(HIGHT). It is inverted.
    led_on = !led_on;
  }else
  {
    digitalWrite(LED_BUILTIN, HIGH); // turn it off
  }
}

void EspClient::_measure()
{
  for(Sensor * pSensor : _sensors)
  {
    if(pSensor != NULL && isConnected())
      pSensor->sendMeasure();
  }
}

void EspClient::_enableSensor(const char* name, bool enable)
{
  // for(int i=0; i<MAX_SENSORS; i++)
  for(Sensor *pSensor : _sensors)
  {
    if(pSensor != NULL && strcmp(name, pSensor->name) == 0)
    {
      Serial.print(name);
      Serial.print(F(" sensor enabled: "));
      Serial.println(enable);

      pSensor->enable(enable);
    }
  }
}

// NOTE: this is called in MQTT onMessage() based on interrupt. Do not do heavy duty staff!
// if you have to, set a ACT timer and do it overthere, e.g., 'CMD_MEASURE => ACT_MEASURE'
void EspClient::_cmdHandler(const char* topic, const char* payload)
{
  #ifdef _DEBUG
    Serial.println(topic);
    Serial.println(payload);
  #endif

  if(strcmp(topic, CMD_SSR_FILTER) == 0)
  {
    int filter = atoi(payload);

    // for(int i=0; i<MAX_SENSORS; i++)
    for(Sensor * pSensor : _sensors)
    {
      if(pSensor != NULL) pSensor->setFilter((FilterType)filter);
    }
  }
  else if(strcmp(topic, CMD_LED_BLINK) == 0)
  {
    if(strcmp(payload, "true") == 0) _ledBlink = true;
    else _ledBlink = false;
  }
  else if(strcmp(topic, CMD_AUTO) == 0)
  {
    if(strcmp(payload, "true") == 0) _autoMode = true;
    else _autoMode = false;
  }
  else if(strcmp(topic, CMD_MEASURE) == 0)
  {
    // measure may take longer time, leave the task to timer with one-time timer
    pTimer->setTimer(this, ACT_CMD_MEASURE, 100);
  }
  else if(strcmp(topic, CMD_INTERVAL) == 0)
  {
    // reset the interval of the timer
    _timer_sensor->setDelay(atoi(payload)*1000);

  }
  else if(strcmp(topic, CMD_RESTART) == 0)
  {
    _restart();
  }
  // else if(strcmp(topic, CMD_RESET_WIFI) == 0)
  // {
  //   _resetWifi();
  //   _restart();
  // }
  else if(strstr(topic, "/cmd/on/") != NULL)
  {
      // find he last occurrence of '/'
      char *ptr = strrchr(topic, '/');

      if(ptr != NULL)
      {
        Serial.print(F("Sensor: ")); Serial.println(ptr+1);
        _enableSensor(ptr+1, strcmp(payload, "true") == 0);
      }
  }
  else if(strcmp(topic, CMD_SLEEP) == 0)
  {
    if(strcmp(payload, "Modem") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Modem..."));
      #endif
      //  uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, "Modem");
    }
    else if(strcmp(payload, "Light") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Light..."));
      #endif
      // uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, "Light");
    }
    else if(strcmp(payload, "Deep") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Deep..."));
      #endif

      // After upload code, connect D0 and RST. NOTE: DO NOT connect the pins if using OTG uploading code!
//      String msg = "I'm awake, but I'm going into deep sleep mode for 60 seconds";
//      uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, msg.c_str());
//      delay(1000);
//      ESP.deepSleep(0); //10e6); 
    }
    else if(strcmp(payload, "Normal") == 0)
    {
      #ifdef _DEBUG
        Serial.println(F("Normal..."));
      #endif
      // uint16_t packetIdPub = mqttClient.publish(MQTT_PUB_INFO, 1, false, "Normal");
    }
  }  
}

void EspClient::_restart(RsCode code)
{
  Serial.print(F("Restart device, code: "));
  Serial.println(code);

  // disconnect mqtt broker
  mqttClient.disconnect(true);

  // stop OTA (Seems AruinoOTA does not have a end() to call)
  // ArduinoOTA.end();
  
  // delete all sensor objects in _sensors[]
  for(Sensor * pSensor : _sensors)
  {
    if(pSensor != NULL) delete pSensor;
  }

  #ifdef ESP8266
    ESP.reset();
  #elif defined(ESP32)
    ESP.restart();
  #else
      #error Platform not supported
  #endif
}