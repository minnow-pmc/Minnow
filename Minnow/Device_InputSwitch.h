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
      && input_switch_info[device_number].pin != 0xFF);
  }

  FORCE_INLINE static uint8_t GetPin(uint8_t device_number)
  {
    return input_switch_info[device_number].pin;
  }
  
  // returns APP_ERROR_TYPE_SUCCESS or error code
  static uint8_t SetPin(uint8_t device_number, uint8_t pin);
  
  // These are applied to/from NVConfigStore.  
  static uint8_t SetEnablePullup(uint8_t device_number, bool enable);
  static bool GetEnablePullup(uint8_t device_number);

  FORCE_INLINE static bool ReadState(uint8_t device_number)
  {
    const InputSwitchInfoInternal *info = &input_switch_info[device_number];
    return (*info->switch_register & info->switch_bit) != 0;
  }

private:

  // Ideally we keep the sizeof this struct as a power of 2 
  // so that accessing it doesn't require multiplication.
  struct InputSwitchInfoInternal
  {
    uint8_t pin;
    volatile uint8_t *switch_register;
    uint8_t switch_bit;
  };
  
  static uint8_t num_input_switches;
  static InputSwitchInfoInternal *input_switch_info;
};


#endif


    