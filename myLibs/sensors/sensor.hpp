#pragma once

#include <time.h>
#include <AsyncMqttClient.h>

#include "filter.hpp"
#include "band.hpp"

/*
 * Base Sensor class for all derived sensor classes, e.g., SR04, DH11, VL53L0X, etc
 */
class Sensor
{
public:
    Sensor(const char *sensorName, int nMeasures,
           FilterType filter = Median,
           BandType band = BT_None, uint16_t gap = 0, bool pct = false);

    void enable(bool enable) { _enabled = enable; };
    bool isEnabled() { return _enabled; };

    // Returns the measurement in an array (in cm)
    bool measure();
    void setFilter(FilterType type);            // set all filters to the same type
    void setFilter(int index, FilterType type); // set specific filter

    void setBand(BandType type, uint16_t gap, bool pct);            // set all bands to the same type
    void setBand(int index, BandType type, uint16_t gap, bool pct); // set specific band

    void setMqtt(AsyncMqttClient *pClient, const char *topic, int qos = 0, bool retain = false); // set MQTT client
    void sendMeasure();                                                                          // send measurement using MQTT message
    virtual char *getPayload() = 0;
    virtual ~Sensor();

    char name[25]; // sensor name

protected:
    // MQTT parameters
    AsyncMqttClient *_pMqttClient = NULL;

    char _topic[25];      // mqtt topic
    int _qos = 0;         // mqtt qos
    bool _retain = false; // mqtt retain

    time_t _timestamp; // timestamp of the current measure

    // Measurements
    virtual bool _read() = 0; // overload this function to read sensor values

    int _nMeasures;
    float *_measures = NULL;  // Save the measures. Filter processed measures are saved here. Length: _nMeasures
    Filter **_filters = NULL; // Data filter pointer
    Band **_bands = NULL;

private:
    bool _enabled = true; // If this sensor is enabled
};
