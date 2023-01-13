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

#include <StensTimer.h>

StensTimer* StensTimer::_instance = NULL;

/* Return existing instance, if none exists, create one */
StensTimer* StensTimer::getInstance(){
    if (_instance == NULL){ // prevents double initialization
        _instance = new StensTimer();
    }
    return _instance;
}

StensTimer::StensTimer(){}

/* Delete timers before object will be destroyed */
StensTimer::~StensTimer(){
  deleteTimers();
  _instance = NULL;
}

void StensTimer::setStaticCallback(void (*staticTimerCallback)(Timer*)){
  _staticTimerCallback = staticTimerCallback;
}

void StensTimer::deleteStaticCallback(){
  _staticTimerCallback = NULL;
}

/* Creates new timer and returns its pointer */
Timer* StensTimer::createTimer(IStensTimerListener* listener, int action, long interval, int repetitions){
  int freeSlot = findFreeSlot();

  /* No space for more timers */
  if(freeSlot < 0){
    Serial.println(F("Error: Timer slots full!"));
    return NULL;
  }

  /* Increment _lastId by one, making always unique */
  _lastId++;
  _timers[freeSlot] = new Timer(listener, _lastId, action, interval, repetitions);
  return _timers[freeSlot];
}

/* Creates and returns timer that runs once */
Timer* StensTimer::setTimer(IStensTimerListener* listener, int action, long delay, long repetitions){
  return createTimer(listener, action, delay, repetitions);
}

/* Sets timer without listener object */
Timer* StensTimer::setTimer(int action, long delay, long repetitions){
  /* Static callback must be set */
  if(_staticTimerCallback == NULL){
    return NULL;
  }

  return createTimer(NULL, action, delay, repetitions);
}

/* Creates and returns timer that runs forever */
Timer* StensTimer::setInterval(IStensTimerListener* listener, int action, long interval){
  return createTimer(listener, action, interval, 0);
}

/* Sets interval without listener object */
Timer* StensTimer::setInterval(int action, long interval){
  /* static callback must be set */
  if(_staticTimerCallback == NULL){
    return NULL;
  }

  return createTimer(NULL, action, interval, 0);
}

/* Returns first index of empty timer slot, if full, return -1 */
int StensTimer::findFreeSlot(){
  for(int index = 0; index < MAX_TIMERS; index++){
    if(_timers[index] == NULL){
      return index;
    }
  }
  return -1;
}

/* Delete given timer by searching in timers and then deleting it */
void StensTimer::deleteTimer(Timer* timer){
  if(timer == NULL){
    return;
  }

  /* Check if timer exists in timers array. If so, delete it */
  for(int i = 0; i < MAX_TIMERS; i++){
    if(_timers[i]->equals(timer)){
      delete _timers[i];
      _timers[i] = NULL;
      return;
    }
  }
}

/* Delete all timers by looping through array and call delete */
void StensTimer::deleteTimers(){
  for(int i = 0; i < MAX_TIMERS; i++){
    if(_timers[i] != NULL){
      delete _timers[i];
      _timers[i] = NULL;
    }
  }
}

/* Checks for every slot of the timer array if there is any timer.
If so, it will be processed further. */
void StensTimer::run(){

  long now = millis();

  /* Execute callback for every timer slot that is not null */
  for(int i = 0; i < MAX_TIMERS; i++){

    /* Skip iteration if no timer exists in this slot */
    if(_timers[i] == NULL){
      continue;
    }

    /* For readability create temporary variable */
    Timer* timer = _timers[i];

    /* Skip timer instance if time is not yet due */
    if(now < timer->getLastCall() + timer->getDelay()){
      continue;
    }

    /* Update last call time */
    timer->setLastCall(now);

    /* Pass timer to callback function implemented by user, can be a static function */
    if(_staticTimerCallback == NULL){
      timer->getListener()->timerCallback(timer);
    }else{
      _staticTimerCallback(timer);
    }

    /* if user deleted timer while being called, skip over last lines */
    if(_timers[i] == NULL){
      continue;
    }

    int repetitions = timer->getRepetitions();

    /* Check if timer is done repeating, if so, delete it */
    if(repetitions > 1){
      timer->setRepetitions(repetitions - 1);
    }
    else if(repetitions == 1){
      delete _timers[i]; // Destroy object
      _timers[i] = NULL; // Delete pointer
    }

  }

}
