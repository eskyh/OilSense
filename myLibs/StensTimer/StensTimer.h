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

#pragma once

#include <Arduino.h>
#include <Timer.h>
#include <IJTimerListener.h>

#ifdef Arduino_h
// arduino is not compatible with std::vector
#undef min
#undef max
#endif

#define MAX_TIMERS 20

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

    void setStaticCallback(void (*staticTimerCallback)(Timer*));
    void deleteStaticCallback();

  private:
    JTimer() {};
    JTimer(const JTimer&) = delete; // deleting copy constructor.
    JTimer& operator=(const JTimer&) = delete; // deleting copy operator.

    Timer _timers[MAX_TIMERS];
    void (*_staticTimerCallback)(Timer*) = NULL;

    /* Helper functions */
    Timer* _resetTimer(IJTimerListener* listener, int action, long interval, int repetitions);
};
