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
// Interface for buzzer devices
//

#include "Device_Buzzer.h"

uint8_t Device_Buzzer::buzzer_pins[MAX_BUZZERS];

//
// Methods
//

void Device_Buzzer::Init()
{
  memset(buzzer_pins, 0xFF, sizeof(buzzer_pins));
}

uint8_t Device_Buzzer::GetNumDevices()
{
  for (int8_t i=MAX_BUZZERS-1; i>=0; i--)
  {
    if (buzzer_pins[i] != 0xFF)
      return i+1;
  }    
  return 0;
}


bool Device_Buzzer::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= MAX_BUZZERS)
    return false;
  
  buzzer_pins[device_number] = pin;
  if (pin != 0xFF)
    pinMode(pin, OUTPUT);

  return true;
}

