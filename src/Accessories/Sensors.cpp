/*
 *  Sensors.cpp
 * 
 *  This file is part of CommandStation.
 *
 *  CommandStation is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Sensors.h"

#include "../CommInterface/CommManager.h"
#include "EEStore.h"

#if !defined(ARDUINO_ARCH_SAMD)
#include <EEPROM.h>
#endif

void Sensor::check(int comId, int connId){
  Sensor *tt;

  for(tt=firstSensor;tt!=NULL;tt=tt->nextSensor){
    tt->signal = tt->signal * (1.0 - SENSOR_DECAY) + digitalRead(tt->data.pin) 
      * SENSOR_DECAY;

    if(!tt->active && tt->signal<0.5){
    tt->active=true;
    CommManager::printf(comId, connId, "<Q %d>", tt->data.snum);
    } else if(tt->active && tt->signal>0.9){
    tt->active=false;
    CommManager::printf(comId, connId,"<q %d>", tt->data.snum);
    }
  } // loop over all sensors

}

Sensor *Sensor::create(int comId, int connId,int snum, int pin, int pullUp, int v){
  Sensor *tt;

  if(firstSensor==NULL){
    firstSensor=(Sensor *)calloc(1,sizeof(Sensor));
    tt=firstSensor;
  } else if((tt=get(snum))==NULL){
    tt=firstSensor;
    while(tt->nextSensor!=NULL)
    tt=tt->nextSensor;
    tt->nextSensor=(Sensor *)calloc(1,sizeof(Sensor));
    tt=tt->nextSensor;
  }

  if(tt==NULL){       // problem allocating memory
    if(v==1)
    CommManager::printf(comId, connId,"<X>");
    return(tt);
  }

  tt->data.snum=snum;
  tt->data.pin=pin;
  tt->data.pullUp=(pullUp==0?LOW:HIGH);
  tt->active=false;
  tt->signal=1;
  pinMode(pin,INPUT);         // set mode to input
  // Don't use Arduino's internal pull-up resistors for external infrared 
  // sensors --- each sensor must have its own 1K external pull-up resistor
  digitalWrite(pin,pullUp);   

  if(v==1)
    CommManager::printf(comId, connId,"<O>");
  return(tt);

}

Sensor* Sensor::get(int n){
  Sensor *tt;
  for(tt=firstSensor;tt!=NULL && tt->data.snum!=n;tt=tt->nextSensor);
  return(tt);
}

void Sensor::remove(int comId, int connId,int n){
  Sensor *tt,*pp;
  tt=firstSensor;
  pp=tt;

  for( ;tt!=NULL && tt->data.snum!=n;pp=tt,tt=tt->nextSensor);

  if(tt==NULL){
    CommManager::printf(comId, connId,"<X>");
    return;
  }

  if(tt==firstSensor)
    firstSensor=tt->nextSensor;
  else
    pp->nextSensor=tt->nextSensor;

  free(tt);

  CommManager::printf(comId, connId,"<O>");
}

void Sensor::show(int comId, int connId){
  Sensor *tt;

  if(firstSensor==NULL){
    CommManager::printf(comId, connId,"<X>");
    return;
  }

  for(tt=firstSensor;tt!=NULL;tt=tt->nextSensor){
    CommManager::printf(comId, connId,"<Q %d %d %d>", tt->data.snum, tt->data.pin, tt->data.pullUp);
  }
}

void Sensor::status(int comId, int connId){
  Sensor *tt;

  if(firstSensor==NULL){
    CommManager::printf(comId, connId,"<X>");
    return;
  }

  for(tt=firstSensor;tt!=NULL;tt=tt->nextSensor){
    CommManager::printf(comId, connId,"<%c %d>", tt->active?'Q':'q', tt->data.snum);
  }
}

void Sensor::load(int comId, int connId){
  struct SensorData data;
  Sensor *tt;

  for(int i=0;i<EEStore::eeStore->data.nSensors;i++){
    EEPROM.get(EEStore::pointer(),data);
    tt=create(comId, connId,data.snum,data.pin,data.pullUp);
    EEStore::advance(sizeof(tt->data));
  }
}

void Sensor::store(){
  Sensor *tt;

  tt=firstSensor;
  EEStore::eeStore->data.nSensors=0;

  while(tt!=NULL){
    EEPROM.put(EEStore::pointer(),tt->data);
    EEStore::advance(sizeof(tt->data));
    tt=tt->nextSensor;
    EEStore::eeStore->data.nSensors++;
  }
}

Sensor *Sensor::firstSensor=NULL;
