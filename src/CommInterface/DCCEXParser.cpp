/*
 *  DCCEXParser.cpp
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

#include "DCCEXParser.h"

#include <inttypes.h>

#include "../Accessories/EEStore.h"
#include "../Accessories/Outputs.h"
#include "../Accessories/Sensors.h"
#include "../Accessories/Turnouts.h"
#include "../CommandStation.h"
#include "CommManager.h"

DCCMain* DCCEXParser::mainTrack;
DCCService* DCCEXParser::progTrack;

int DCCEXParser::p[MAX_PARAMS];

void DCCEXParser::init(DCCMain* mainTrack_, DCCService* progTrack_) {
  mainTrack = mainTrack_;
  progTrack = progTrack_;
} 

int DCCEXParser::stringParser(const char *com, int result[]) {
  byte state=1;
  byte parameterCount=0;
  int runningValue=0;
  const char * remainingCmd=com;  // skips the opcode
  bool signNegative=false;
  
  // clear all parameters in case not enough found
  for (int i=0;i<MAX_PARAMS;i++) result[i]=0;
  
  while(parameterCount<MAX_PARAMS) {
    char hot=*remainingCmd;
    switch (state) {
    case 1: // skipping spaces before a param
      if (hot==' ') break;
      if (hot == '\0' || hot=='>') return parameterCount;
      state=2;
      continue;
    case 2: // checking sign
      signNegative=false;
      runningValue=0;
      state=3; 
      if (hot!='-') continue; 
      signNegative=true;
      break; 
    case 3: // building a parameter   
      if (hot>='0' && hot<='9') {
        runningValue=10*runningValue+(hot-'0');
        break;
      }
      result[parameterCount] = runningValue * (signNegative ?-1:1);
      parameterCount++;
      state=1; 
      continue;
    }   
    remainingCmd++;
  }
  return parameterCount;
}

// See documentation on DCC class for info on this section
void DCCEXParser::parse(Print* stream, const char *com) {
  int numArgs = stringParser(com+1, p);
  
  switch(com[0]) {
  
/***** SET ENGINE THROTTLES USING 128-STEP SPEED CONTROL ****/

  case 't': {      // <t REGISTER CAB SPEED DIRECTION>
    setThrottleResponse throttleResponse;

    int speed=p[2];
    if (speed>126 || speed<-1) break; // invalid JMRI speed code
    if (speed<0) speed=1; // emergency stop DCC speed
    else if (speed>0) speed++; // map 1-126 -> 2-127

    uint8_t speedCode = (speed & 0x7F) + p[3] * 128;

    if(mainTrack->setThrottle(p[1], speedCode, throttleResponse) == ERR_OK)
      // TODO(davidcutting42@gmail.com): move back to throttleResponse struct items instead of p[]
      CommManager::send(stream, F("<T %d %d %d>"), throttleResponse.device, p[2], p[3]);
    
    break;
  }
  
/***** OPERATE ENGINE DECODER FUNCTIONS F0-F28 ****/

  case 'f': {       // <f CAB BYTE1 [BYTE2]>
    genericResponse response;
    
    if(numArgs == 2)
      mainTrack->setFunction(p[0], p[1], response);
    else 
      mainTrack->setFunction(p[0], p[1], p[2], response);
    
    // TODO use response?
    
    break;
  }

/***** OPERATE STATIONARY ACCESSORY DECODERS  ****/

  case 'a': {      // <a ADDRESS SUBADDRESS ACTIVATE>        
    genericResponse response;

    mainTrack->setAccessory(p[0], p[1], p[2], response);
    
    break;
  }
  
/***** CREATE/EDIT/REMOVE/SHOW & OPERATE A TURN-OUT  ****/

  case 'T':       // <T ID THROW>      
    Turnout *t;

    switch(numArgs){

    // argument is string with id number of turnout followed by zero (not 
    // thrown) or one (thrown)
    case 2:   
      t=Turnout::get(p[0]);
      if(t!=NULL)
        t->activate(stream, p[1], (DCCMain*) mainTrack);
      else
        CommManager::send(stream, F("<X>"));
      break;

    // argument is string with id number of turnout followed by an address and 
    // subAddress
    case 3:                     
      Turnout::create(stream, p[0],p[1],p[2],1);
      break;

    case 1:                     // argument is a string with id number only
      Turnout::remove(stream, p[0]);
      break;

    case 0:                    // no arguments
      Turnout::show(stream, 1);                  // verbose show
      break;
    }
    
    break;
  
/***** CREATE/EDIT/REMOVE/SHOW & OPERATE AN OUTPUT PIN  ****/

  case 'Z':       // <Z ID ACTIVATE>
    Output* o;

    switch(numArgs){
    
    // argument is string with id number of output followed by zero (LOW) or 
    // one (HIGH)
    case 2:                     
      o=Output::get(p[0]);
      if(o!=NULL)
        o->activate(stream, p[1]);
      else
        CommManager::send(stream, F("<X>"));
      break;

    // argument is string with id number of output followed by a pin number and 
    // invert flag
    case 3:                     
      Output::create(stream, p[0],p[1],p[2],1);
      break;

    case 1:                     // argument is a string with id number only
      Output::remove(stream, p[0]);
      break;

    case 0:                    // no arguments
      Output::show(stream, 1);                  // verbose show
      break;
    }
    
    break;
  
/***** CREATE/EDIT/REMOVE/SHOW A SENSOR  ****/

  case 'S':
    switch(numArgs){

    // argument is string with id number of sensor followed by a pin number and 
    // pullUp indicator (0=LOW/1=HIGH)
    case 3:                     
      Sensor::create(stream, p[0],p[1],p[2],1);
      break;

    case 1:                     // argument is a string with id number only
      Sensor::remove(stream, p[0]);
      break;

    case 0:                    // no arguments
      Sensor::show(stream);
      break;

    case 2:                     // invalid number of arguments
      CommManager::send(stream, F("<X>"));
      break;
    }
  
    break;

/***** SHOW STATUS OF ALL SENSORS ****/

  case 'Q':         // <Q>
    Sensor::status(stream);
    break;


/***** WRITE CONFIGURATION VARIABLE BYTE TO ENGINE DECODER ON MAIN TRACK  ****/

  case 'w': {     // <w CAB CV VALUE>
    genericResponse response;

    mainTrack->writeCVByteMain(p[0], p[1], p[2], response, stream, POMResponse);
    
    break;
  }

/***** WRITE CONFIGURATION VARIABLE BIT TO ENGINE DECODER ON MAIN TRACK  ****/

  case 'b': {     // <b CAB CV BIT VALUE>
    genericResponse response;

    mainTrack->writeCVBitMain(p[0], p[1], p[2], p[3], response, stream, POMResponse);
    
    break;
  }

/***** WRITE CONFIGURATION VARIABLE BYTE TO ENGINE DECODER ON PROG TRACK  ****/

  case 'W':      // <W CV VALUE CALLBACKNUM CALLBACKSUB>

    progTrack->writeCVByte(p[0], p[1], p[2], p[3], stream, cvResponse);

    break;

/***** WRITE CONFIGURATION VARIABLE BIT TO ENGINE DECODER ON PROG TRACK  ****/

  case 'B':      // <B CV BIT VALUE CALLBACKNUM CALLBACKSUB>
    
    progTrack->writeCVBit(p[0], p[1], p[2], p[3], p[4], stream, cvResponse);
    
    break;

/***** READ CONFIGURATION VARIABLE BYTE FROM ENGINE DECODER ON PROG TRACK  ****/

  case 'R':     // <R CV CALLBACKNUM CALLBACKSUB>        
    progTrack->readCV(p[0], p[1], p[2], stream, cvResponse);

    break;

/***** READ CONFIGURATION VARIABLE BYTE FROM RAILCOM DECODER ON MAIN TRACK ****/

  case 'r': {   // <r CAB CV>
    genericResponse response;

    mainTrack->readCVByteMain(p[0], p[1], response, stream, POMResponse);
    break;
    }

/***** READ 4 CONFIGURATION VARIABLE BYTES FROM RAILCOM DECODER ON MAIN  ****/

  case 'm': { // <m CAB CV>
    genericResponse response;

    mainTrack->readCVBytesMain(p[0], p[1], response, stream, POMResponse);
    break;
    }
/***** TURN ON POWER FROM MOTOR SHIELD TO TRACKS  ****/

  case '1':      // <1>
    mainTrack->board->power(true, false);
    progTrack->board->power(true, false);
    CommManager::broadcast(F("<p1>"));
    break;

/***** TURN OFF POWER FROM MOTOR SHIELD TO TRACKS  ****/

  case '0':     // <0>
    mainTrack->board->power(false, false);
    progTrack->board->power(false, false);
    CommManager::broadcast(F("<p0>"));
    break;

/***** READ MAIN OPERATIONS TRACK CURRENT  ****/

  case 'c':     // <c>
    // TODO(davidcutting42@gmail.com): When JMRI moves to milliamp reporting, 
    // fix this.
    int currRead;
    currRead = mainTrack->board->getCurrentRaw();
    CommManager::send(stream, F("<a %d>"), currRead);
    break;

/***** READ STATUS OF DCC++ BASE STATION  ****/

  case 's':      // <s>
    trackPowerCallback(mainTrack->board->getName(), mainTrack->board->getStatus());
    trackPowerCallback(progTrack->board->getName(), progTrack->board->getStatus());
    //  TODO(davidcutting42@gmail.com): Add throttle status notifications back
    CommManager::send(stream, 
        F("<iDCC++ EX CommandStation / %s: V-%s / %s %s>"), 
        "CommandStation", BOARD_NAME, VERSION, __DATE__, __TIME__);
    CommManager::showInitInfo();
    Turnout::show(stream);
    Output::show(stream);

    break;

/***** STORE SETTINGS IN EEPROM  ****/

  case 'E':     // <E>
    EEStore::store();
    CommManager::send(stream, F("<e %d %d %d>"), EEStore::eeStore->data.nTurnouts, 
      EEStore::eeStore->data.nSensors, EEStore::eeStore->data.nOutputs);
    break;

/***** CLEAR SETTINGS IN EEPROM  ****/

  case 'e':     // <e>

    EEStore::clear();
    CommManager::send(stream, F("<O>"));
    break;

/***** PRINT CARRIAGE RETURN IN SERIAL MONITOR WINDOW  ****/

  case ' ':     // < >
    CommManager::send(stream, F("\n"));
    break;
  }
}

void DCCEXParser::cvResponse(Print* stream, serviceModeResponse response) {
  switch (response.type)
  {
  case READCV:
  case WRITECV:
    CommManager::send(stream, F("<r%d|%d|%d %d>"), response.callback, 
      response.callbackSub, response.cv, response.cvValue);
    break;
  case WRITECVBIT:
    CommManager::send(stream, F("<r%d|%d|%d %d %d>"), response.callback, 
      response.callbackSub, response.cv, response.cvBitNum, response.cvValue);
    break;
  }
}

void DCCEXParser::POMResponse(Print* stream, RailcomPOMResponse response) {
  CommManager::send(stream, F("<k %d %x>"), response.transactionID, response.data);
}

void DCCEXParser::trackPowerCallback(const char* name, bool status) {
  if(status) 
    CommManager::broadcast(F("<p1 %s>"), name);
  else 
    CommManager::broadcast(F("<p0 %s>"), name);
}