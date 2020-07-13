/*
 *  StringFormatter.h
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

#ifndef COMMANDSTATION_COMMINTERFACE_WIFI_STRINGFORMATTER_H_
#define COMMANDSTATION_COMMINTERFACE_WIFI_STRINGFORMATTER_H_

#include <Arduino.h>

class StringFormatter {
public:
  static int parse(const char *com, int result[], byte maxResults);
  static void print(const __FlashStringHelper *input...);
  static void send(Print &serial, const __FlashStringHelper *input...);
private:
  static void send2(Print &serial, const __FlashStringHelper *input, va_list args);
};

#endif  // COMMANDSTATION_COMMINTERFACE_WIFI_STRINGFORMATTER_H_
