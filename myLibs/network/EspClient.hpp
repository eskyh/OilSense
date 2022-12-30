#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.h>

#include "StensTimer.h"
#include "sensor.hpp"
#include "Config.hpp"

#include "ESPAsyncWebServer.h"

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  // #include <ESP8266WebServer.h>
  // #include <ESP8266mDNS.h>
  // #include <ESP8266HTTPUpdateServer.h>

  #define ESPmDNS ESP8266mDNS
  #define WebServer ESP8266WebServer

  // #define DEFAULT_MQTT_CLIENT_NAME "ESP8266"
  // #define ESPHTTPUpdateServer ESP8266HTTPUpdateServer

#elif defined(ESP32)
  #include <WiFiClient.h>
  // #include <WebServer.h>
  // #include <ESPmDNS.h>
  // #include "ESP32HTTPUpdateServer.h"

  // #define DEFAULT_MQTT_CLIENT_NAME "ESP32"
  // #define ESPHTTPUpdateServer ESP32HTTPUpdateServer

#else
    #error Platform not supported
#endif

//----------------------
#define MQTT_SUB_CMD 	"/cmd/#"

#define CMD_OPEN_PORTAL "/cmd/portal"     // this is not sent from MQTT, instead sent from wifi module when there is connection issues!!

#define CMD_SLEEP       "/cmd/sleep"
#define CMD_LED_BLINK   "/cmd/ledblink"
#define CMD_AUTO        "/cmd/auto"       // auto or manual mode for measurement
#define CMD_MEASURE     "/cmd/measure"
#define CMD_INTERVAL    "/cmd/interval"
#define CMD_RESTART     "/cmd/restart"
#define CMD_SSR_FILTER  "/cmd/filter"

// #define CMD_SSR_SR04    "/cmd/on/sr04"  // turn on/off the measure
// #define CMD_SSR_VL53    "/cmd/on/vl53"
// #define CMD_SSR_DH11    "/cmd/on/dh11"

// #define MQTT_PUB_SR04   "/sensor/sr04"
// #define MQTT_PUB_VL53   "/sensor/vl53"
// #define MQTT_PUB_DH11   "/sensor/dh11"

#define MQTT_PUB_HEARTBEAT  "/heartbeat"

// #define MQTT_PUB_INFO       "/msg/info"
// #define MQTT_PUB_WARN       "/msg/warn"
// #define MQTT_PUB_ERROR      "/msg/error"
//----------------------


#define MQTT_MAX_TRY             15     // Max number of MQTT connect tries before ask for turn on portal
#define MQTT_RECONNECT_INTERVAL  2e3    // Time interval between each MQTT reconnection attempt, 2s by default
#define MQTT_SUBSCRIBE_DELAY     1e3    // MQTT subscribe attempt delay after connected
#define WIFI_CONNECTING_TIMEOUT  20e3   // Wifi connecting timeout, 20s bu default
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

    void setup();
    void loop(); // Main loop, to call at each sketch loop()

    virtual void timerCallback(Timer* timer);

  protected:
    // std::vector<Sensor> sensors;

    // Settings
    // Settings settings;

  private:
    // https://stackoverflow.com/questions/448056/c-singleton-getinstance-return
    EspClient() {};
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

    // bool _mqttCleanSession;
    // char* _mqttLastWillTopic;
    // char* _mqttLastWillMessage;
    // bool _mqttLastWillRetain;
    // unsigned int _failedMQTTConnectionAttemptCount;

  private:
    // Config ortal related
    bool _portalOn = false;         // indicate if configuration portal is on
    bool _portalSubmitted = false;  // The configuration form submitted
    char _portalReason[50];         // reason of open portal (Not used at this moment.)

    AsyncWebServer _webServer = AsyncWebServer(80);

    // WiFi related
    bool _wifiConnected = false;
    bool _wifiReconnected = false;
    void _setupWifi();

    // MQTT related
    Timer *_pTimerReconnect = NULL;
    bool _mqttConnected = false;
    void _setupMQTT();
    void _setupOTA();

    void _connectToWifi(bool blocking=false);
    void _connectToMqttBroker();

    void _cmdHandler(const char* topic, const char* payload);

    // Timer action IDs
    enum {
      ACT_HEARTBEAT,  // heartbeat
      ACT_MEASURE,     // sensor measure timer
      ACT_WIFI_CONNECT_TIMEOUT,  // wait for WIFI connect time out
      ACT_MQTT_RECONNECT,       // MQTT reconnect try (max count of try defined in _nMaxMqttReconnect)
      ACT_MQTT_SUBSCRIBE,       // MQTT subscribe action better delay sometime when MQTT connected. This is delayed time out for subscribing
      ACT_CLOSE_PORTAL          // Config portal open time out (i.e., 120s after open)
    };

    // Sensors
    bool _ledBlink = true;
    bool _autoMode = true;
    Timer* _timer_sensor = NULL; // timer to control measure interval
    Sensor *_sensors[MAX_SENSORS] = {NULL}; // initialized all with NULL
    void _initSensors();
    void _enableSensor(const char* name, bool enable);
    void _measure();
    void _blink();

    // Utility functions
    // static void _extractIpAddress(const char* sourceString, short* ipAddress);
};
