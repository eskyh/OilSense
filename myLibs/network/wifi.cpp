#include "wifi.hpp"
// <time.h> and <WiFiUdp.h> not needed. already included by core.
#include <EEPROM.h>

// void myWifi::syncTimeNTP()
// {
//   // // When time() is called for the first time, the NTP sync is started immediately. It then takes 15 to 200ms or more to complete.
//   configTime(TZ_America_New_York, "pool.ntp.org"); // daytime saying will be adjusted by SDK automatically.
//   time(NULL); // fist call to start sync
// }

// void myWifi::waitSyncNTP()
// {
//   #ifdef _DEBUG
//     unsigned long t0 = millis();
//   #endif

//   Serial.println("Sync time over NTP...");
//   while(time(NULL) < NTP_MIN_VALID_EPOCH) {
//     Serial.print(F("."));
//     delay(1000);
//   }
//   Serial.println(F("NTP synced."));

//   #ifdef _DEBUG
//     unsigned long t1 = millis() - t0;
//     Serial.println("NTP time first synch took " + String(t1) + "ms");
//   #endif
// }

void myWifi::_setupWifiListener()
{
  // The next two lines create handlers that will allow both the MQTT broker and Wi-Fi connection
  // to reconnect, in case the connection is lost.
  if(_wifiConnectHandler == nullptr)
    _wifiConnectHandler = WiFi.onStationModeGotIP([] (const WiFiEventStationModeGotIP& event) {
      Serial.print(F("\nConnected to Wi-Fi. IP: "));
      Serial.println(WiFi.localIP().toString().c_str());
      // syncTimeNTP();
      setUpMQTT();
      _connectToMqtt(); //or mqttReconnectTimer.once(1, _connectToMqtt);
      setUpOTA();
    });

  if(_wifiDisconnectHandler == nullptr)
    _wifiDisconnectHandler = WiFi.onStationModeDisconnected([] (const WiFiEventStationModeDisconnected& event)
      {
        Serial.println(F("Disconnected from Wi-Fi."));
        // mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        // if(!_portalOn) wifiReconnectTimer.once(2, autoConnect);
        
        // The WiFi will auto connect if disconnected. Sometime, it takes about 50s to be reconnected
        // Call WiFi.begin() in _connectToWifi() may reset the time and cause not been connected forever (Not sure though!).
        // wifiReconnectTimer.once(2, _connectToWifi);
      });
}

void myWifi::_connectToWifi()
{
  Serial.println(F("Connecting WiFi..."));
  // check the reason of the the two line below: https://github.com/esp8266/Arduino/issues/2186
  // WiFi.disconnect(true);  // Delete old config
  // WiFi.persistent(false); // Avoid to store Wifi configuration in Flash   

  // do not change the mode (can be WIFI_STA or WIFI_AP_STA), just reconnect

  // https://arduino.stackexchange.com/questions/78604/esp8266-wifi-not-connecting-to-internet-without-static-ip
  // Configure static IP will work for latest ESP8266 lib v3.0.2 where DHCP has problem
  // To make DHCP work, it need revert back to v2.5.2
  IPAddress ip;   //ESP static ip
  ip.fromString(settings.ip);
  IPAddress subnet(255,255,255,0);  //Subnet mask
  IPAddress gateway(192,168,0,1);   //IP Address of your WiFi Router (Gateway)
  IPAddress dns(0, 0, 0, 0);        //DNS 167.206.10.178
  WiFi.config(ip, subnet, gateway, dns);
  
  WiFi.begin(settings.ssid, settings.pass);

  // auto reconnect when lost WiFi connection
  // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html#setautoconnect
  // https://randomnerdtutorials.com/solved-reconnect-esp8266-nodemcu-to-wifi/
  WiFi.setAutoConnect(true);   // Configure module to automatically connect on power on to the last used access point.
  WiFi.setAutoReconnect(true); // Set whether module will attempt to reconnect to an access point in case it is disconnected.
  WiFi.persistent(true);
}

void myWifi::autoConnect(CommandHandler cmdHandler, const char* cmdTopic)
{
  // set up cmdHandler and cmdTopic
  // do this before anything else, as turn on portal cmd will be send to cmdHandler if there is connection issue
  _cmdHandler = cmdHandler;
  strncpy(_cmdTopic, cmdTopic, sizeof(_cmdTopic));

  // set up wifi listener if needed
  _setupWifiListener();

  // read settings from EEPROM. CRC is checked to see if the saved data are valid
  if(getSettings())
  {
    _connectToWifi();
    //uncomment below to trigger startConfigPortal
    // WiFi.disconnect(true);

    #ifdef _DEBUG
      Serial.print("MAC: ");
      Serial.println(WiFi.macAddress());

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

    byte tries = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      if (tries++ > 130)
      {
        Serial.println(F("Can't connect WiFi. Start portal..."));
        sendCmdOpenPortal("Wifi connect failure!");
        break;
      }

      #ifdef _DEBUG
        Serial.print(WiFi.status());
      #endif

      delay(2000);
    }
  }else
  {
    // no settings saved in EEPROM, therefore initial usage
    startConfigPortal(false);
  }

  // if(WiFi.isConnected()) waitSyncNTP();
}

// Sometime can not run the startConfigPortal in an event handler or timer
// Use this function to inform the main loop to open portal instead!
void myWifi::sendCmdOpenPortal(const char* reason)
{
  if(_cmdHandler!= NULL)
  {
    _cmdHandler("cmdOpenPortal", NULL);
    strncpy(_portalReason, reason, sizeof(_portalReason));      
  }else
  {
    Serial.println(F("cmdHandler is not set!"));
  }
}

// This will start a webserver allowing user to configure all settings via web.
// Two approach to access the webserver:
//     1. Connect AP WiFi SSID (usually named "ESP-XXXX"). Browse 192.168.4.1 
//        (Confirm the console prompt for the actual AP IP address). This
//        approach is good even the module is disconnected from family Wifi router.
//     2. Connect via Family network. Only works when the module is still connected
//        to the family Wifi router with a valid ip address.
void myWifi::startConfigPortal(bool force) //char const *apName, char const *apPassword)
 {
   Serial.print("Force portal on: ");
   Serial.println(force?F("True"):F("False"));

  _portalOn = true;         // this will enable the pollPortal call in the loop of main.cpp
  _portalSubmitted = false; // will be set to true when the configuration is done in _handlePortal
  _forcePortal = force;

  // recorde start time of the force started portal. Used for timeout
  if(_forcePortal) _timeStartPortal = millis();

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

  // construct ssid name
  char ssid[40];    // this will be used as AP WiFi SSID
  short ipAddress[4]; // used to save the forw octets of the ipaddress
  _extractIpAddress(settings.ip, &ipAddress[0]);
  // return actual bytes written.  If not, compiler will complain with warning
  int n = snprintf(ssid, sizeof(ssid), "ESP-%s-%d", settings.otaHost, ipAddress[3]);
  if(n == sizeof(ssid)) Serial.println("ssid might be trunckated!");  

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, NULL); // apPassword);
  _webServer.on("/",  _handlePortal);
  _webServer.begin();

  Serial.println(F("**Configuration portal on**"));
  // Serial.print(ssid);
  // Serial.print(F(", IP:"));
  Serial.printf("Browse %s for portal, or connect to Wifi \"%s\" and browse %s instead.\n", settings.ip, ssid, WiFi.softAPIP().toString().c_str());
}

void myWifi::closeConfigPortal()
{
    Serial.println(F("Configuration portal completed."));

    // The portal may be forced to open when WiFi is still on (e.g., cmd sent from Node-Red dashboard)
    Serial.println(F("Close AP."));
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    // reconnect WiFi if needed
    if(!WiFi.isConnected())  WiFi.begin(settings.ssid, settings.pass);

    // reconnect MQTT broker if needed
    if(!mqttClient.connected()) _connectToMqtt();
    
    _webServer.stop();
    
    // reset flag
    _forcePortal = false;
    _portalOn = false;
    return;
}

// A loop to enable the _webServer process client request and will invoke the
// callback function _handlePortal set above. The loop will be running untill
// the 'connect' is set to true within _handlePortal when configuration is done.
//---------------------------------------------------------------------------
// Stop condition: 1) Configuration form submitted
//                 2) Portal non-force open, WiFi and MQTT are reconnected
void myWifi::pollPortal()
{
  if(!_portalOn) return;

  // handle config portal form POST. When it is done, set _portalSubmitted false.
  _webServer.handleClient();

  if (_portalSubmitted) // Form submitted and handled by _webServer @ _handlePortal()
  {
    delay(1000); // leave enough time for _handlePortal _webServer.send() to complete before closing softAP mode.
    Serial.println(F("Configuration portal completed."));
    closeConfigPortal();
    return;
  }

  // Forced portal will be time out in 2min
  if(_forcePortal)
  {
    // time out in 120s (2min)
    if(millis() - _timeStartPortal > 120e3)
    {
      // if it is forced portal, the WiFi should be working
      // no need to reconnect and such
      Serial.println(F("Time out. Exit portal."));
      closeConfigPortal();
      return;
    }
  }else // Non-forced portal
  {
    // return if WiFi is connected and mqtt is connected
    // this can happen when wifi is still reconnect periodically while the portal is on
    if(WiFi.isConnected() && mqttClient.connected())
    {
      // both WiFi and MQTT are connected, close the portal
      Serial.println(F("WiFi & MQTT reconnected. Exit portal."));
      closeConfigPortal();
    }
  }
}

void myWifi::_handlePortal()
{
  if (_webServer.method() == HTTP_POST)
  {
    Serial.println("POST received.");
  
    _webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Success!</h1> <br/> <p>Your settings have been saved successfully!<br />Device is starting...</p></main></body></html>");

    // save configuration received
    
    if(String("configuration") == _webServer.arg("type"))
    {
      #ifdef _DEBUG
        Serial.println(F("Portal configuration submitted."));
      #endif

      strncpy(settings.ssid, _webServer.arg("ssid").c_str(), sizeof(settings.ssid));
      strncpy(settings.pass, _webServer.arg("password").c_str(), sizeof(settings.pass));
      strncpy(settings.ip, _webServer.arg("ip").c_str(), sizeof(settings.ip));

      strncpy(settings.mqttHost, _webServer.arg("mqttHost").c_str(), sizeof(settings.mqttHost));
      settings.mqttPort = atoi(_webServer.arg("mqttPort").c_str());
      strncpy(settings.mqttUser, _webServer.arg("mqttUser").c_str(), sizeof(settings.mqttUser));
      strncpy(settings.mqttPass, _webServer.arg("mqttPass").c_str(), sizeof(settings.mqttPass));
      strncpy(settings.otaHost, _webServer.arg("otaHost").c_str(), sizeof(settings.otaHost));
      strncpy(settings.otaPass, _webServer.arg("otaPass").c_str(), sizeof(settings.otaPass));

      setSettings();
      
      _portalSubmitted = true;

    }else if(String("restart") == _webServer.arg("type"))
    {
      #ifdef _DEBUG
        Serial.println(F("Portal restart clicked."));
      #endif

      ESP.restart();
    }
    
  } else {

    // bool hasSettings = getSettings();

    // String template = "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='{SSID}' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' value='{PASS}' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='{MQTTHOST}' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='{MQTTPORT}' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='{MQTTUSER}' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' value='{MQTTPASS}' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' value='{OTAHOST}' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' value='{OTAPASS}' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>";

    // template.replace("{SSID}",      settings.ssid);
    // template.replace("{PASS}",      settings.pass);
    // template.replace("{MQTTHOST}",  settings.mqttHost);
    // template.replace("{MQTTPORT}",  settings.mqttPort);
    // template.replace("{MQTTUSER}",  settings.mqttUser);
    // template.replace("{MQTTPASS}",  settings.mqttPass);
    // template.replace("{OTAHOST}",   settings.otaHost);
    // template.replace("{OTAPASS}",   settings.otaPass);

    // _webServer.send(200, "text/html", template);

    // if(hasSettings) // reset the form to previous configures
    // {
    //   // password mode (password hidden)
    //   //_webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='" + String(settings.ssid) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' type='password' value='" + String(settings.pass) + "' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='" + String(settings.mqttHost) + "' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='" + String(settings.mqttPort) + "' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='" + String(settings.mqttUser) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' type='password' value='" + String(settings.mqttPass) + "' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' value='" + String(settings.otaHost) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' type='password' value='" + String(settings.otaPass) + "' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");

    //   // non-password mode (show password)
    _webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><p>&nbsp;</p><main class='form-signin'><form action='/' method='post'><input type='hidden' name='type' value='configuration'><h1 class=''>Device Setup</h1><br /><p style='color: red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' value='" + String(settings.ssid) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' value='" + String(settings.pass) + "' /></div><div class='form-floating'>Static IP:<input class='form-control' name='ip' value='" + String(settings.ip) + "' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='" + String(settings.mqttHost) + "' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='" + String(settings.mqttPort) + "' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' value='" + String(settings.mqttUser) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' value='" + String(settings.mqttPass) + "' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' value='" + String(settings.otaHost) + "' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' value='" + String(settings.otaPass) + "' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form><hr><form action='/' method='post'><input type='hidden' name='type' value='restart'><button type='submit'>Restart</button></form></main></body></html>");
    // }else
    // {
    //   _webServer.send(200, "text/html", "<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title> <style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{cursor: pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1{text-align: center}</style> </head> <body><main class='form-signin'><form action='/' method='post'><h1 class=''>Device Setup</h1><br /><p style='color:red;'>" + String(_portalReason) + "</p><div class='form-floating'>SSID:<input class='form-control' name='ssid' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='password' type='pass' /></div><strong>MQTT:</strong><div class='form-floating'>Host:<input class='form-control' name='mqttHost' type='text' value='raspberrypi.local' /></div><div class='form-floating'>Port:<input class='form-control' name='mqttPort' type='text' value='1883' /></div><div class='form-floating'>User:<input class='form-control' name='mqttUser' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='mqttPass' type='pass' /></div><strong>OTA:</strong><div class='form-floating'>Host:<input class='form-control' name='otaHost' type='text' /></div><div class='form-floating'>Pass:<input class='form-control' name='otaPass' type='pass' /></div><br /><br /><button type='submit'>Save</button><p style='text-align: right;'>&nbsp;</p></form></main></body></html>");
    // }
  }
}

// use CRC to check EEPROM data
// https://community.particle.io/t/determine-empty-location-with-eeprom-get-function/18869/12
bool myWifi::getSettings()
{
  EEPROM.begin(sizeof(struct Settings));
  EEPROM.get(0, settings);

  uint32_t checksum = CRC32::calculate((uint8_t *)&settings, sizeof(settings)-sizeof(settings.CRC));
  bool crcMatched = settings.CRC == checksum;

  // if CRC check fail, the data retrieved to settings are garbage, reset it.
  if(!crcMatched)
  {
    settings.reset();
    Serial.println(F("Initialize EEPROM..."));
  }else
  {
    #ifdef _DEBUG
      Serial.println(F("\nCRC matched: "));
      settings.print();
    #endif
  }

  return crcMatched;
}

void myWifi::setSettings()
{
  settings.CRC = CRC32::calculate((uint8_t *)&settings, sizeof(settings)-sizeof(settings.CRC));

  #ifdef _DEBUG
    Serial.println("Set EEPROM:");
    settings.print();
  #endif
  
  EEPROM.begin(sizeof(struct Settings));
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void myWifi::_connectToMqtt()
{
  mqttClient.connect();
  Serial.print(F("Connecting to MQTT: "));
  Serial.println(settings.mqttHost);
}

void myWifi::_subscribeMqttCommand()
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
    mqttsubscribeTimer.once(1, _subscribeMqttCommand);

    // close portal if it is on
    if(_portalOn) closeConfigPortal();
  });

  mqttClient.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
    #ifdef _DEBUG
      Serial.print("Disconnected from MQTT, reason: ");
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

    // subscribe command topics. Do not call _connectToMqtt() directly in eventHandler.
    // Better call it with a schedule timer!! Also do not use delay() in eventhandler as well!
    // refer https://github.com/marvinroger/async-mqtt-client/issues/264
    if (WiFi.isConnected())
    {
      mqttReconnectTimer.once(2, _connectToMqtt);

      // open portal if tried to connect more than 10 times
      if(_nMqttReconnect++ > 10 && !_portalOn)
      {
        // Note: when Wifi is connected but fake, the waitSyncNTP() is taking the control
        // at the end of autoConnect(), such that the main loop does not have a chance to
        // start the portal with the following line of code!!
        sendCmdOpenPortal("MQTT broker connect failure!");
      }
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
      Serial.println(F("Publish received."));
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

/*
https://www.includehelp.com/c-programs/format-ip-address-octets.aspx
Arguments : 
1) sourceString - String pointer that contains ip address
2) ipAddress - Target variable short type array pointer that will store ip address octets
*/
void myWifi::_extractIpAddress(const char* sourceString, short* ipAddress)
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