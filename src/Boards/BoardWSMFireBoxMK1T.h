/*
 *  BoardWSMFireBoxMK1T.h
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

#ifndef COMMANDSTATION_BOARDS_BOARDWSMFIREBOXMK1T_H_
#define COMMANDSTATION_BOARDS_BOARDWSMFIREBOXMK1T_H_

#include "Board.h"

struct BoardConfigWSMFireBoxMK1T : public BoardConfig {
  // Used to store functionality special to this type of shield
  int cutout_pin;
  int limit_pin;
};

class BoardWSMFireBoxMK1T : public Board
{
public:
  BoardConfigWSMFireBoxMK1T config;

  BoardWSMFireBoxMK1T(BoardConfigWSMFireBoxMK1T _config) {
    config = _config;
  }

  static void getDefaultConfigA(BoardConfigWSMFireBoxMK1T& _config) {
    _config.track_name = "A";
    _config.signal_a_pin = PIN_MAIN_CTRL_A;
    _config.signal_b_pin = PIN_MAIN_CTRL_B;
    _config.enable_pin = PIN_MAIN_SLEEP;
    _config.sense_pin = PIN_MAIN_CURRENT;   
    _config.board_voltage = 3.3;
    _config.amps_per_volt = 0.6;
    _config.current_trip = 5000;
    _config.current_trip_prog = 250;
    _config.prog_trip_time = 100;
    _config.main_preambles = 16;
    _config.prog_preambles = 22;
    _config.track_power_callback = nullptr; // Needs to be set in the main file
  
    _config.cutout_pin = PIN_RAILCOM_MAIN_CUTOUT;
    _config.limit_pin = PIN_MAIN_LIMIT;
  }

  static void getDefaultConfigB(BoardConfigWSMFireBoxMK1T& _config) {
    _config.track_name = "B";
    _config.signal_a_pin = PIN_PROG_CTRL_A;
    _config.signal_b_pin = PIN_PROG_CTRL_B;
    _config.enable_pin = PIN_PROG_SLEEP;
    _config.sense_pin = PIN_PROG_CURRENT;       
    _config.board_voltage = 3.3;
    _config.amps_per_volt = 0.6;
    _config.current_trip = 5000;
    _config.current_trip_prog = 250;
    _config.prog_trip_time = 100;
    _config.main_preambles = 16;
    _config.prog_preambles = 22;
    _config.track_power_callback = nullptr; // Needs to be set in the main file
  
    _config.cutout_pin = PIN_RAILCOM_PROG_CUTOUT;
    _config.limit_pin = PIN_PROG_LIMIT;
  }

  void setup();

  const char* getName();
  
  void power(bool, bool announce);
  void signal(bool);
  void cutout(bool);
  void progMode(bool);

  uint16_t getCurrentRaw();
  uint16_t getCurrentMilliamps();
  uint16_t getCurrentMilliamps(uint16_t reading);

  uint16_t setCurrentBase();
  uint16_t getCurrentBase();

  bool getStatus();

  void checkOverload();

  uint8_t getPreambles();
  
private:
  bool isCurrentLimiting();

  // Variable used to protect railcom circuit against track power being applied
  bool inCutout;
};


#endif