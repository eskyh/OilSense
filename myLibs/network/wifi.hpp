#pragma once

// all includes are from Arduino Core, no external lib
#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>         
  #include <WebServer.h>
  #include <time.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  #include <TZ.h>
  // <time.h> and <WiFiUdp.h> not needed. already included by core.         
#endif

#include <ArduinoOTA.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
// #include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define NTP_MIN_VALID_EPOCH 1640995200  // 1/2/2022. Use https://www.epochconverter.com/

#include <CRC32.h>

struct Settings {
    //Wifi ssid and pass
    char ssid[25];
    char pass[15];

    // MQTT host name and port
    char mqttHost[25];
    int mqttPort = 1883;
    char mqttUser[20];
    char mqttPass[15];

    // OTA host name and pass
    char otaHost[25];
    char otaPass[15];

    uint32_t CRC = 0;
};

typedef std::function<void(const char* topic, const char* payload)> CommandHandler;

class myWifi
{
  public:
    //------------------------------------
    // Wifi portal
    // static void setOTACredential(const char* otaHostName, const char* otaPassword);
    // static void setMqttCredential(const char* mqttHost, const char* user, const char* pass); //, int mqttPort);
    inline static bool portalOn = false; // indicate if configuration portal is on
    inline static bool forcePortal = false; // indicate if force turn on portal anyway no matter network is connected or not
    inline static char portalReason[50]; // reason of open portal
    inline static ESP8266WebServer server = ESP8266WebServer(80);

    static void setupWifiListener();
    static void connect();
    static void autoConnect(CommandHandler cmdHandler, const char* cmdTopic);
    static void handlePortal();
    static void startConfigPortal(bool force=false); //char const *apName, char const *apPassword);

    static void syncTimeNTP();
    static void waitSyncNTP();

    static void setUpMQTT();
    static void connectToMqtt();
    static void subscribeMqtt();

    static void setUpOTA();

    // static void OnCommand(const char* cmdTopic, CommandHandler cmdHandler);

    // static member has to be defined with inline here or in .cpp
    inline static AsyncMqttClient mqttClient;

    static void pollPortal();

    // settings
    static bool getSettings();
    static void setSettings();
    inline static Settings settings;

  protected:
    // https://stackoverflow.com/questions/9110487/undefined-reference-to-a-static-member
    inline static Ticker wifiReconnectTimer;
    inline static Ticker mqttReconnectTimer;
    inline static Ticker mqttsubscribeTimer;

    inline static WiFiEventHandler wifiConnectHandler;
    inline static WiFiEventHandler wifiDisconnectHandler;

    inline static CommandHandler _cmdHandler;
    inline static char _cmdTopic[20];

  private:
    inline static byte _nMqttReconnect = 0;

    // Disallow creating an instance of this object by putting constructor in private
    myWifi() {};
};
