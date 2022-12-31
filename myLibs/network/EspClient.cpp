
#include "EspClient.hpp"

#include <ArduinoOTA.h>
#include <FS.h>
#include "LittleFS.h"

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
    Serial.println(F("Failed to load configuration, open portal."));
    openConfigPortal(true);
  }

  _setupWifi();
  _setupMQTT();
  
  _connectToWifi(true); // blocking mode

  _initSensors();

  //-- Create timers
  pTimer->setInterval(this, ACT_HEARTBEAT, 1e3);                  // heartbeat every 1 second
  _timer_sensor = pTimer->setInterval(this, ACT_MEASURE, 10e3);   // every 10 seconds. Save the returned timer for reset the time interval
}

void EspClient::_initSensors()
{
    // set up the sensors
  for(int i=0; i<MAX_SENSORS; i++)
  {
    CfgSensor cfgSensor = cfg.sensors[i];

    if(cfgSensor.type[0]=='\0') continue;

    Sensor *pSensor = NULL;
    if(strcmp(cfgSensor.type, "HC-SR04") == 0)
    {
      //pinTrig: pin0
      //pinEcho: pin1;
      pSensor = new SR04(cfgSensor.name, cfgSensor.pin0, cfgSensor.pin1);

    }else if(strcmp(cfgSensor.type, "VL53L0X") == 0)
    {
      pSensor = new VL53L0X(cfgSensor.name);

    }else if(strcmp(cfgSensor.type, "DHT11") == 0)
    {
      //pinData: pin0
      pSensor = new DH11(cfgSensor.name, cfgSensor.pin0);
    }

    if(pSensor != NULL)
    {
      String mqtt_pub_sensor = String(cfg.module) + "/sensor/" + pSensor->name;
      pSensor->setMqtt(&mqttClient, mqtt_pub_sensor.c_str(), 0, false);
      Serial.print("Sensor pub: "); Serial.println(mqtt_pub_sensor);
    }else
    {
      Serial.print(F("Invalide sensor type: "));
      Serial.println(cfgSensor.type);
    }

    _sensors[i] = pSensor;
  }
}

void EspClient::loop()
{
  // let StensTimer do it's magic every time loop() is executed
  pTimer->run();

  // // A loop to enable the _webServer process client request 
  // if(_portalOn) _webServer.handleClient();

  // Wifi handling
  if(!_wifiConnected)
  {
    // Do nothing else as ESP WiFi module take care of reconnecting (configured in _connectToWifi())
    return;

  }else
  {
    ArduinoOTA.handle();
  }

  // // MQTT reconnect handling if it is disconnected
  // if(_wifiConnected && !_mqttConnected && !_mqttConnecting)
  // {
  //   // mqtt is not connected yet.

  //   // At least 500 miliseconds of waiting before an mqtt connection attempt.
  //   // Some people have reported instabilities when trying to connect to 
  //   // the mqtt broker right after being connected to wifi.
  //   // This delay prevent these instabilities.

  //   // set timmer to connect MQTT broker
  //   // At least 500 miliseconds of waiting before an mqtt connection attempt.
  //   // Some people have reported instabilities when trying to connect to 
  //   // the mqtt broker right after being connected to wifi.
  //   // This delay prevent these instabilities.
  // }

  // process command msgs. MQTT cmd msg should be handled in the main loop!

}

// Initiate a Wifi connection
void EspClient::_connectToWifi(bool blocking)
{
  #ifdef ESP8266
    WiFi.hostname(cfg.module);
  #elif defined(ESP32)
    WiFi.setHostname(cfg.module);
  #else
    #error Platform not supported
  #endif

  // check the reason of the the two line below: https://github.com/esp8266/Arduino/issues/2186
  // turn off wif then on (in case it is auto pown on connecting)
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);   // this is a temporary line, to be removed after SDK update to 1.5.4
  WiFi.mode(WIFI_STA);

  // Run the following two lines once to persistently disalbe auto power on connection
  // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html#setautoconnect
  // https://randomnerdtutorials.com/solved-reconnect-esp8266-nodemcu-to-wifi/
  // WiFi.persistent(true);       // persistent is true by default, not anymore since v3 (https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html)
  // WiFi.setAutoConnect(false);  // (Disable it) Configure module to automatically connect on power on to the last used access point.

  // do not change the mode (can be WIFI_STA or WIFI_AP_STA), just reconnect

  // NOTE: The following reason is outdated, true reason explained down below (next NOTE:)
  // https://arduino.stackexchange.com/questions/78604/esp8266-wifi-not-connecting-to-internet-without-static-ip
  // Configure static IP will work for latest ESP8266 lib v3.0.2 where DHCP has problem
  // To make DHCP work, it need revert back to v2.5.2

  // check if static ip address configured (non-empty)
  if(cfg.ip[0]=='\0')
  {
    IPAddress ip;   //ESP static ip
    if(ip.fromString(cfg.ip)) // parsing ip address
    {
      Serial.println(F("Using static ip"));

      IPAddress subnet(255,255,255,0);  //Subnet mask
      
      IPAddress gateway;
      if(cfg.gateway[0]=='\0' || !gateway.fromString(cfg.gateway))
        gateway = IPAddress(192,168,0,1);   //IP Address of your WiFi Router (Gateway)

      IPAddress dns(0, 0, 0, 0);        //DNS 167.206.10.178
      WiFi.config(ip, subnet, gateway, dns);
    }
  }

  //------------------------------------------------------------------------------------------------------------------
  // NOTE: the reason above is persistent WiFi.setAutoConnect(true) which will trigger auto connect at power on
  // conflicting with the WiFi.begin(ssid, pass) call below. To solve this problem, WiFi.setAutoConnect(false), and
  // persistent the setting by WiFi.persistent(true).
  //------------------------------------------------------------------------------------------------------------------
  WiFi.begin(cfg.ssid, cfg.pass);

  // set timmer for Wifi connect timeout in 120s (will open portal in blocking mode!!)
  StensTimer *pTimer = StensTimer::getInstance(); 
  pTimer->setTimer(this, ACT_WIFI_CONNECT_TIMEOUT, WIFI_CONNECTING_TIMEOUT);

  if(blocking)
  {
    Serial.print("Connecting to Wifi");
    while (WiFi.status() != WL_CONNECTED)
    {
      pTimer->run(); // on blocking mode, need to handle timer logic to check timeout event!
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    // auto reconnect when lost WiFi connection
    // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html#setautoconnect
    // https://randomnerdtutorials.com/solved-reconnect-esp8266-nodemcu-to-wifi/
    // WiFi.persistent(true);       // persistent is true by default, not anymore since v3 (https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html)
    // WiFi.setAutoConnect(false);  // (Disable it) Configure module to automatically connect on power on to the last used access point.
    WiFi.setAutoReconnect(true);    // Set whether module will attempt to reconnect to an access point in case it is disconnected.
  }
}

// Try to connect to the MQTT broker and return True if the connection is successfull (blocking)
void EspClient::_connectToMqttBroker()
{
  mqttClient.connect();
  Serial.print(F("MQTT: Connecting "));
  Serial.println(cfg.mqttServer);
}

void EspClient::_setupWifi()
{
  #ifdef _DEBUG
    Serial.println("_setupWifi()");
  #endif

  // Set up WiFi even handler. NOTE:
  // 1. Must save the handler returned, otherwise it will be auto deleted and won't catch up any events
  //    see https://github.com/esp8266/Arduino/issues/2545
  // 2. For labmda function, check diff between [&], [=], [this]
  //    see https://stackoverflow.com/questions/21105169/is-there-any-difference-betwen-and-in-lambda-functions
  
  // static WiFiEventHandler CONNECT_HANDLER = WiFi.onStationModeConnected([&] (const WiFiEventStationModeConnected& event) {
  //   Serial.println(F("-------------------------------------"));
  //   Serial.println(F("WiFi: Connected"));
  //   Serial.println(F("-------------------------------------"));
  // });
  
  static WiFiEventHandler GOT_IP_HANDLER = WiFi.onStationModeGotIP([&] (const WiFiEventStationModeGotIP& event) {
    Serial.println(F("-------------------------------------"));
    Serial.print(F("WiFi: Got IP"));
    Serial.println(WiFi.localIP().toString().c_str());
    Serial.println(F("-------------------------------------"));
    _wifiConnected = true;

    // reconnect MQTT
    if(_pTimerReconnect == NULL)
    {
      StensTimer *pTimer = StensTimer::getInstance(); 
      _pTimerReconnect = pTimer->setTimer(this, ACT_MQTT_RECONNECT, MQTT_RECONNECT_INTERVAL, MQTT_MAX_TRY);
    }

    #ifdef _DEBUG
      Serial.println("MQTT: Set reconnect timmer");
    #endif


    // Only set up OTA when got IP!!
    _setupOTA();
  });

  static WiFiEventHandler DISCONNECT_HANDLER = WiFi.onStationModeDisconnected([&] (const WiFiEventStationModeDisconnected& event) {
    Serial.println(F("-------------------"));  
    Serial.println(F("WiFi: Disconnected"));
    Serial.println(F("-------------------"));
    _wifiConnected = false;
  });
}

void EspClient::_setupMQTT()
{
  #ifdef _DEBUG
    Serial.println("_setupMQTT()");
  #endif

  // set the event callback functions
  mqttClient.onConnect([&](bool sessionPresent) {
    Serial.println(F("-------------------"));
    Serial.println(F("MQTT: Connected"));
    Serial.println(F("-------------------"));

    #ifdef _DEBUG
      Serial.println(F("MQTT: connected already."));
    #endif

    if(_pTimerReconnect != NULL)
    {
      pTimer->deleteTimer(_pTimerReconnect);
      _pTimerReconnect = NULL;
    }

    // set delay timmer to subscribe command topic
    StensTimer *pTimer = StensTimer::getInstance(); 
    pTimer->setTimer(this, ACT_MQTT_SUBSCRIBE, MQTT_SUBSCRIBE_DELAY);

    // set the flag at the end to make sure there is no other hareware interrup action casusing exception!!
    _mqttConnected = true;
  });

  mqttClient.onDisconnect([&](AsyncMqttClientDisconnectReason reason) {
    _mqttConnected = false;

    // reconnect MQTT
    if(_pTimerReconnect == NULL)
    {
      StensTimer *pTimer = StensTimer::getInstance(); 
      _pTimerReconnect = pTimer->setTimer(this, ACT_MQTT_RECONNECT, MQTT_RECONNECT_INTERVAL, MQTT_MAX_TRY);
    }
    
    Serial.println(F("----------------------------------------------"));
    Serial.println(F("MQTT: Disconnected"));

    #ifdef _DEBUG
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
    #endif
    Serial.println(F("----------------------------------------------"));
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

    // static uint8_t h=t=0, nsize=sizeof(_msgQueue)/sizeof(_msgQueue[0]);

    // // copies at most size-1 non-null characters from src to dest and adds a null terminator
    // snprintf(_msgQueue[h][1], min(len+1, (size_t)sizeof(_msgQueue[h][1])), "%s", topic);
    // snprintf(_msgQueue[h][2], min(len+1, (size_t)sizeof(_msgQueue[h][2])), "%s", payload);
    // h = h++ % nsize;

    // -----------------------------------------------------------------------------------------
    // NOTE: onMessage callback is hadware interuption triggered, if the _cmdHandler is not done yet, can cause
    // buffer mess and crash!! so do not do heavy work in _cmdHandler!!

    // Reason to make a copy: https://github.com/marvinroger/async-mqtt-client/issues/42
    static char buffer[20]; // Command payload is usually short, 20 is enough.
    // copies at most size-1 non-null characters from src to dest and adds a null terminator
    snprintf(buffer, min(len+1, (size_t)sizeof(buffer)), "%s", payload);

    int size = strlen(cfg.module);
    _cmdHandler(&(topic[size]), buffer);
  });

  // set MQTT server
  mqttClient.setServer(cfg.mqttServer, cfg.mqttPort);
  
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(cfg.mqttUser, cfg.mqttPass);
}

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
  ArduinoOTA.setHostname(cfg.module);

  // No authentication by default
  ArduinoOTA.setPassword(cfg.otaPass);

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
  Serial.printf("OTA: %s @ %s\n", cfg.module, WiFi.localIP().toString().c_str());
}

String processor(const String& var){
  // Serial.println(var);
  // if(var == "GPIO_STATE"){
  //   if(digitalRead(ledPin)){
  //     ledState = "OFF";
  //   }
  //   else{
  //     ledState = "ON";
  //   }
  //   Serial.print(ledState);
  //   return ledState;
  // }
  return String("Haha");
}

// This will start a webserver allowing user to configure all settings via web.
// Two approach to access the webserver:
//     1. Connect AP WiFi SSID (usually named "ESP-XXXX"). Browse 192.168.4.1 
//        (Confirm the console prompt for the actual AP IP address). This
//        approach is good even the module is disconnected from family Wifi router.
//     2. Connect via Family network. Only works when the module is still connected
//        to the family Wifi router with a valid ip address.
// parameter: force means force open the portal (even there is no connection issue)
void EspClient::openConfigPortal(bool blocking) //char const *apName, char const *apPassword)
 {
  if(_portalOn)
  {
    #ifdef _DEBUG
      Serial.println(F("Portal already on."));
    #endif

    return;
  }

  _portalOn = true;         // this will enable the pollPortal call in the loop of main.cpp
  _portalSubmitted = false; // will be set to true when the configuration is done in _handleConfigPortal

  // use WIFI_AP_STA model such that WiFi reconnecting still going on
  // This is useful when the router is off. Once router is back on
  // the WiFi will reconnect and close the portal automatically.

  // construct ssid name
  char ssid[40];    // this will be used as AP WiFi SSID
  // int n;
  //   // short ipAddress[4]; // used to save the forw octets of the ipaddress
  //   // _extractIpAddress(WiFi.localIP().toString().c_str(), &ipAddress[0]);
  //   // return actual bytes written.  If not, compiler will complain with warning
  //   n = snprintf(ssid, sizeof(ssid), "ESP-%s-%d", cfg.module, ESP.getChipId()); //ipAddress[3]);

  int n = snprintf(ssid, sizeof(ssid), "ESP-%s-%d", cfg.module, ESP.getChipId());

  if(n == sizeof(ssid)) Serial.println("ssid might be trunckated!");

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, NULL); // apPassword);
  
  // Setting web server
  // refer: https://microcontrollerslab.com/esp8266-nodemcu-web-server-using-littlefs-flash-file-system/
  // _webServer.on("/",  std::bind(&EspClient::_handleWebRequest, this)); // bind is to make class function as static callback

    // Route for root / web page
  _webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, [](const String& var){
      Serial.println(var);
      if(var == "MODULE") return String(cfg.module);
      else if(var == "WIFI_SSID") return String(cfg.ssid);
      else if(var == "WIFI_PASS") return String(cfg.pass);
      else if(var == "IP") return String(cfg.ip);
      else if(var == "GATEWAY") return String(cfg.gateway);
      else if(var == "MQTT_SERVER") return String(cfg.mqttServer);
      else if(var == "MQTT_PORT") return String(cfg.mqttPort);
      else if(var == "MQTT_USER") return String(cfg.mqttUser);
      else if(var == "MQTT_PASS") return String(cfg.mqttPass);
      else if(var == "OTA_PASS") return String(cfg.otaPass);
      else if(var == "S0_NAME") return String(cfg.sensors[0].name);
      else if(var == "S0_TYPE") return String(cfg.sensors[0].type);
      else if(var == "S0_PIN0") return String(cfg.nameByPin(cfg.sensors[0].pin0));
      else if(var == "S0_PIN1") return String(cfg.nameByPin(cfg.sensors[0].pin1));
      else if(var == "S0_PIN2") return String(cfg.nameByPin(cfg.sensors[0].pin2));
      else if(var == "S1_NAME") return String(cfg.sensors[1].name);
      else if(var == "S1_TYPE") return String(cfg.sensors[1].type);
      else if(var == "S1_PIN0") return String(cfg.nameByPin(cfg.sensors[1].pin0));
      else if(var == "S1_PIN1") return String(cfg.nameByPin(cfg.sensors[1].pin1));
      else if(var == "S1_PIN2") return String(cfg.nameByPin(cfg.sensors[1].pin2));
      else if(var == "S2_NAME") return String(cfg.sensors[2].name);
      else if(var == "S2_TYPE") return String(cfg.sensors[2].type);
      else if(var == "S2_PIN0") return String(cfg.nameByPin(cfg.sensors[2].pin0));
      else if(var == "S2_PIN1") return String(cfg.nameByPin(cfg.sensors[2].pin1));
      else if(var == "S2_PIN2") return String(cfg.nameByPin(cfg.sensors[2].pin2));
      else return String();
    });
  });

  // Route to load style.css file
  _webServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  _webServer.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
    #ifdef _DEBUG
      Serial.println("------------------------");
      Serial.println("Config submission received");
      // Serial.println(request->contentType());
      Serial.println("------------------------");
    #endif

    String msg;
    int nparams = request->params();
    // Serial.printf("%d params sent in\n", nparams);
    for (int i = 0; i < nparams; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isPost())
      {
          Serial.printf("%s: %s \n", p->name().c_str(), p->value().c_str());

          if (p->name() == "module") p->value().toCharArray(cfg.module, sizeof(cfg.module)); //copy String over char array
          if (p->name() == "ssid") p->value().toCharArray(cfg.ssid, sizeof(cfg.ssid));
          if (p->name() == "pass") p->value().toCharArray(cfg.pass, sizeof(cfg.pass));
          if (p->name() == "ip") p->value().toCharArray(cfg.ip, sizeof(cfg.ip));
          if (p->name() == "gateway") p->value().toCharArray(cfg.gateway, sizeof(cfg.gateway));

          if (p->name() == "mqttServer") p->value().toCharArray(cfg.mqttServer, sizeof(cfg.mqttServer));
          if (p->name() == "mqttPort") cfg.mqttPort = atoi(p->value().c_str());
          if (p->name() == "mqttUser") p->value().toCharArray(cfg.mqttUser, sizeof(cfg.mqttUser));
          if (p->name() == "mqttPass") p->value().toCharArray(cfg.mqttPass, sizeof(cfg.mqttPass));
          if (p->name() == "otaPass") p->value().toCharArray(cfg.otaPass, sizeof(cfg.otaPass));

          if (p->name() == "s0_name") p->value().toCharArray(cfg.sensors[0].name, sizeof(cfg.sensors[0].name));
          if (p->name() == "s0_type") p->value().toCharArray(cfg.sensors[0].type, sizeof(cfg.sensors[0].type));
          if (p->name() == "s0_pin0") cfg.sensors[0].pin0 = cfg.pinByName(p->value().c_str());
          if (p->name() == "s0_pin1") cfg.sensors[0].pin1 = cfg.pinByName(p->value().c_str());
          if (p->name() == "s0_pin2") cfg.sensors[0].pin2 = cfg.pinByName(p->value().c_str());

          if (p->name() == "s1_name") p->value().toCharArray(cfg.sensors[1].name, sizeof(cfg.sensors[1].name));
          if (p->name() == "s1_type") p->value().toCharArray(cfg.sensors[1].type, sizeof(cfg.sensors[1].type));
          if (p->name() == "s1_pin0") cfg.sensors[1].pin0 = cfg.pinByName(p->value().c_str());
          if (p->name() == "s1_pin1") cfg.sensors[1].pin1 = cfg.pinByName(p->value().c_str());
          if (p->name() == "s1_pin2") cfg.sensors[1].pin2 = cfg.pinByName(p->value().c_str());

          if (p->name() == "s2_name") p->value().toCharArray(cfg.sensors[2].name, sizeof(cfg.sensors[2].name));
          if (p->name() == "s2_type") p->value().toCharArray(cfg.sensors[2].type, sizeof(cfg.sensors[2].type));
          if (p->name() == "s2_pin0") cfg.sensors[2].pin0 = cfg.pinByName(p->value().c_str());
          if (p->name() == "s2_pin1") cfg.sensors[2].pin1 = cfg.pinByName(p->value().c_str());
          if (p->name() == "s2_pin2") cfg.sensors[2].pin2 = cfg.pinByName(p->value().c_str());
      }
      // else if (p->isFile())
      // {
      //     Serial.printf("_FILE[%s]: %s, size: %u", p->name().c_str(), p->value().c_str(), p->size());
      // }
      // else
      // {
      //     Serial.printf("_GET[%s]: %s", p->name().c_str(), p->value().c_str());
      // }
    }
    
    // String msg;
    // if(request->hasParam("module")) {
    //   msg = request->getParam("module")->value(); msg.toCharArray(cfg.module, sizeof(cfg.module)); //copy String over char array
    // }
    
    cfg.saveConfig();

    // send back the success web page.
    request->send(LittleFS, "/success.html");
  });

  AsyncElegantOTA.begin(&_webServer);  // Start ElegantOTA right before webserver start

  _webServer.begin(); // start web server

  Serial.println(F("---------------------------------------------------"));
  Serial.println(F("Config portal on:"));
  if(_wifiConnected) Serial.printf("Browse http://%s/ or %s for portal, or\n", cfg.module, WiFi.localIP().toString().c_str());
  Serial.printf("Connect to Wifi \"%s\" and browse http://%s/ or %s.\n", ssid, cfg.module, WiFi.softAPIP().toString().c_str());
  Serial.println(F("---------------------------------------------------"));

  // set timmer to close portal in 120s
  StensTimer *pTimer = StensTimer::getInstance(); 
  pTimer->setTimer(this, ACT_CLOSE_PORTAL, PORTAL_TIMEOUT);

  if(blocking)
  {
    // blocking mode, open untill configuration done!
    Serial.println(F("Portal open on blocking mode."));
    while(!_portalSubmitted)
    {
      // consider just use loop(); or rethink if blocking is really useful?
      // _webServer.handleClient();
      pTimer->run(); // on blocking mode, need to handle timer logic to check timeout event!
      delay(1000);
    } 
    closeConfigPortal();
  }
}

void EspClient::closeConfigPortal()
{
  if(!_portalOn)
  {
    #ifdef _DEBUG
      Serial.println(F("Portal already closed!"));
    #endif

    return;
  }

  // Wifi may be connected if portal is opened intentionaly
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  // _webServer.stop();
  _webServer.end();
  _portalOn = false; // reset flag

  Serial.println(F("Config portal closed"));

  return;
}

void EspClient::timerCallback(Timer* timer)
{
    /* check if the timer is one we expect */
    int action = timer->getAction();

    switch(action)
    {
      case ACT_WIFI_CONNECT_TIMEOUT:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_WIFI_CONNECT_TIMEOUT"));
        #endif

        if(_wifiConnected) Serial.println(F("WIFI: connected already!"));
        else
        {
          Serial.println(F("WIFI: not connected, open portal."));
          openConfigPortal(true); // open portal in blocking mode

          // reconnect? Somethime WiFi AutoReconnect does not work!
          // Serial.println("WiFi: Reconnect!");
          // WiFi.disconnect(true);
          // WiFi.reconnect();
          // cfg.printConfig();
        }
        break;

      case ACT_MQTT_RECONNECT:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_MQTT_RECONNECT"));
        #endif
        
        if(_mqttConnected)
        {
          // TODO: The following (delete timer) code crash sometime. check it!
          // MQTT connected already, delete reconnect timer
          // StensTimer *pTimer = StensTimer::getInstance();
          // pTimer->deleteTimer(timer);

          #ifdef _DEBUG
            Serial.println(F("MQTT: connected already."));
          #endif

          if(_pTimerReconnect != NULL)
          {
            pTimer->deleteTimer(_pTimerReconnect);
            _pTimerReconnect = NULL;
          }

        }else
        {
          int repetitions = timer->getRepetitions();
          if(repetitions > 1)
          {
            // not reaching max number of reconnect try, reconnect again
            _connectToMqttBroker();
            Serial.print(F("Repetitions: "));
            Serial.println(repetitions);
          }else
          {
            // reach last one, open config portal
            openConfigPortal();
          }
        }
        break;

      case ACT_MQTT_SUBSCRIBE:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_MQTT_SUBSCRIBE"));
        #endif

        // check mqtt connection before subscribe. Sometime it disconnect
        if(_mqttConnected && cfg.module[0] != '\0')
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

      case ACT_CLOSE_PORTAL:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_CLOSE_PORTAL"));
        #endif

        Serial.println(F("----------------------------"));
        Serial.println(F("Portal closed on timeout"));
        Serial.println(F("----------------------------"));
        
        closeConfigPortal();
        if(!isConnected()) _restart(RsCode::RS_DISCONNECT);
        break;

      case ACT_HEARTBEAT:
      {
        // #ifdef _DEBUG
        //   Serial.println(F("TIMER: ACT_HEARTBEAT"));
        // #endif

        // memory is allocated in heap, supposed to be deleted after use, but for ESP8266, it is fine
        static String mqtt_pub_heartbeat = String(cfg.module) + MQTT_PUB_HEARTBEAT;
        // static char *mqtt_pub_heartbeat = strcat(cfg.module, MQTT_PUB_HEARTBEAT);
        // Serial.println(mqtt_pub_heartbeat);

        // send heartbeat message
        mqttClient.publish(mqtt_pub_heartbeat.c_str(), 0, true);

        _blink();
        break;
      }
        
      case ACT_MEASURE:
        #ifdef _DEBUG
          Serial.println(F("TIMER: ACT_MEASURE"));
        #endif

        if (_autoMode) _measure();
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
  for(int i=0; i<MAX_SENSORS; i++)
  {
    if(_sensors[i] != NULL && isConnected())
      _sensors[i]->sendMeasure();
  }
}

void EspClient::_enableSensor(const char* name, bool enable)
{
  for(int i=0; i<MAX_SENSORS; i++)
  {
    if(_sensors[i] != NULL && strcmp(name, _sensors[i]->name) == 0)
    {
      Serial.print(name);
      Serial.print(F(" sensor enabled: "));
      Serial.println(enable);

      _sensors[i]->enable(enable);
    }
  }
}

void EspClient::_cmdHandler(const char* topic, const char* payload)
{
  #ifdef _DEBUG
    Serial.println(topic);
    Serial.println(payload);
  #endif

  if(strcmp(topic, CMD_OPEN_PORTAL) == 0)
  {
    #ifdef _DEBUG
      Serial.println(F("Open portal in _cmdHandler()"));
    #endif

    EspClient::instance().openConfigPortal();
  }
  else if(strcmp(topic, CMD_SSR_FILTER) == 0)
  {
    int filter = atoi(payload);

    for(int i=0; i<MAX_SENSORS; i++)
    {
      if(_sensors[i] != NULL) _sensors[i]->setFilter((FilterType)filter);
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
    pTimer->setTimer(this, ACT_MEASURE, 100);
  }
  else if(strcmp(topic, CMD_INTERVAL) == 0)
  {
    // reset the interval of the timer
    _timer_sensor->setDelay(atoi(payload)*1000);

  }else if(strcmp(topic, CMD_RESTART) == 0)
  {
    _restart();
  }
  else if(strstr(topic, "/cmd/on/") != NULL)
  {
      // fint he last occurrence of '/'
      char *ptr = strrchr(topic, '/');

      if(ptr != NULL)
      {
        Serial.print("Sensor: "); Serial.println(ptr+1);
        _enableSensor(ptr+1, strcmp(payload, "true") == 0);
      }
      
  //     char *pch = strstr(topic, "/sensor/");
  //     const char* sensorName = pch + sizeof("/sensor/");
  //     Serial.print("Sensor: "); Serial.println(sensorName);
  //     _enableSensor(sensorName, strcmp(payload, "true") == 0);

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
  
  // disconnect Wifi
  WiFi.disconnect(true);  // Force disconnect.
  WiFi.mode(WIFI_OFF);    // Turen off wifi radio
  WiFi.persistent(false); // Avoid to store Wifi configuration in Flash

  //elete all sensor objects in _sensors[]
  for(int i=0; i<MAX_SENSORS; i++)
  {
    if(_sensors[i] != NULL) delete _sensors[i];
  }

  #ifdef ESP8266
    ESP.reset();
  #elif defined(ESP32)
    ESP.restart();
  #else
      #error Platform not supported
  #endif
}


/*
https://www.includehelp.com/c-programs/format-ip-address-octets.aspx
Arguments : 
1) sourceString - String pointer that contains ip address
2) ipAddress - Target variable short type array pointer that will store ip address octets
*/
/*
void EspClient::_extractIpAddress(const char* sourceString, short* ipAddress)
{
    short len = 0;
    char oct[4] = { 0 }, buf[5];
    unsigned cnt = 0, cnt1 = 0;

    len = strlen(sourceString);
    for (int i = 0; i < len; i++) {
        if (sourceString[i] != '.') {
            buf[cnt++] = sourceString[i];
        }
        if (sourceString[i] == '.' || i == len - 1) {
            buf[cnt] = '\0';
            cnt = 0;
            oct[cnt1++] = atoi(buf);
        }
    }
    ipAddress[0] = oct[0];
    ipAddress[1] = oct[1];
    ipAddress[2] = oct[2];
    ipAddress[3] = oct[3];
}
*/