#include <stdint.h>
#include <stdlib.h>
#include "band.hpp"

void Band::reset(BandType type, uint16_t gap, bool pct)
{
  _type = type;
  _gap = gap;
  _pct = pct;

  _last = 0;
}

bool Band::check(double measure)
{
  status = false;

  if(_type == BT_None || _last == 0)
  {
    _last = measure;
    status = true;
  }

	float gap = _pct ? _last*_gap/100.0 : _gap;
  float delta = abs(measure - _last);

  if(delta == 0)
  {
    if(_type == BandType::Deadband0 || _type == BandType::Narrowband0)
    {
      _last = measure;
      status = true;
    }
  }else if(delta > gap)
  {
    if(_type == BandType::Deadband0 || _type == BandType::Deadband1)
    {
      _last = measure;
      status = true;
    }
  }else if(delta < gap)
  {
    if(_type == BandType::Narrowband0 || _type == BandType::Narrowband1)
    {
      _last = measure;
      status = true;
    }
  }

	return status;
}