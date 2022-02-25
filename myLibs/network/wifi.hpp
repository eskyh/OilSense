#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>

#include <Ticker.h>

typedef std::function<void(char* topic, char* payload)> CommandHandler;

class myWifi
{
  public:
    static void setWifiCredential(const char* ssid, const char* password);
    static void setOTACredential(const char* otaHostName, const char* otaPassword);
    static void setMqttCredential(const char* mqttHost, int mqttPort);

    static void setupWifi();
    static void connectToWifi();

    static void setUpMQTT();
    static void connectToMqtt();

    static void setUpOTA();

    static void OnCommand(const char* cmdTopic, CommandHandler cmdHandler);

    // static member has to be defined with inline here or in .cpp
    inline static AsyncMqttClient mqttClient;

  protected:
    // https://stackoverflow.com/questions/9110487/undefined-reference-to-a-static-member

    // WiFi ssid and pass
    inline static char _ssid[15];
    inline static char _password[15];

    // OTA host name and pass
    inline static char _otaHostName[20];
    inline static char _otaPassword[15];

    // MQTT host name and port
    inline static char _mqttHost[20];
    inline static int _mqttPort;

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
