/*
Copyright (C) 2017  Arjen Stens

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef StensTimer_h
#define StensTimer_h

#include <Arduino.h>
#include <timer/Timer.h>
#include <IStensTimerListener.h>

#ifdef Arduino_h
// arduino is not compatible with std::vector
#undef min
#undef max
#endif

class StensTimer {

  #define MAX_TIMERS 20

    public:
      static StensTimer* getInstance();
      ~StensTimer();

      void run();

      Timer* setTimer(IStensTimerListener* listener, int action, long delay, long repetitions = 1);
      Timer* setTimer(int action, long delay, long repetitions = 1);
      Timer* setInterval(IStensTimerListener* listener, int action, long interval);
      Timer* setInterval(int action, long interval);
      void setStaticCallback(void (*staticTimerCallback)(Timer*));
      void deleteStaticCallback();
      void deleteTimer(Timer* timer);
      void deleteTimers();

    private:
      static StensTimer* _instance;
      StensTimer();

      long _lastId = 0;
      Timer* _timers[MAX_TIMERS] = {NULL};
      void (*_staticTimerCallback)(Timer*) = NULL;

      /* Helper functions */
      int findFreeSlot();
      Timer* createTimer(IStensTimerListener* listener, int action, long interval, int repetitions);
  };

#endif
