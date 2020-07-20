/*
 *  Outputs.h
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

#ifndef COMMANDSTATION_ACCESSORIES_OUTPUTS_H_
#define COMMANDSTATION_ACCESSORIES_OUTPUTS_H_

#include <Arduino.h>

struct OutputData {
  uint8_t oStatus;
  uint8_t id;
  uint8_t pin; 
  uint8_t iFlag; 
};

struct Output {
  static Output *firstOutput;
  int num;
  struct OutputData data;
  Output *nextOutput;
  void activate(int comId, int connId,int s);
  static void parse(const char *c);
  static Output* get(int);
  static void remove(int comId, int connId,int);
  static void load(int,int);
  static void store();
  static Output *create(int , int ,int, int, int, int=0);
  static void show(int comId, int connId,int=0);
};
  
#endif  // COMMANDSTATION_ACCESSORIES_OUTPUTS_H_