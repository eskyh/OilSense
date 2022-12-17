// #define DEBUG

#include "sensor.hpp"
#include "filter.hpp"

// name: sensor name
// nMeasures: number of measures a sensor can generator (some sensor integrates multiple type of measures)
Sensor::Sensor(const char* name, int nMeasures, FilterType filter)
{
  // sizeof: Returns the length of the given byte string, include null terminator;
  // strlen: Returns the length of the given byte string not including null terminator;
  strncpy(_name, name, sizeof(_name));
	_nMeasures = nMeasures;

  // init the filter pointer array
	_filters = new Filter*[_nMeasures];
	for (int i = 0; i < _nMeasures; i++)
	{
		_filters[i] = NULL;
	}

  setFilter(filter);

  // init the measure array
	_measures = (float *) calloc(nMeasures, sizeof(float));
}

Sensor::~Sensor()
{
	for (int i = 0; i < _nMeasures; i++)
	{
		if (_filters[i] != NULL) delete _filters[i];
	}

	free(_measures);
}

void Sensor::setFilter(FilterType type)
{
	for (int i = 0; i < _nMeasures; i++)
	{
		setFilter(i, type);
	}
}

void Sensor::setFilter(int index, FilterType type)
{
	if (_filters[index] != NULL)
	{
		delete _filters[index];
		_filters[index] = NULL;
	}

	switch (type)
	{
	case None:
		_filters[index] = NULL;
		break;
	case Median:
		_filters[index] = new MedianFilter();
		break;
	case Kalmen:
		_filters[index] = new KalmenFilter();
		break;
	case EWMA:
		_filters[index] = new EWMAFilter();
		break;
	}
}

// qos :
//     0: At most once
//     1: At least once
//     2: Exactly once
void Sensor::setMqtt(AsyncMqttClient *pClient, const char* topic, int qos, bool retain)
{
  _pMqttClient = pClient;

	// write the default topic of the sensor for mqtt
  // strcpy(_topic, topic);
  strncpy(_topic, topic, sizeof(_topic));

  _qos = qos;
  _retain = retain;
}

void Sensor::sendMeasure()
{
  if (_pMqttClient == NULL)
  {
    Serial.println(F("MQTT client is not set!"));
    return;
  }
  
  // format the payload. Avoid using String!!
  // _timestamp = time(NULL); // get current timestamp (in sec), convert it to millisec below
  
  // Accuracy only to 1mm. so output to 1 decimal place.
  if(!measure())
  {
    Serial.printf("%s: measure failed!\n", _name);
    return;
  }
	
  char* payload = getPayload();
  _pMqttClient->publish(_topic, _qos, _retain, payload); // retain will clear the chart when deploying!! set it to false

  #ifdef _DEBUG
	Serial.printf("%s: %s\n", _name, payload);
  #endif
}

// return measure in cm
bool Sensor::measure() 
{
  if(!_read()) return false;

  for(int i=0; i<_nMeasures; i++)
  {
    // get filter value if filter is set
    if(_filters[i] != NULL)
      _measures[i] = _filters[i]->state(_measures[i]);
  }

  return true;
}
