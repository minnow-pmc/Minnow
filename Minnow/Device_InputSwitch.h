/*
 Minnow Pacemaker client firmware.
    
 Copyright (C) 2013 Robert Fairlie-Cuninghame

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

//
// Interface for input switch devices
//

#ifndef DEVICE_INPUTSWITCH_H
#define DEVICE_INPUTSWITCH_H

#include "Minnow.h"

class Device_InputSwitch
{
public:

  static void Init();
  static uint8_t GetNumDevices();
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < MAX_INPUT_SWITCHES 
      && input_switch_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return input_switch_pins[device_number];
  }
  
  static bool SetPin(uint8_t device_number, uint8_t pin);

  // Not: this write method is not used for time-critical input pins
  // such as stepper or heater pins
  // TODO ... but still, look at using digitalWriteFast library
  FORCE_INLINE static uint8_t ReadState(uint8_t device_number)
  {
    return digitalRead(input_switch_pins[device_number]);
  }

private:
  static uint8_t input_switch_pins[MAX_INPUT_SWITCHES];
};

#endif


    