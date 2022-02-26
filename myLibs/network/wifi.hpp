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
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define NTP_MIN_VALID_EPOCH 1640995200  // 1/2/2022. Use https://www.epochconverter.com/

typedef std::function<void(char* topic, char* payload)> CommandHandler;

class myWifi
{
  public:
    static void setOTACredential(const char* otaHostName, const char* otaPassword);
    static void setMqttCredential(const char* mqttHost, const char* user, const char* pass, int mqttPort);

    // apName is the wifi ssid of the AP mode of the controller.
    // Connect this wifi ssid to select the actual wifi ssid
    static void setupWifi();
    static void connectToWifi();

    static void syncTimeNTP();
    static void waitSyncNTP();

    static void setUpMQTT();
    static void connectToMqtt();

    static void setUpOTA();

    static void OnCommand(const char* cmdTopic, CommandHandler cmdHandler);

    // static member has to be defined with inline here or in .cpp
    inline static AsyncMqttClient mqttClient;

  protected:
    inline static WiFiManager _wifiManager;
    inline static char _apName[20];

    // https://stackoverflow.com/questions/9110487/undefined-reference-to-a-static-member

    // OTA host name and pass
    inline static char _otaHostName[20];
    inline static char _otaPassword[15];

    // MQTT host name and port
    inline static char _mqttHost[20];
    inline static int _mqttPort;
    inline static char _mqttUser[20];
    inline static char _mqttPass[15];

    inline static Ticker wifiReconnectTimer;
    inline static Ticker mqttReconnectTimer;

    inline static WiFiEventHandler wifiConnectHandler;
    inline static WiFiEventHandler wifiDisconnectHandler;

    inline static CommandHandler _cmdHandler;
    inline static char _cmdTopic[20];

    static void onWifiConnect(const WiFiEventStationModeGotIP& event);
    static void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);

  private:
    // Disallow creating an instance of this object by putting constructor in private
    myWifi() {};
};
