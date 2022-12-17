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
  // #include <TZ.h>
  // <time.h> and <WiFiUdp.h> not needed. already included by core.         
#endif

#include <ArduinoOTA.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
// #include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager

#define NTP_MIN_VALID_EPOCH 1640995200  // 1/2/2022. Use https://www.epochconverter.com/

#include <CRC32.h>

// Module configuration data structure in flash
struct Settings {
  //Wifi ssid and pass
  char ssid[25] = "";
  char pass[15] = "";
  char ip[16] = "";

  // MQTT host name and port
  char mqttHost[25] = "";
  int mqttPort = 1883;
  char mqttUser[20] = "";
  char mqttPass[15] = "";

  // OTA host name and pass
  char otaHost[25] = "";
  char otaPass[15] = "";

  // CRC used to check if the data is valid
  uint32_t CRC = 0;

  // Reset all parameters
  void reset()
  {
    ssid[0] = '\0';
    pass[0] = '\0';
    ip[0] = '\0';

    mqttHost[0] = '\0';
    mqttPort = 1883;
    mqttUser[0] = '\0';
    mqttPass[0] = '\0';
    
    otaHost[0] = '\0';
    otaPass[0] = '\0';
    CRC = 0;
  }

#ifdef _DEBUG
  void print()
  {
    Serial.println(F("-------------------"));
    Serial.print(F("SSID\t: ")); Serial.println(ssid);
    Serial.print(F("PASS\t: ")); Serial.println(pass);
    Serial.print(F("IP\t: ")); Serial.println(ip);
    Serial.print(F("MQTT\t: ")); Serial.print(mqttHost); Serial.print(F(":")); Serial.println(mqttPort);
    Serial.print(F("user\t: ")); Serial.println(mqttUser);
    Serial.print(F("pass\t: ")); Serial.println(mqttPass);
    Serial.print(F("OTA\t: ")); Serial.println(otaHost);
    Serial.print(F("pass\t: ")); Serial.println(otaPass);
    Serial.println(F("-------------------"));
  }
#endif
};

typedef std::function<void(const char* topic, const char* payload)> CommandHandler;

class myWifi
{
  public:
    // static member has to be defined with inline keyword
    inline static AsyncMqttClient mqttClient;
    inline static char module[20] = {"ESP-"}; // this will be used as AP WiFi SSID, initialized in autoConnect() function

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

    // Disallow creating an instance of this object by putting constructor in private
    myWifi() {};
};
