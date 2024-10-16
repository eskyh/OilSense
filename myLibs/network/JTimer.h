#pragma once

#include <Arduino.h>
#include "JTimer.h"

#ifdef Arduino_h
#undef min
#undef max
#endif

#define MAX_TIMERS 10

class IJTimerListener;

class Timer {
  public:
    bool enable = false;  // default to false untill init
    int action = 0;       // action is the unique id of this timer
    long delay = 0;
    long lastCall = 0;
    int repetitions = 0;
    IJTimerListener* listener = NULL;

  public:
    Timer() {}
    ~Timer() {listener = NULL;}

    void init(IJTimerListener* listener, int action, long delay, int repetitions) {
      this->listener = listener;
      this->action = action;
      this->delay = delay;
      this->repetitions = repetitions;

      this->enable = true;

      /* this is nessecary to prevent infinite callback loop when a new timer is
      created inside callback method */
      this->lastCall = millis();
    }
};

class IJTimerListener {
  public:
    virtual void timerCallback(Timer& timer) = 0;
};

class JTimer
{
  public:
    static JTimer& instance();
    ~JTimer();

    void run();

    Timer* setTimer(IJTimerListener* listener, int action, long delay, long repetitions = 1);
    Timer* setTimer(int action, long delay, long repetitions = 1);

    Timer* setInterval(IJTimerListener* listener, int action, long interval);
    Timer* setInterval(int action, long interval);

    Timer* getTimer(int action);

    void setStaticCallback(void (*staticTimerCallback)(Timer&));
    void deleteStaticCallback();

  private:
    JTimer() {};
    JTimer(const JTimer&) = delete; // deleting copy constructor.
    JTimer& operator=(const JTimer&) = delete; // deleting copy operator.

    Timer _timers[MAX_TIMERS];
    void (*_staticTimerCallback)(Timer&) = NULL;

    /* Helper functions */
    Timer* _resetTimer(IJTimerListener* listener, int action, long interval, int repetitions);
};
