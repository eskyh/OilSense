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

#include "Timer.h"

Timer::Timer(IStensTimerListener* listener, long id, int action, long delay, int repetitions){
  _listener = listener;
  _id = id;
  _action = action;
  _delay = delay;
  _repetitions = repetitions;

  /* this is nessecary to prevent infinite callback loop when a new timer is
   created inside callback method */
  _lastCall = millis();
}

Timer::~Timer(){
  _listener = NULL;
}

long Timer::getId(){
  return _id;
}

int Timer::getAction(){
  return _action;
}

void Timer::setAction(int action){
  _action = action;
}

long Timer::getDelay(){
  return _delay;
}

void Timer::setDelay(long delay){
  _delay = delay;
}

long Timer::getLastCall(){
  return _lastCall;
}

void Timer::setLastCall(long time){
  _lastCall = time;
}

int Timer::getRepetitions(){
  return _repetitions;
}

void Timer::setRepetitions(int repetitions){
  _repetitions = repetitions;
}

IStensTimerListener* Timer::getListener(){
  return _listener;
}

/* compare timer based on its unique id */
bool Timer::equals(Timer* timer){
  if(_id == timer->getId()){
    return true;
  }
  return false;
}
