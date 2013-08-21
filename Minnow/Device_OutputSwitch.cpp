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
// Implementation for output switch devices
//

#include "Device_OutputSwitch.h"

uint8_t Device_OutputSwitch::output_switch_pins[MAX_OUTPUT_SWITCHES];
bool Device_OutputSwitch::output_switch_disabled[MAX_OUTPUT_SWITCHES];

//
// Methods
//

void Device_OutputSwitch::Init()
{
  memset(output_switch_pins, 0xFF, sizeof(output_switch_pins));
}

uint8_t Device_OutputSwitch::GetNumDevices()
{
  for (int8_t i=MAX_OUTPUT_SWITCHES-1; i>=0; i--)
  {
    if (output_switch_pins[i] != 0xFF)
      return i+1;
  }    
  return 0;
}


bool Device_OutputSwitch::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= MAX_OUTPUT_SWITCHES)
    return false;
  
  output_switch_pins[device_number] = pin;
  output_switch_disabled[device_number] = true;

  return true;
}


    