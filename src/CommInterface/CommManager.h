/*
 *  CommManager.h
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

#ifndef COMMANDSTATION_COMMINTERFACE_COMMMANAGER_H_
#define COMMANDSTATION_COMMINTERFACE_COMMMANAGER_H_

#include "CommInterface.h"
#include "../DCC/DCCMain.h"
#include "../DCC/DCCService.h"

#if defined(ARDUINO_ARCH_SAMD)
  // Some processors use a gcc compiler that renames va_list!!!
  #include <cstdarg>
  #define DIAGSERIAL SerialUSB
#elif defined(ARDUINO_ARCH_SAMC)
  #include <cstdarg>
  #define DIAGSERIAL Serial
#elif defined(ARDUINO_ARCH_AVR)
  #define DIAGSERIAL Serial
#elif defined(ARDUINO_ARCH_MEGAAVR)
  #define DIAGSERIAL Serial
  #define __FlashStringHelper char
#endif

class CommManager {
public:
  static void update();
  static void registerInterface(CommInterface *interface);
  static void showConfiguration();
  static void showInitInfo();
  static void broadcast(const __FlashStringHelper* input, ...);
  static void print(const __FlashStringHelper* input, ...);
  static void send(Print* stream, const __FlashStringHelper* input, ...);
  static void printEscapes(Print* stream, char * input);
  static void printEscapes(Print* stream, const __FlashStringHelper* input);
  static void printEscape(Print* stream, char c);
private:
  static void send2(Print* stream, const __FlashStringHelper* format, va_list args);
  static CommInterface *interfaces[5];
  static int nextInterface;
};

#endif	// COMMANDSTATION_COMMINTERFACE_COMMMANAGER_H_