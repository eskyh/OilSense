#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.h>

#include "JTimer.h"
#include "sensor.hpp"
#include "Config.hpp"

#include "ESPAsyncWebServer.h"
#include "vector"

#ifdef ESP8266
  #include <ESP8266WiFi.h>

  #define ESPmDNS ESP8266mDNS
  #define WebServer ESP8266WebServer

#elif defined(ESP32)
  #include <WiFi.h>

  #define LED_BUILTIN 33
#else
    #error Platform not supported
#endif

// Known issu: ESP32 FS.h does not have interface to get used/max disk space!!

//----------------------
// MQTT command message topics

#define MQTT_SUB_CMD 	"/cmd/#"

#define CMD_OPEN_PORTAL "/cmd/portal"     // this is not sent from MQTT, instead sent from wifi module when there is connection issues!!

#define CMD_SLEEP       "/cmd/sleep"
#define CMD_LED_BLINK   "/cmd/ledblink"
#define CMD_AUTO        "/cmd/auto"       // auto or manual mode for measurement
#define CMD_MEASURE     "/cmd/measure"
#define CMD_INTERVAL    "/cmd/interval"
#define CMD_RESTART     "/cmd/restart"
#define CMD_RESET_WIFI  "/cmd/reset_wifi"  // earase wifi credential from flash
#define CMD_SSR_FILTER  "/cmd/filter"

// #define CMD_SSR_SR04    "/cmd/on/sr04"  // toggle the sensor on/off
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


//---------------------------------------
// Some timer default value
#define MQTT_MAX_TRY             15     // Max number of MQTT connect tries before ask for turn on portal
#define MQTT_RECONNECT_INTERVAL  5e3    // Time interval between each MQTT reconnection attempt, 2s by default
#define MQTT_RECONNECT_INTERVAL_LONG  10e3    // Time interval between each MQTT reconnection attempt, 2s by default
#define MQTT_SUBSCRIBE_DELAY     1e3    // MQTT subscribe attempt delay after connected
#define WIFI_CONNECTING_TIMEOUT  20e3   // Wifi connecting timeout, 20s by default

typedef std::function<void(const char* topic, const char* payload)> CommandHandler;

/* 
Implement IJTimerListener and its timerCallback function on EspClient instance
to allow being callback when the timer triggered!
*/
class EspClient : public IJTimerListener
{
  // Singleton design (e.g., private constructor)
  public:
    static EspClient& instance();
    ~EspClient() {};

    AsyncMqttClient mqttClient;

    inline bool isConnected() const { return _wifiConnected && _mqttConnected && _NtpSynched; }; // Return true if everything is connected

    // Portal    
    void setupPortal(bool blocking=false);

    void setup();
    void loop(); // Main loop, to call at each sketch loop()

    virtual void timerCallback(Timer& timer);

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
    bool _NtpSynched = false;

    // Config ortal related
    bool _portalOn = false;         // indicate if configuration portal is on
    bool _portalSubmitted = false;  // The configuration form submitted
    char _portalReason[50];         // reason of open portal (Not used at this moment.)

    AsyncWebServer _webServer = AsyncWebServer(80);

    // WiFi related
    bool _wifiConnected = false;
    void _setupWifi();
    void _connectToWifi();

    void _startAP();
    void _stopAP();

    // MQTT related
    bool _mqttConnected = false;
    void _setupMQTT();
    void _connectToMqttBroker();
  #ifdef _DEBUG
    void _printMqttDisconnectReason(AsyncMqttClientDisconnectReason reason);
  #endif

    // OTA related 
    void _setupOTA();

    void _cmdHandler(const char* topic, const char* payload);

    // Timer action IDs
    enum {
      ACT_HEARTBEAT,            // heartbeat
      ACT_MEASURE,              // sensor measure timer
      
      ACT_MQTT_RECONNECT,       // MQTT reconnect try (max count of try defined in _nMaxMqttReconnect)
      
      //-- One-off timers
      ACT_MQTT_SUBSCRIBE,       // MQTT subscribe action better delay sometime when MQTT connected. This is delayed time out for subscribing
      // ACT_CMD_RESET_WIFI,
      ACT_CMD_SYNC_NTP,         // synch internet time
      ACT_CMD_RESTART,          // restart
      ACT_CMD_MEASURE           // manual measure timer
    };

    // Sensors
    bool _ledBlink = true;
    bool _autoMode = true;
    std::vector<Sensor*> _sensors;
    void _initSensors();
    void _enableSensor(const char* name, bool enable);
    void _measure();
    void _blink();

    // Utility functions
    // void _resetWifi();
    // static void _extractIpAddress(const char* sourceString, short* ipAddress);

    // void _listDir(fs::FS &fs, const char * dirname, uint8_t levels=0);
    void _printLine() {
      #ifdef _DEBUG
        Serial.println(F("----------------------------------------------"));
      #endif
    }
};
