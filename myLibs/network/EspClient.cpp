
#include "EspClient.hpp"

#include <ArduinoOTA.h>

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

  if(!cfg.valid)
  {
    #ifdef _DEBUG
      Serial.println(F("Open portal in setup()"));
    #endif

    // open portal at blocking mode to ensure the following setup is executed later.
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

    String mqtt_pub_sensor = String(cfg.module) + "/sensor/" + pSensor->name;
    pSensor->setMqtt(&mqttClient, mqtt_pub_sensor.c_str(), 0, false);
    Serial.print("Sensor pub: "); Serial.println(mqtt_pub_sensor);

    _sensors[i] = pSensor;
  }
}

void EspClient::loop()
{
  // let StensTimer do it's magic every time loop() is executed
  pTimer->run();

  // A loop to enable the _webServer process client request 
  if(_portalOn) _webServer.handleClient();

  // Wifi handling
  if(!_wifiConnected)
  {
    // Do nothing else as ESP WiFi module take care of reconnecting (configured in _connectToWifi())
    return;
  }else
  {
    // #ifdef ESP8266
    //   MDNS.update(); // We need to do this only for ESP8266
    // #endif

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

  cfg.print(true);
  // cfg.print();

  WiFi.mode(WIFI_STA);

  // check the reason of the the two line below: https://github.com/esp8266/Arduino/issues/2186
  // WiFi.disconnect(true);  // Delete old config
  // WiFi.persistent(false); // Avoid to store Wifi configuration in Flash   

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
      IPAddress gateway(192,168,0,1);   //IP Address of your WiFi Router (Gateway)
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
    WiFi.setAutoConnect(false);  // (Disable it) Configure module to automatically connect on power on to the last used access point.
    WiFi.setAutoReconnect(true); // Set whether module will attempt to reconnect to an access point in case it is disconnected.
    WiFi.persistent(true);       // persistent is true by default
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

    // Command payload is usually short, 20 is enough.
    // Reason to make a copy: https://github.com/marvinroger/async-mqtt-client/issues/42
    static char buffer[20];
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

// void EspClient::setCommandHandler(CommandHandler cmdHandler, const char* cmdTopic)
// {
//   // set up cmdHandler and cmdTopic
//   // do this before anything else, as turn on portal cmd will be send to cmdHandler if there is connection issue
//   _cmdHandler = cmdHandler;
//   strncpy(_cmdTopic, cmdTopic, sizeof(_cmdTopic));
// }

// Do this after Wifi is connected!!
// This is called in WiFi connected callback set in _setupWifi()
void EspClient::_setupOTA()
{
  #ifdef _DEBUG
    Serial.println("_setupOTA()");
  #endif

  // -- OTA is using mDNS, so no need to begin mDNS specifically
  // Start mDNS
  // if(MDNS.begin(cfg.module)) Serial.println("MDNS: started");
  // MDNS.addService("http", "tcp", 80);

  // Port defaults to 8266
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
  // Serial.println("OTA: setup");
  Serial.printf("OTA: %s @ %s\n", cfg.module, WiFi.localIP().toString().c_str());
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
  // if(_wifiConnected)
  // {
  //   // short ipAddress[4]; // used to save the forw octets of the ipaddress
  //   // _extractIpAddress(WiFi.localIP().toString().c_str(), &ipAddress[0]);

  //   // return actual bytes written.  If not, compiler will complain with warning
  //   n = snprintf(ssid, sizeof(ssid), "ESP-%s-%d", cfg.module, ESP.getChipId()); //ipAddress[3]);
  // }else
  // {
  //   // return actual bytes written.  If not, compiler will complain with warning
  //   n = snprintf(ssid, sizeof(ssid), "ESP-%s", cfg.module);
  // }

  int n = snprintf(ssid, sizeof(ssid), "ESP-%s-%d", cfg.module, ESP.getChipId());

  if(n == sizeof(ssid)) Serial.println("ssid might be trunckated!");

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, NULL); // apPassword);
  
  _webServer.on("/",  std::bind(&EspClient::_handleWebRequest, this)); // bind is to make class function as static callback
  _webServer.begin();

  Serial.println(F("---------------------------------------------------"));
  Serial.println(F("Configuration portal on"));
  if(_wifiConnected)
    Serial.printf("Browse http://%s/ or %s for portal, or\n", cfg.module, WiFi.localIP().toString().c_str());
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
      _webServer.handleClient();
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

  // The portal may be forced to open when WiFi is still on (e.g., cmd sent from Node-Red dashboard)
  Serial.println(F("Close AP."));
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  // // reconnect WiFi if needed
  // if(!WiFi.isConnected())  WiFi.begin(cfg.ssid, cfg.pass);

  // // reconnect MQTT broker if needed
  // if(!mqttClient.connected()) _connectToMqtt();
  
  _webServer.stop();
  
  // reset flag
  _portalOn = false;

  Serial.println(F("Configuration portal closed."));

  return;
}

void EspClient::_handleWebRequest()
{
  if (_webServer.method() == HTTP_POST)
  {
    // NOTE: no need to close portal specifically, it will timeout to close by timer handling
    const char htmlSuccess[] = "<html lang='en'><head><meta charset='utf-8'><style>body{font-family:'Segoe UI','Roboto';color:#212529;background-color:#f5f5f5;}h1,p{text-align: center}</style></head><body><main class='form-signin'><h1>Success!</h1><br/><p>Your settings have been saved successfully!<br/>Device is starting...</p></main></body></html>";
    _webServer.send(200, "text/html", htmlSuccess);

    // save configuration received
    if(String("configuration") == _webServer.arg("type"))
    {
      strncpy(cfg.module, _webServer.arg("module").c_str(), sizeof(cfg.module));
      strncpy(cfg.ssid, _webServer.arg("ssid").c_str(), sizeof(cfg.ssid));
      strncpy(cfg.pass, _webServer.arg("password").c_str(), sizeof(cfg.pass));
      strncpy(cfg.ip, _webServer.arg("ip").c_str(), sizeof(cfg.ip));

      strncpy(cfg.mqttServer, _webServer.arg("mqttServer").c_str(), sizeof(cfg.mqttServer));
      cfg.mqttPort = atoi(_webServer.arg("mqttPort").c_str());
      strncpy(cfg.mqttUser, _webServer.arg("mqttUser").c_str(), sizeof(cfg.mqttUser));
      strncpy(cfg.mqttPass, _webServer.arg("mqttPass").c_str(), sizeof(cfg.mqttPass));
      strncpy(cfg.otaPass, _webServer.arg("otaPass").c_str(), sizeof(cfg.otaPass));

      cfg.saveConfig();
      
      _portalSubmitted = true;

      #ifdef _DEBUG
        Serial.println(F("PORTAL: submitted"));
      #endif
    }else if(String("restart") == _webServer.arg("type"))
    {
      #ifdef _DEBUG
        Serial.println(F("PORTAL: Restart"));
      #endif

      _restart(RsCode::RS_CFG_CHANGE);
    }
    
  } else {

    // bool hasSettings = getSettings();

    // String template = "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='{SSID}' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' value='{PASS}' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttServer' type='text' value='{MQTTSERVER}' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='{MQTTPORT}' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='{MQTTUSER}' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' value='{MQTTPASS}' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='module' type='text' value='{MODULE}' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' value='{OTAPASS}' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>";

    // template.replace("{MODULE}",   cfg.module);
    // template.replace("{SSID}",      cfg.ssid);
    // template.replace("{PASS}",      cfg.pass);
    // template.replace("{MQTTSERVER}",  cfg.mqttServer);
    // template.replace("{MQTTPORT}",  cfg.mqttPort);
    // template.replace("{MQTTUSER}",  cfg.mqttUser);
    // template.replace("{MQTTPASS}",  cfg.mqttPass);
    // template.replace("{OTAPASS}",   cfg.otaPass);

    // _webServer.send(200, "text/html", template);

    // if(hasSettings) // reset the form to previous configures
    // {
    //   // password mode (password hidden)
    //   //_webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='" + String(cfg.ssid) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' type='password' value='" + String(cfg.pass) + "' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttServer' type='text' value='" + String(cfg.mqttServer) + "' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='" + String(cfg.mqttPort) + "' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='" + String(cfg.mqttUser) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' type='password' value='" + String(cfg.mqttPass) + "' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='module' type='text' value='" + String(cfg.module) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' type='password' value='" + String(cfg.otaPass) + "' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");

    //   // non-password mode (show password)
    _webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><input type='hidden' name='type' value='configuration'><h1 class=''>Device Setup</h1><p style='color: red;'>" + String(_portalReason) + "</p><div class='form-floating'>Module:<input class='form-control' name='module' type='text' value='" + String(cfg.module) + "' /></div><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='" + String(cfg.ssid) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' value='" + String(cfg.pass) + "' /></div><div class='form-floating'>Static IP:<input class='form-control' name='ip' value='" + String(cfg.ip) + "' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttServer' type='text' value='" + String(cfg.mqttServer) + "' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='" + String(cfg.mqttPort) + "' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='" + String(cfg.mqttUser) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' value='" + String(cfg.mqttPass) + "' /></div><strong>OTA:</strong><div class='form-floating'>Pass:<input class='form-control' name='otaPass' value='" + String(cfg.otaPass) + "' /></div><br/><button type='submit'>Save</button></form><hr><form action='/' method='post'><input type='hidden' name='type' value='restart'><button type='submit'>Restart</button></form></main></body></html>");
    // }else
    // {
    //   _webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color:red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' type='pass' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttServer' type='text' value='raspberrypi.local' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='1883' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' type='pass' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='module' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' type='pass' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");
    // }
  }
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
          // cfg.print();
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
    if(strcmp(name, _sensors[i]->name) == 0)
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