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
// Implementation for pwm output devices
//

#include "Device_PwmOutput.h"

uint8_t Device_PwmOutput::pwm_output_pins[MAX_PWM_OUTPUTS];

//
// Methods
//

void Device_PwmOutput::Init()
{
  memset(pwm_output_pins, 0xFF, sizeof(pwm_output_pins));
}

uint8_t Device_PwmOutput::GetNumDevices()
{
  for (int8_t i=MAX_PWM_OUTPUTS-1; i>=0; i--)
  {
    if (pwm_output_pins[i] != 0xFF)
      return i+1;
  }    
  return 0;
}


bool Device_PwmOutput::SetPin(uint8_t device_number, uint8_t pin)
{
  if (device_number >= MAX_PWM_OUTPUTS)
    return false;
  
  pwm_output_pins[device_number] = pin;

  if (pin != 0xFF)
    pinMode(pin, OUTPUT);

  return true;
}

