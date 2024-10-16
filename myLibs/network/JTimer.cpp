#include "JTimer.h"

JTimer &JTimer::instance()
{
  static JTimer _instance;
  return _instance;
}

/* Delete timers before object will be destroyed */
JTimer::~JTimer() {}

void JTimer::setStaticCallback(void (*staticTimerCallback)(Timer &))
{
  _staticTimerCallback = staticTimerCallback;
}

void JTimer::deleteStaticCallback()
{
  _staticTimerCallback = NULL;
}

// return enabled timer with matching action
Timer *JTimer::getTimer(int action)
{
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    if (_timers[i].enable && _timers[i].action == action)
      return &(_timers[i]);
  }

  return NULL;
}

/* Creates new timer and returns its pointer */
Timer *JTimer::_resetTimer(IJTimerListener *listener, int action, long interval, int repetitions)
{
  int idx0 = -1;
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    if (_timers[i].enable)
    {
      if (_timers[i].action == action)
      {
        _timers[i].init(listener, action, interval, repetitions);
        return &(_timers[i]);
      }
    }
    else
    {
      if (idx0 == -1)
        idx0 = i; // record the first disabled timer index
    }
  }

  if (idx0 != -1)
  {
    _timers[idx0].init(listener, action, interval, repetitions);
    return &(_timers[idx0]);
  }
  else
  {
#ifdef _DEBUG
    Serial.println(F("Error: Timer slots full!"));
#endif

    return NULL;
  }
}

/* Creates and returns timer that runs once */
Timer *JTimer::setTimer(IJTimerListener *listener, int action, long delay, long repetitions)
{
  return _resetTimer(listener, action, delay, repetitions);
}

/* Sets timer without listener object */
Timer *JTimer::setTimer(int action, long delay, long repetitions)
{
  /* Static callback must be set */
  if (_staticTimerCallback == NULL)
  {
    return NULL;
  }

  return _resetTimer(NULL, action, delay, repetitions);
}

/* Creates and returns timer that runs forever */
Timer *JTimer::setInterval(IJTimerListener *listener, int action, long interval)
{
  return _resetTimer(listener, action, interval, 0);
}

/* Sets interval without listener object */
Timer *JTimer::setInterval(int action, long interval)
{
  /* static callback must be set */
  if (_staticTimerCallback == NULL)
  {
    return NULL;
  }

  return _resetTimer(NULL, action, interval, 0);
}

/* Checks for every slot of the timer array if there is any timer.
If so, it will be processed further. */
void JTimer::run()
{
  long now = millis();

  /* Execute callback for every timer slot that is not null */
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    if (!_timers[i].enable)
      continue;

    /* For readability create temporary variable */
    Timer &timer = _timers[i];

    /* Skip timer instance if time is not yet due */
    if (now < timer.lastCall + timer.delay)
      continue;

    /* Update last call time */
    timer.lastCall = now;

    /* Pass timer to callback function implemented by user, can be a static function */
    if (_staticTimerCallback == NULL)
    {
      timer.listener->timerCallback(timer);
    }
    else
    {
      _staticTimerCallback(timer);
    }

    /* if user deleted timer while being called, skip over last lines */
    if (!timer.enable)
      continue;

    int repetitions = timer.repetitions;

    /* Check if timer is done repeating, if so, delete it */
    if (repetitions > 1)
      timer.repetitions--;
    else if (repetitions == 1)
      timer.enable = false;
  }
}
