#pragma once

// #include <PubSubClient.h>
//#include <vector>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
//   #include <ESP8266HTTPUpdateServer.h>

  #define ESPmDNS ESP8266mDNS
  #define WebServer ESP8266WebServer

  #define DEFAULT_MQTT_CLIENT_NAME "ESP8266"
//   #define ESPHTTPUpdateServer ESP8266HTTPUpdateServer

#elif defined(ESP32)
  #include <WiFiClient.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
//   #include "ESP32HTTPUpdateServer.h"

  #define DEFAULT_MQTT_CLIENT_NAME "ESP32"
//   #define ESPHTTPUpdateServer ESP32HTTPUpdateServer

#else
    #error Platform not supported
#endif

#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>
#include <CRC32.h>

#include "StensTimer.h"
#include "Settings.hpp"

typedef std::function<void(const char* topic, const char* payload)> CommandHandler;

class EspClient : public IStensTimerListener
{
  // Singleton design (e.g., private constructor)
  public:
    static EspClient& instance();
    ~EspClient() {};

    AsyncMqttClient mqttClient;

    // must called before loop(), therefore in the setup() in main program
    void setCommandHandler(CommandHandler cmdHandler, const char* cmdTopic); 

    void loop(); // Main loop, to call at each sketch loop()

    virtual void timerCallback(Timer* timer);

  protected:
      // settings
      Settings settings;
      bool getSettings();
      void saveSettings();

  private:
    // https://stackoverflow.com/questions/448056/c-singleton-getinstance-return
    EspClient();
    EspClient(const EspClient&);
    EspClient& operator=(const EspClient&);

    // Wifi related
  //   const char* _wifiSsid;
  //   const char* _wifiPassword;

  //   const char* _mqttServerIp;
  //   const char* _mqttUsername;
  //   const char* _mqttPassword;
  //   const char* _mqttClientName;
  //   uint16_t _mqttServerPort;
    bool _mqttCleanSession;
  //   char* _mqttLastWillTopic;
  //   char* _mqttLastWillMessage;
  //   bool _mqttLastWillRetain;
    unsigned int _failedMQTTConnectionAttemptCount;

    CommandHandler _cmdHandler;     // command handler function for command from either wifi module or mqtt command
    char _cmdTopic[25];             // save the mqtt command topic set by autoConnect()

    // HTTP/OTA update related

  public:
    // inline bool isConnected() const { return isWifiConnected() && isMqttConnected(); }; // Return true if everything is connected
    // inline bool isWifiConnected() const { return _wifiConnected; }; // Return true if wifi is connected
    // inline bool isMqttConnected() const { return _mqttConnected; }; // Return true if mqtt is connected
    // inline unsigned int getConnectionEstablishedCount() const { return _connectionEstablishedCount; }; // Return the number of time onConnectionEstablished has been called since the beginning.

    // Default to onConnectionEstablished, you might want to override this for special cases like two MQTT connections in the same sketch
    // inline void setOnConnectionEstablishedCallback(ConnectionEstablishedCallback callback) { _connectionEstablishedCallback = callback; };

    // Allow to set the minimum delay between each MQTT reconnection attempt. 15 seconds by default.
    // inline void setMqttReconnectionAttemptDelay(const unsigned int milliseconds) { _mqttReconnectionAttemptDelay = milliseconds; };

  private:
    inline static bool _portalOn = false;         // indicate if configuration portal is on
    inline static bool _portalSubmitted = false;  // The configuration form submitted
    inline static bool _forcePortal = false;      // indicate if force turn on portal anyway no matter network is connected or not
    inline static char _portalReason[50];         // reason of open portal

    WebServer _webServer = WebServer(80);
    void _handleWebRequest();
    void _openConfigPortal(bool force=false);
    void _closeConfigPortal();
    void _handleConfigPortal();

    bool _wifiConnected = false;
    bool _wifiReconnected = false;
    void _setupWifi();
    void _handleWifi();

    // MQTT related
    byte _nMqttReconnect = 0;       // count number of MQTT connect try before ask for turn on portal_nMqttReconnect
    bool _mqttConnected = false;
    unsigned long _nextMqttConnectionAttemptMillis;
    unsigned int _mqttReconnectionAttemptDelay;
    void _setupMQTT();
    void _handleMQTT();

    void _setupOTA();

    void _connectToWifi();
    void _connectToMqttBroker();
  //   void processDelayedExecutionRequests();

    // Timer for reconnections
    enum {
      ACT_MQTT_RECONNECT = 0,
      ACT_MQTT_SUBSCRIBE = 1,
      ACT_CFG_PORTAL     = 2
    };

    // Utilities
    static void _extractIpAddress(const char* sourceString, short* ipAddress);
};
