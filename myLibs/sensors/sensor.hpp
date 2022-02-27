#pragma once

#include <time.h>
#include <AsyncMqttClient.h>

/*
 * Sonar readings with median noise filtering
 */
class Sensor
{
  public:
		Sensor(const char* name, int nMeasures=1);

    // Returns the measurement in an array (in cm)
    bool measure();
    void setFilter(int filter);

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
  
    // Cyclic buffer of the last readings accumulated for computing the median
		int _nMeasures;
    float (*_lastReadings)[5] = NULL; // an array of pointer to array: two dimentional array _nMeasures x 5
    // The _index (0-4) of the current measure saved in the in the cyclic buffer _lastReadings
    int _index = 0;
		
		float *_measures = NULL; // save final measures. Filter processed measures are saved in here. Length: _nMeasures

    //-- Filters -------
    int _filter = 0; // filter type id
    
    // Median filter
    float _fMedian(float a, float b, float c, float d, float e);
    
    // Kalman filter parameters
    float _fKalman(double U);

    //  const double F = 1.0; // true state coeff
    //  const couble B = 0.0; // there is no control input
    const double H = 1.0; // measurement coeff

    const double Q = 10;   // initial estimated covariance
    const double R = 40.0; // noise covariance. The higher R, the less K

    double K = 0;          // Kalmen gain, the higher K, the more weight to the new observation
    double P = 0;          // initial error covariance (must be 0)
    double X_hat = 0;      // initial estimated state (assume unknown)
    
    // EWMA filter
    float _fEwma(double measure);
    float _ewma = 0;
};
