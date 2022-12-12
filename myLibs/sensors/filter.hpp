#pragma once

enum FilterType {
	None=0,
	Median=1,
	Kalmen=2,
	EWMA=3
};

/*
 * Filter
 */
class Filter
{
  public:
	Filter() {};
	virtual ~Filter() {};

    virtual float state(double measure)=0; // abstract function, measure is the new sensor data, return the evaluated new state.
};

class MedianFilter: public Filter
{
  public:
	MedianFilter() {};
	virtual ~MedianFilter() {};

    virtual float state(double measure);

  private:
	float _last = 0; // record the last median value
	bool _full = false; // mark if the buffer is fullfilled to calculate the median

    float _lastReadings[5] = {0}; // an array of recent 5 measures
    // The _index (0-4) of the current measure saved in the in the cyclic buffer _lastReadings

    int _index = -1;

    float _median(float a, float b, float c, float d, float e);
};

class KalmenFilter: public Filter
{
  public:
	KalmenFilter() {};
	virtual ~KalmenFilter() {};

    virtual float state(double measure);
  
  private:
  // Kalman filter parameters
   
    //  const double F = 1.0; // true state coeff
    //  const couble B = 0.0; // there is no control input
    const double H = 1.0; // measurement coeff

    const double Q = 10;   // initial estimated covariance
    const double R = 40.0; // noise covariance. The higher R, the less K

    double K = 0;          // Kalmen gain, the higher K, the more weight to the new observation
    double P = 0;          // initial error covariance (must be 0)
    double X_hat = 0;      // initial estimated state (assume unknown)
};

class EWMAFilter: public Filter
{
  public:
	EWMAFilter() {};
	virtual ~EWMAFilter() {};

    virtual float state(double measure);

  private:
    float _ewma = 0;
};
