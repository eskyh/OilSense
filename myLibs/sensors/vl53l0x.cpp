#include "vl53l0x.hpp"

VL53L0X::VL53L0X(const char* name)
	 : Sensor(name, 1, Median)
{
  if(!_lox.begin())
  {
    Serial.println("Failed to boot VL53L0X");
    _ready = false;
  }else
  {
	  _ready = true;
  }
}

// _timestamp is the one of the lastest measure
char* VL53L0X::getPayload()
{
	static char payload[50];
  // snprintf(payload, sizeof(payload), "{\"timestamp\":%lld,\"distance\":%.1f}",
  //   _timestamp*1000, _measures[0]);

  snprintf(payload, sizeof(payload), "{\"distance\":%.1f}", _measures[0]);
  return payload;
}

// Measure distance in mm
bool VL53L0X::_read()
{
  bool res = false;

  if(_ready)
  {
    VL53L0X_RangingMeasurementData_t measure;
    _lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
    
    if (measure.RangeStatus != 4)
    {  // phase failures have incorrect data
      _measures[0] = measure.RangeMilliMeter/10.0;
      res = true;
    }
    else {
      #ifdef DEBUG
        Serial.printf("%s: out of range.\n", _name);
      #endif
    }
  }

  return res;
}
