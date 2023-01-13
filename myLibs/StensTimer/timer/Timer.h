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

#ifndef Timer_h
#define Timer_h

  #include <Arduino.h>

  class IStensTimerListener;
  class Timer {
      public:
        Timer(IStensTimerListener* listener, long id, int action, long delay, int repetitions);
        ~Timer();

        long getId();
        int getAction();
        void setAction(int action);
        long getDelay();
        void setDelay(long delay);
        long getLastCall();
        void setLastCall(long time);
        int getRepetitions();
        void setRepetitions(int repetitions);
        IStensTimerListener* getListener();

        bool equals(Timer* timer);

      private:
        long _id = 0;
        int _action = 0;
        long _delay = 0;
        long _lastCall = 0;
        int _repetitions = 0;
        IStensTimerListener* _listener = NULL;

    };

#endif
