#pragma once

#include <time.h>
#include <AsyncMqttClient.h>

#include "filter.hpp"

/*
 * Sonar readings with median noise filtering
 */
class Sensor
{
  public:
		Sensor(const char* name, int nMeasures=1, FilterType filter=None);

    // Returns the measurement in an array (in cm)
    bool measure();
    void setFilter(FilterType type); // set all filters to the same type
    void setFilter(int index, FilterType type); // set specific filter

    void setMqtt(AsyncMqttClient *pClient, const char* topic, int qos=0, bool retain=false);
    void sendMeasure();
		virtual char* getPayload() = 0;
		virtual ~Sensor();
      
  protected:
		char _name[25]; // sensor name
	
    // MQTT parameters
    AsyncMqttClient* _pMqttClient = NULL;
    
    char _topic[25];   		// mqtt topic
    int _qos = 0;      		// mqtt qos
    bool _retain = false; // mqtt retain
		
		time_t _timestamp; 		// timestamp of the current measure
    // char _payload[120]; 	// string buffer for SR04 sensor message

		// Measurements
    virtual bool _read() = 0; // overload this function to read sensor values
  
		int _nMeasures; 		
		float *_measures = NULL; // save final measures. Filter processed measures are saved in here. Length: _nMeasures
    Filter** _filters = NULL;
};
