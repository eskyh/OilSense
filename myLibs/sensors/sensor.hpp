#pragma once

#include <time.h>
#include <AsyncMqttClient.h>

#include "filter.hpp"
#include "band.hpp"

/*
 * Sonar readings with median noise filtering
 */
class Sensor
{
  public:
		Sensor(const char* sensorName, int nMeasures,
    			 FilterType filter=Median,
					 BandType band=BT_None, uint16_t gap=0, bool pct=false);

    void enable(bool enable) {_enabled = enable;};
    bool isEnabled() {return _enabled;};

    // Returns the measurement in an array (in cm)
    bool measure();
    void setFilter(FilterType type); // set all filters to the same type
    void setFilter(int index, FilterType type); // set specific filter

    void setBand(BandType type, uint16_t gap, bool pct); // set all bands to the same type
    void setBand(int index, BandType type, uint16_t gap, bool pct); // set specific band

    void setMqtt(AsyncMqttClient *pClient, const char* topic, int qos=0, bool retain=false);
    void sendMeasure();
		virtual char* getPayload() = 0;
		virtual ~Sensor();

    char name[25]; // sensor name

  protected:
    // MQTT parameters
    AsyncMqttClient* _pMqttClient = NULL;
    
    char _topic[25];   		// mqtt topic
    int _qos = 0;      		// mqtt qos
    bool _retain = false; // mqtt retain
		
		time_t _timestamp; 		// timestamp of the current measure

		// Measurements
    virtual bool _read() = 0; // overload this function to read sensor values
  
		int _nMeasures; 		
		float *_measures = NULL; // save final measures. Filter processed measures are saved in here. Length: _nMeasures
    Filter** _filters = NULL;
    Band** _bands = NULL;

  private:
    bool _enabled = true;
};
