// #define DEBUG

#include "sensor.hpp"

Sensor::Sensor(const char* name, int nMeasures=1)
{
  // sizeof: Returns the length of the given byte string, include null terminator;
  // strlen: Returns the length of the given byte string not including null terminator;
  strncpy(_name, name, sizeof(_name));
	_nMeasures = nMeasures;
  _lastReadings = (float (*)[5]) calloc(nMeasures, sizeof(float[5]));
	_measures = (float *) calloc(nMeasures, sizeof(float));
}

Sensor::~Sensor()
{
  free(_lastReadings);
	free(_measures);
}

// qos :
//     0: At most once
//     1: At least once
//     2: Exactly once
void Sensor::setMqtt(AsyncMqttClient *pClient, const char* topic, int qos=0, bool retain=false)
{
  _pMqttClient = pClient;

	// write the default topic of the sensor for mqtt
  // strcpy(_topic, topic);
  strncpy(_topic, topic, sizeof(_topic));

  _qos = qos;
  _retain = retain;
}

void Sensor::setFilter(int filter)
{
	if( filter != 0 &&
			filter != 1 &&
			filter != 2 &&
			filter != 3 )
	{
		Serial.printf("Invalide filter: %d!\n", filter);
		return;
	}
	
  _filter = filter;
}

void Sensor::sendMeasure()
{
  if (_pMqttClient == NULL)
  {
    Serial.println(F("MQTT client is not set!"));
    return;
  }
  
  // format the payload. Avoid using String!!
  _timestamp = time(NULL); // get current timestamp (in sec), convert it to millisec below
  
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
	_index++;
	_index %= 5;
	
	for(int i=0; i<_nMeasures; i++)
	{
		if(!_read()) return false;

    switch(_filter)
    {
      case 0: // no filter results
				#ifdef DEBUG
					Serial.println(F("No filter."));
				#endif
        _measures[i] = _lastReadings[i][_index];
        break;
        
      case 1: // Median filter results
				#ifdef DEBUG
					Serial.println(F("Median filter."));
				#endif
    		_measures[i] = _fMedian(
    				_lastReadings[i][0],
    				_lastReadings[i][1],
    				_lastReadings[i][2],
    				_lastReadings[i][3],
    				_lastReadings[i][4]);
        break;

      case 2: // Kalmen filter results
				#ifdef DEBUG
					Serial.println(F("Kalmen filter."));
				#endif
        _measures[i] = _fKalman(_lastReadings[i][_index]);
        break;
        
      case 3: // EWMA filter
				#ifdef DEBUG
					Serial.println(F("EWMA filter."));
				#endif
        _measures[i] = _fEwma(_lastReadings[i][_index]);
        break;
				
			default:
				Serial.println(F("Invalid filter!"));
				return false;
    }
	}

  return true;
	
//    #ifdef DEBUG
//    Serial.printf("HCSR04 reading: %umm, median: %umm\n", _lastReadings[(_index - 1) % 5], m);
//    #endif
//    
//    return m;
}

// Trick using XOR to swap two variables
//#define swap(a,b) a ^= b; b ^= a; a ^= b; // good for int only
#define swap(a,b) a = a+b; b = a-b; a = a-b;
#define sort(a,b) if(a>b){ swap(a,b); }

// http://cs.engr.uky.edu/~lewis/essays/algorithms/sortnets/sort-net.html
// Median could be computed two less steps...
float Sensor::_fMedian(float a, float b, float c, float d, float e)
{
    sort(a,b);
    sort(d,e);
    sort(a,c);
    sort(b,c);
    sort(a,d);  
    sort(c,d);
    sort(b,e);
    sort(b,c);
    // this last one is obviously unnecessary for the median
    //sort(d,e);
  
    return c;
}

float Sensor::_fEwma(double measure)
{
  static const double lambda = 0.5; // The smaller, the more weight put on the new data

  if(_ewma == 0) _ewma = measure; // initial value
  else _ewma = lambda*_ewma + (1-lambda)*measure;
  return _ewma;
}

// https://github.com/rizkymille/ultrasonic-hc-sr04-kalman-filter/blob/master/hc-sr04_kalman_filter/hc-sr04_kalman_filter.ino
// https://en.wikipedia.org/wiki/Kalman_filter
// http://bilgin.esme.org/BitsAndBytes/KalmanFilterforDummies <= nice one!
float Sensor::_fKalman(double Z)
{
//  static const double F = 1.0; // true state coeff
//  static const couble B = 0.0; // there is no control input
//  static const double H = 1.0; // measurement coeff

//  static const double Q = 10;   // initial estimated covariance
//  static const double R = 40.0; // noise covariance. The higher R, the less K

//  static double K = 0;          // Kalmen gain, the higher K, the more weight to the new observation
//  static double P = 0;          // initial error covariance (must be 0)
//  static double X_hat = 0;      // initial estimated state (assume unknown)

  if(X_hat == 0) X_hat = Z;  // initial estimated state set to the first measure
  else
  {
    K = P*H/(H*P*H+R);      // Optimal Kalman gain
    X_hat += K*(Z-H*X_hat); // Updated (a posteriori) state estimate. F=1.0, ignored
    P = (1-K*H)*(P+Q);      // Updated (a posteriori) estimate covariance (F=1.0)
  }
  return X_hat;
}
