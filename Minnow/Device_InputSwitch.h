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

  static uint8_t Init(uint8_t num_input_switches);
  
  FORCE_INLINE static uint8_t GetNumDevices()
  {
    return num_input_switches;
  }
  
  FORCE_INLINE static bool IsInUse(uint8_t device_number)
  {
    return (device_number < num_input_switches 
      && input_switch_pins[device_number] != 0xFF);
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return input_switch_pins[device_number];
  }
  
  // returns APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);

  // Note: this read method is not used for time-critical input pins
  // such as stepper or heater pins or for enqueued commands
  FORCE_INLINE static uint8_t ReadState(uint8_t device_number)
  {
    return digitalRead(input_switch_pins[device_number]);
  }

private:
  static uint8_t num_input_switches;
  static uint8_t *input_switch_pins;
};

#endif


    