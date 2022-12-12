#include "Filter.hpp"

float MedianFilter::state(double measure)
{
    _index++;
	if (_index == 4) _full = true;
	_index %= 5;

    // cyclic reusing the data buffer
    _lastReadings[_index] = measure;

	// if the data buffer is not full, return the current measure
	if (!_full) _last = measure;
	else
	{
		_last = _median(
			_lastReadings[0],
			_lastReadings[1],
			_lastReadings[2],
			_lastReadings[3],
			_lastReadings[4]);
	}

	return _last;
}

// Trick using XOR to swap two variables
//#define swap(a,b) a ^= b; b ^= a; a ^= b; // good for int only
#define swap(a,b) a = a+b; b = a-b; a = a-b;
#define sort(a,b) if(a>b){ swap(a,b); }

// http://cs.engr.uky.edu/~lewis/essays/algorithms/sortnets/sort-net.html
// Median could be computed two less steps...
float MedianFilter::_median(float a, float b, float c, float d, float e)
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


// https://github.com/rizkymille/ultrasonic-hc-sr04-kalman-filter/blob/master/hc-sr04_kalman_filter/hc-sr04_kalman_filter.ino
// https://en.wikipedia.org/wiki/Kalman_filter
// http://bilgin.esme.org/BitsAndBytes/KalmanFilterforDummies <= nice one!
float KalmenFilter::state(double Z)
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

float EWMAFilter::state(double measure)
{
  static const double lambda = 0.5; // The smaller, the more weight put on the new data

  if(_ewma == 0) _ewma = measure; // initial value
  else _ewma = lambda*_ewma + (1-lambda)*measure;
  return _ewma;
}