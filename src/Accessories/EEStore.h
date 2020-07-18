/*
 *  EEStore.h
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

#ifndef COMMANDSTATION_ACCESSORIES_EESTORE_H_
#define COMMANDSTATION_ACCESSORIES_EESTORE_H_

#include <Arduino.h>

#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAMC)
#include <SparkFun_External_EEPROM.h>
#endif

#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAMC)
extern ExternalEEPROM EEPROM;
#endif

#define EESTORE_ID "DCC++"

struct EEStoreData{
  char id[sizeof(EESTORE_ID)];
  int nTurnouts;
  int nSensors;  
  int nOutputs;
};

struct EEStore{
  static EEStore *eeStore;
  EEStoreData data;
  static int eeAddress;
  static void init();
  static void reset();
  static int pointer();
  static void advance(int);
  static void store();
  static void clear();
};

#endif  // COMMANDSTATION_ACCESSORIES_EESTORE_H_