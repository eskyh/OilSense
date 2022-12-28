#pragma once

// #include <PubSubClient.h>
//#include <vector>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  // #include <ESP8266mDNS.h>
  // #include <ESP8266HTTPUpdateServer.h>

  #define ESPmDNS ESP8266mDNS
  #define WebServer ESP8266WebServer

  // #define DEFAULT_MQTT_CLIENT_NAME "ESP8266"
  // #define ESPHTTPUpdateServer ESP8266HTTPUpdateServer

#elif defined(ESP32)
  #include <WiFiClient.h>
  #include <WebServer.h>
  // #include <ESPmDNS.h>
  // #include "ESP32HTTPUpdateServer.h"

  // #define DEFAULT_MQTT_CLIENT_NAME "ESP32"
  // #define ESPHTTPUpdateServer ESP32HTTPUpdateServer

#else
    #error Platform not supported
#endif

#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>
#include <CRC32.h>

#include "StensTimer.h"
#include "Settings.hpp"

#define MQTT_MAX_TRY             15     // Max number of MQTT connect tries before ask for turn on portal
#define MQTT_RECONNECT_INTERVAL  2e3    // Time interval between each MQTT reconnection attempt, 2s by default
#define MQTT_SUBSCRIBE_DELAY     1e3    // MQTT subscribe attempt delay after connected
#define WIFI_CONNECTING_TIMEOUT  20e3   // Wifi connecting timeout
#define PORTAL_TIMEOUT           120e3  // Configuration portal timeout

typedef std::function<void(const char* topic, const char* payload)> CommandHandler;

/* StensTimer:: To allow callbacks on class instances you should let your class implement
IStensTimerListener and implement its timerCallback function as shown below.
Check examples in https://gitlab.com/arduino-libraries/stens-timer
*/
class EspClient : public IStensTimerListener
{
  // Singleton design (e.g., private constructor)
  public:
    static EspClient& instance();
    ~EspClient() {};

    AsyncMqttClient mqttClient;

    inline bool isConnected() const { return _wifiConnected && _mqttConnected; }; // Return true if everything is connected

    // Portal    
    void openConfigPortal(bool blocking=false);
    void closeConfigPortal();

    // must called before loop(), therefore in the setup() in main program
    void setCommandHandler(CommandHandler cmdHandler, const char* cmdTopic); 

    void setup(CommandHandler cmdHandler, const char* cmdTopic);
    void loop(); // Main loop, to call at each sketch loop()

    virtual void timerCallback(Timer* timer);

  protected:
    // Settings
    Settings settings;
    bool getSettings();
    void saveSettings();

  private:
    // https://stackoverflow.com/questions/448056/c-singleton-getinstance-return
    EspClient();
    EspClient(const EspClient&) = delete; // deleting copy constructor.
    EspClient& operator=(const EspClient&) = delete; // deleting copy operator.

    // restart code
    enum RsCode{
      RS_NORMAL,
      RS_WIFI_DISCONNECT,
      RS_MQTT_DISCONNECT,
      RS_DISCONNECT,
      RS_CFG_CHANGE
    };

    void _restart(RsCode code=RS_NORMAL);  // restart the device

    bool _mqttCleanSession;
    // char* _mqttLastWillTopic;
    // char* _mqttLastWillMessage;
    // bool _mqttLastWillRetain;
    // unsigned int _failedMQTTConnectionAttemptCount;

    CommandHandler _cmdHandler;     // command handler function for command from either wifi module or mqtt command
    char _cmdTopic[25];             // save the mqtt command topic set by autoConnect()

  private:
    // Config ortal related
    bool _portalOn = false;         // indicate if configuration portal is on
    bool _portalSubmitted = false;  // The configuration form submitted
    char _portalReason[50];         // reason of open portal (Not used at this moment.)

    WebServer _webServer = WebServer(80);
    void _handleWebRequest();

    // WiFi related
    bool _wifiConnected = false;
    bool _wifiReconnected = false;
    void _setupWifi();

    // MQTT related
    bool _mqttConnected = false;
    bool _mqttConnecting = false;    // indicate the timer to reconnect is set, but not connected yet.
    void _setupMQTT();
    void _setupOTA();

    void _connectToWifi(bool blocking=false);
    void _connectToMqttBroker();

    // Timer action IDs
    enum {
      ACT_WIFI_CONNECT_TIMEOUT,  // wait for WIFI connect time out
      ACT_MQTT_RECONNECT,       // MQTT reconnect try (max count of try defined in _nMaxMqttReconnect)
      ACT_MQTT_SUBSCRIBE,       // MQTT subscribe action better delay sometime when MQTT connected. This is delayed time out for subscribing
      ACT_CLOSE_PORTAL          // Config portal open time out (i.e., 120s after open)
    };

    // Utility functions
    static void _extractIpAddress(const char* sourceString, short* ipAddress);
};
