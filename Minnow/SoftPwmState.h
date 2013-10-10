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

#ifndef SOFT_PWM_STATE_H
#define SOFT_PWM_STATE_H

#include "Minnow.h"

//
// Class to store additional state for soft pwm when enabled for a device class
//
class SoftPwmState
{
public:
  bool Init(uint8_t num_devices);
  
  bool EnableSoftPwm(uint8_t device_number, uint8_t pin, bool enable);
  
  FORCE_INLINE void SetPower(uint8_t device_number, uint8_t power)
  {
    if (device_number < num_devices)
    {
      if (power != 0xFF)
        pwm_power[device_number] = power / 2;
      else
        pwm_power[device_number] = 0x80; // power always on
    }
  }

private:
  friend void updateSoftPwm();
  
  uint8_t num_devices;
  
  uint8_t *pwm_power; // desired duty cycle for each device
  uint8_t *output_bit;
  uint8_t **output_reg;
  
  uint8_t *pwm_count; // raw pwm count for each device
};


#endif