#pragma once

// all includes are from Arduino Core, no external lib
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  // #include <TZ.h>
  // <time.h> and <WiFiUdp.h> not needed. already included by core.         
#elif defined(ESP32)
  #include <WiFi.h>         
  #include <WebServer.h>
  #include <time.h>
#else
    #error Platform not supported
#endif

#include <ArduinoOTA.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
// #include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager

#define NTP_MIN_VALID_EPOCH 1640995200  // 1/2/2022. Use https://www.epochconverter.com/

#include "Settings.hpp"

typedef std::function<void(const char* topic, const char* payload)> CommandHandler;

class myWifi
{
  public:
    // static member has to be defined with inline keyword
    inline static AsyncMqttClient mqttClient;

    static void autoConnect(CommandHandler cmdHandler, const char* cmdTopic);
    static void sendCmdOpenPortal(const char* reason);
    static void startConfigPortal(bool force=false); //char const *apName, char const *apPassword);
    static void closeConfigPortal();
    static void pollPortal();

  protected:
    // settings
    inline static Settings settings;
    static bool getSettings();
    static void setSettings();

    static void setUpMQTT();
    static void setUpOTA();
    
    // sync NTP time
    // static void syncTimeNTP();
    // static void waitSyncNTP();

    // https://stackoverflow.com/questions/9110487/undefined-reference-to-a-static-member
    // timer used to reconnect when disconnection happend
    inline static Ticker wifiReconnectTimer;
    inline static Ticker mqttReconnectTimer;
    inline static Ticker mqttsubscribeTimer;

  private:
    inline static CommandHandler _cmdHandler;     // command handler function for command from either wifi module or mqtt command
    inline static char _cmdTopic[20];             // save the mqtt command topic set by autoConnect()
    inline static byte _nMqttReconnect = 0;       // count number of MQTT connect try before ask for turn on portal
    inline static unsigned long _timeStartPortal; // start time of forced portal (2 min time out check)

    // only used to mark if the wifi event listener is set (nullptr by default)
    inline static WiFiEventHandler _wifiConnectHandler;
    inline static WiFiEventHandler _wifiDisconnectHandler;

    // used to subscribe mqtt command topic
    static void _subscribeMqttCommand();

    // Wifi portal
    inline static bool _portalOn = false;         // indicate if configuration portal is on
    inline static bool _portalSubmitted = false;  // The configuration form submitted
    inline static bool _forcePortal = false;      // indicate if force turn on portal anyway no matter network is connected or not
    inline static char _portalReason[50];         // reason of open portal
    inline static ESP8266WebServer _webServer = ESP8266WebServer(80); // portal web server

    static void _handlePortal();

    static void _setupWifiListener();
    static void _connectToWifi();
    static void _connectToMqtt();

    // extract ip address
    static void _extractIpAddress(const char* sourceString, short* ipAddress);

    // Disallow creating an instance of this object by putting constructor in private
    myWifi() {};
};
